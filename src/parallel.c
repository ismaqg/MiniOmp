#include "libminiomp.h"

// This file implements the PARALLEL construct as part of your 
// miniomp runtime library. parallel.h contains definitions of 
// data types used here

// Declaration of array for storing pthread identifier from 
// pthread_create function
pthread_t *miniomp_threads;

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

// Para que desde GOMP_parallel se pueda decir a un thread que empiece:
sem_t *sem_synchro;

// Para que el ultimo thread del parallel le pueda decir al creador que ya han acabado:
sem_t sem_threads_parallel_finalizados;

// Threads restantes por acabar el parallel:
int threads_restantes;

// Para contar, en un cierto parallel, a cuántos singles distintos he entrado
int contSingle;

// This is the prototype for the Pthreads starting function
void *
worker(void *args) {

  miniomp_parallel_t * worker_thread = args;

  // save thread-specific data
  pthread_setspecific(miniomp_specifickey, (void*)(long) worker_thread->id);  

  while(1){ // tendré a los MAX_THREADS en el sem_wait bloqueados hasta que el creador les desbloquee porque tienen que actuar en una region paralela y luego se volveran a bloquear
	pthread_setspecific(local_sense_key, 0); 
	sem_wait(&sem_synchro[worker_thread->id]); //bloqueamos hasta que el creador les despierte porque deben actuar en una cierta region paralela

	// invoke the per-threads instance of function encapsulating the parallel region
	worker_thread->fn(worker_thread->fn_data); 

	ejecutar_tasks_taskqueue(miniomp_taskqueue);

	if(__sync_add_and_fetch (&threads_restantes, -1) == 0) sem_post(&sem_threads_parallel_finalizados);
	else {
		while(threads_restantes > 0){
			__sync_synchronize(); 
			ejecutar_tasks_taskqueue(miniomp_taskqueue); //hay que seguir haciendo esto mientras sigan threads en la región paralela porque esos threads han podido crear nuevas tasks!
		}			
	}	
  }

}

void
GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {

  if(!num_threads) num_threads = miniomp_icv.nthreads_var; 
  miniomp_icv.nthreads_en_este_instante = num_threads;

  /* inicializo / reinicializo la barrier para los N threads de este parallel */
  mibarrier.han_entrado = 0; mibarrier.num_threads_barrier = num_threads; mibarrier.flag = 0;

  /* inicializo lo necesario a los singles que hayan dentro de este parallel (está hecho también para que puedan ser varios) */
  miniomp_single.counter = 0; miniomp_single.num_threads = num_threads; 

  threads_restantes = num_threads;
  contSingle = 0;
  for (int i=0; i<num_threads; i++){
	miniomp_parallel[i].fn = fn;
	miniomp_parallel[i].fn_data = data;
	miniomp_parallel[i].contSingle = 0;

	sem_post(&sem_synchro[i]);
  }
  sem_wait(&sem_threads_parallel_finalizados);

  // En este punto ya ha ocurrido la barrera implicita del final del parallel y voy a salir de la región paralela.

  // Ahora vuelve a haber solo 1 thread:
  miniomp_icv.nthreads_en_este_instante = 1;
 
  /*vuelvo a poner numthreads del single a '1' por si me usan un single fuera de un parallel que no reviente todo*/
  miniomp_single.num_threads = 1; 
  
  /* vuelvo a hacer que la barrier sea de solo 1 thread por si la usan fuera de un parallel que no se joda el codigo*/
  mibarrier.han_entrado = 0; mibarrier.num_threads_barrier = 1; mibarrier.flag = 0;
  pthread_setspecific(local_sense_key, 0);
}
