#include "libminiomp.h"

// Default lock for unnamed critical sections
pthread_mutex_t miniomp_default_lock;

// Lock para la situación de que 2 threads entren ambos a la vez por primera vez en un named critical, para que no miren a la vez si son el primero en entrar en ese named critical:
pthread_mutex_t lock_first_in_named_critical; 

pthread_mutex_t* mutex_pointer_named_critical[64]; //este vector lo usaremos para almacenar todos punteros a mutex de named criticals que hayamos creado durante la ejecución del programa. Si hemos creado más de MAX_NAMED_CRITICAL pues algunos de ellos compartiran mutex.

int indice_vect_mutex_named_critical;

void 
GOMP_critical_start (void) {
  pthread_mutex_lock(&miniomp_default_lock);
}

void 
GOMP_critical_end (void) {
  pthread_mutex_unlock(&miniomp_default_lock);
}

void 
GOMP_critical_name_start (void **pptr) {
  pthread_mutex_t * plock = *pptr;
  
  /* if plock is NULL it means that the lock associated to the name has not yet been allocated and initialized: */
  if (plock == NULL) { // test test and set
	pthread_mutex_lock(&lock_first_in_named_critical); // si 2 threads han entrado a un named critical a la misma vez, solamente 1 de ellos debe acabar haciendo el malloc para el mutex que se asociará a ese named critical.
	if(plock == NULL){
		if(indice_vect_mutex_named_critical < MAX_NAMED_CRITICAL){
			plock = malloc(sizeof(pthread_mutex_t)); 
			*pptr = plock; // ver explicacion de esto mas abajo
			mutex_pointer_named_critical[indice_vect_mutex_named_critical] = plock;
			++indice_vect_mutex_named_critical;
		}
		else {
			plock = mutex_pointer_named_critical[(indice_vect_mutex_named_critical % MAX_NAMED_CRITICAL)]; //compartira mutex con otro named critical
			++indice_vect_mutex_named_critical;
		}
	}
	pthread_mutex_unlock(&lock_first_in_named_critical);
  }
  
  pthread_mutex_lock(plock);
}

void 
GOMP_critical_name_end (void **pptr) {
  pthread_mutex_t * plock = *pptr;

  /* if plock is still NULL something went wrong */
  if (plock == NULL) {
	printf(" ERROR critical_name_end");
	exit(1);
  }
   
  pthread_mutex_unlock(plock); //plock ya es un puntero a mutex, por eso no pongo el &
}

// Default barrier within a parallel region
mibarrier_t mibarrier;
pthread_key_t local_sense_key; // "sense" variable (local por thread) para el "sense reverse barrier" casero.


// Basado en la parte de "sense reversal" de http://15418.courses.cs.cmu.edu/spring2013/article/43
void 
GOMP_barrier() {

  long local_sense = (long)pthread_getspecific(local_sense_key);
  local_sense = !local_sense;
  pthread_setspecific(local_sense_key, (void*)local_sense);

  ejecutar_tasks_taskqueue(miniomp_taskqueue); // retornará sin ejecutar ninguna si no quedan. 

  int han_entrado = __sync_add_and_fetch (&mibarrier.han_entrado, 1); 

  if(han_entrado == mibarrier.num_threads_barrier) { 
	mibarrier.han_entrado = 0;
	mibarrier.flag = local_sense;
  } else {
	while(mibarrier.flag != local_sense){
		__sync_synchronize(); 
		ejecutar_tasks_taskqueue(miniomp_taskqueue); 
	} 
  }

}


