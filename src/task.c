#include "libminiomp.h"

miniomp_taskqueue_t * miniomp_taskqueue; // REALMENTE LO HE IMPLEMENTADO COMO UN BUFFER CIRCULAR

int tasks_pendientes_finalizar; //lo utilizaremos por ejemplo en el taskwait, ya que no es suficiente con que la taskqueue este vacia sino que se han debido de acabar de ejecutar las tasks

// Initializes the task queue
miniomp_taskqueue_t *TQinit(int max_elements) {
    miniomp_taskqueue_t * task_queue = malloc(sizeof(miniomp_taskqueue_t));
    task_queue->max_elements = max_elements;
    task_queue->head = task_queue->tail = task_queue->size = 0;
    task_queue->queue = malloc(sizeof(miniomp_task_t*));
    task_queue->lock_queue = 0;
    return task_queue;
}

// Destruye una the task queue
void TQdestroy(miniomp_taskqueue_t *task_queue) {
    if(task_queue != NULL){
	free(task_queue->queue);
	free(task_queue);
    }
}

// Checks if the task queue is empty
bool TQis_empty(miniomp_taskqueue_t *task_queue) {
    return (task_queue->size == 0);
}

// Checks if the task queue is full
bool TQis_full(miniomp_taskqueue_t *task_queue) {
    return (task_queue->size == task_queue->max_elements);
}

// Enqueues the task descriptor at the tail of the task queue
bool TQenqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t task_descriptor) {

    bool successful;
    do{ //cogemos el lock
	while(task_queue->lock_queue == 1) __sync_synchronize(); 
        successful = __sync_bool_compare_and_swap(&task_queue->lock_queue, 0, 1);
    } while(!successful);
    
    bool se_ha_podido_encolar;
    if(TQis_full(task_queue)){
	se_ha_podido_encolar = false;
    } else {
        task_queue->queue[task_queue->tail] = task_descriptor;
        task_queue->tail = (task_queue->tail + 1) % task_queue->max_elements;
	(task_queue->size)++;
	se_ha_podido_encolar = true;
    }

    task_queue->lock_queue = 0; //liberamos lock
    return se_ha_podido_encolar;

}

// Dequeue the task descriptor at the head of the task queue
miniomp_task_t TQdequeue(miniomp_taskqueue_t *task_queue) { 
    //ojo, a este punto han podido entrar más threads que tasks restantes en la taskqueue.
    bool successful;
    do{ //cogemos el lock
	while(task_queue->lock_queue == 1) __sync_synchronize(); 
        successful = __sync_bool_compare_and_swap(&task_queue->lock_queue, 0, 1); 
    } while(!successful);
    
    miniomp_task_t task_desencolada;
    if(TQis_empty(task_queue)) {
        task_desencolada.fn = NULL; 
    } else{
		task_desencolada = task_queue->queue[task_queue->head];	
		task_queue->head = (task_queue->head + 1) % task_queue->max_elements;
		(task_queue->size)--;
    }

    task_queue->lock_queue = 0; //liberamos lock
    return task_desencolada;

}



// Returns the task descriptor at the head of the task queue.
// NOTA-> DE LA FORMA QUE HE PENSADO UTILIZAR LA COLA Y SUS FUNCIONES, CREO QUE ESTA FUNCION ES INUTIL
miniomp_task_t TQfirst(miniomp_taskqueue_t *task_queue) {
    bool successful;
    do{ //cogemos el lock
	    while(task_queue->lock_queue == 1) __sync_synchronize(); 
        successful = __sync_bool_compare_and_swap(&task_queue->lock_queue, 0, 1);
    } while(!successful);
    
    miniomp_task_t task_first;
    if(TQis_empty(task_queue)) {
        task_first.fn = NULL; //no hay nadie en la primera posicion
    } else{
		task_first = task_queue->queue[task_queue->head];
    }

    task_queue->lock_queue = 0; //liberamos lock
    return task_first;
}

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)
#define GOMP_TASK_FLAG_REDUCTION        (1 << 12)


void ejecutar_tasks_taskqueue(miniomp_taskqueue_t *taskqueue){
	while(!TQis_empty(taskqueue)){
		miniomp_task_t task = TQdequeue(taskqueue);
		if(task.fn != NULL){ //Es posible que 2 threads vean a la vez que !TQisempty pero realmente solo quedaba 1 task.
				     // Así que a 1 de ellos el dequeue les habrá retornado una task inexistente con fn=NULL. El desencolar lo hace atomicamente.
			task.fn(task.data);
			if (task.thread_creador_task_en_taskgroup != NULL){
				__sync_add_and_fetch(&task.thread_creador_task_en_taskgroup->tasks_taskgroup_actual_por_finalizar_ejecucion, -1); 
			}
			__sync_add_and_fetch(&tasks_pendientes_finalizar,-1);
		}
	}
}


// Called when encountering an explicit task directive. Arguments are:
//      1. void (*fn) (void *): the generated outlined function for the task body
//      2. void *data: the parameters for the outlined function
//      3. void (*cpyfn) (void *, void *): copy function to replace the default memcpy() from 
//                                         function data to each task's data
//      4. long arg_size: specify the size of data
//      5. long arg_align: alignment of the data
//      6. bool if_clause: the value of if_clause. true --> 1, false -->0; default is set to 1 by compiler
//      7. unsigned flags: see list of the above

void
GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags,
           void **depend, int priority)
{
    // This part of the code appropriately copies data to be passed to task function,
    // either using a compiler cpyfn function or just memcopy otherwise; no need to
    // fully understand it for the purposes of this assignment
    char *arg;
    if (__builtin_expect (cpyfn != NULL, 0)) {
	      char *buf =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          arg       = (char *) (((uintptr_t) buf + arg_align - 1)
                               & ~(uintptr_t) (arg_align - 1));
          cpyfn (arg, data);
    } else {
          arg       =  malloc(sizeof(char) * (arg_size + arg_align - 1));
          memcpy (arg, data, arg_size);
    }
 
    int id = omp_get_thread_num();

    __sync_add_and_fetch(&tasks_pendientes_finalizar,1);

    if(omp_get_num_threads() == 1){
		fn(arg); //la ejecuto yo porque NO estoy dentro de una región paralela
		__sync_add_and_fetch(&tasks_pendientes_finalizar,-1);
    }
    else{
		miniomp_task_t task_descriptor;
		task_descriptor.fn = fn;
		task_descriptor.data = arg; 
		if (miniomp_parallel[id].tag_taskgroup_actual != -1) { // esta task se está creando dentro de un taskgroup
			__sync_add_and_fetch(&miniomp_parallel[id].tasks_taskgroup_actual_por_finalizar_ejecucion, +1); 
			task_descriptor.thread_creador_task_en_taskgroup = &miniomp_parallel[id]; 
		} else task_descriptor.thread_creador_task_en_taskgroup = NULL;
		bool encolado = TQenqueue(miniomp_taskqueue, task_descriptor); // no encola si la cola está llena.
		if(!encolado){ 
			fn(arg); //si no la pudo encolar porque la cola estaba llena, la ejecuto yo mismo.
			if (miniomp_parallel[id].tag_taskgroup_actual != -1) { 
				__sync_add_and_fetch(&miniomp_parallel[id].tasks_taskgroup_actual_por_finalizar_ejecucion, -1); 
			}
			__sync_add_and_fetch(&tasks_pendientes_finalizar,-1);
		} 
    }
    

}
