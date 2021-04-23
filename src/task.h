/* This structure describes a "task" to be run by a thread.  */
typedef struct {
    void (*fn)(void *);
    void (*data);
    miniomp_parallel_t * thread_creador_task_en_taskgroup; //puntero al thread que creó la task ÚNICAMENTE SI LA TASK FUE CREADA DENTRO DE UN TASKGROUP. NULL si no.
} miniomp_task_t;

typedef struct {
    int max_elements;
    int size;
    int head; //apunta al elemento de donde toca leer en caso de no estar vacia
    int tail; //apunta al elemento donde toca llenar en caso de no estar vacia 
    int lock_queue; 
    miniomp_task_t *queue;
} miniomp_taskqueue_t;

extern miniomp_taskqueue_t * miniomp_taskqueue;
#define MAXELEMENTS_TQ 128

extern int tasks_pendientes_finalizar;

extern int generador_tag_taskgroup; //su valor será usado como tag del siguiente taskgroup que haya. Su modificacion debe ser atómica

// funtions to implement basic management operations on taskqueue
miniomp_taskqueue_t *TQinit(int max_elements);
void TQdestroy(miniomp_taskqueue_t *task_queue);
bool TQis_empty(miniomp_taskqueue_t *task_queue);
bool TQis_full(miniomp_taskqueue_t *task_queue) ;
bool TQenqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t task_descriptor); 
miniomp_task_t TQdequeue(miniomp_taskqueue_t *task_queue);
miniomp_task_t TQfirst(miniomp_taskqueue_t *task_queue); 


void ejecutar_tasks_taskqueue(miniomp_taskqueue_t *taskqueue);


// Functions implemented in task* modules
void GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags,
           void **depend, int priority);
void GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
               long arg_size, long arg_align, unsigned flags,
               unsigned long num_tasks, int priority,
               long start, long end, long step);
void GOMP_taskwait (void);
void GOMP_taskgroup_start (void);
void GOMP_taskgroup_end (void);
void GOMP_taskgroup_reduction_register (uintptr_t *data);
void GOMP_taskgroup_reduction_unregister (uintptr_t *data);
void GOMP_task_reduction_remap (size_t cnt, size_t cntorig, void **ptrs);

