#include "libminiomp.h"

// Library constructor and desctructor
void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

void
init_miniomp(void) {
  // Parse OMP_NUM_THREADS environment variable to initialize nthreads_var internal control variable
  parse_env();
  // Initialize pthread and parallel data structures 

  // Initialize Pthread thread-specific data, now just used to store the OpenMP thread identifier
  pthread_key_create(&miniomp_specifickey, NULL);
  pthread_setspecific(miniomp_specifickey, (void *) 0); // implicit initial pthread with id=0. Así cuando se haga un get_thread_num fuera de una región paralela, se devolvera un 0.

  // Initialize OpenMP default locks and default barrier
  pthread_mutex_init(&miniomp_default_lock, NULL);
  pthread_mutex_init(&lock_first_in_named_critical, NULL);
  indice_vect_mutex_named_critical = 0;
  
  mibarrier.han_entrado = 0; mibarrier.num_threads_barrier = 1; mibarrier.flag = 0;
  pthread_key_create(&local_sense_key, NULL); // "sense" variable (local por thread) para el "sense reverse barrier" casero.
  pthread_setspecific(local_sense_key, 0);

  //Lo relativo a la pool de threads:
  miniomp_threads  = malloc(MAX_THREADS * sizeof(pthread_t));
  miniomp_parallel = malloc(MAX_THREADS * sizeof(miniomp_parallel_t));
  sem_synchro = malloc(MAX_THREADS * sizeof(sem_t));
  cpu_set_t cpuset;
  int num_cpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
  for (int i=0; i<MAX_THREADS; i++){
	sem_init(&sem_synchro[i], 0, 0); //sem de sincronización para que el parallel diga a cada thread cuando deben entrar en una region paralela
	miniomp_parallel[i].id = i;
	pthread_create(&miniomp_threads[i], NULL, &worker, &miniomp_parallel[i]); //en worker() se quedara bloqueado hasta que GOMP_parallel le de trabajo 
	// Bind thread 'i' to processor 'i':
	CPU_ZERO(&cpuset);
	CPU_SET(i%num_cpus, &cpuset);
	pthread_setaffinity_np(miniomp_threads[i], sizeof(cpu_set_t), &cpuset);
  }
  sem_init(&sem_threads_parallel_finalizados, 0, 0); //sem de sincronizacion para que el ultimo thread del parallel le diga al padre que ya ha acabado
   
  // Initialize OpenMP task queue for task and taskloop
  miniomp_taskqueue = TQinit(MAXELEMENTS_TQ);
  tasks_pendientes_finalizar = 0;
  generador_tag_taskgroup = 0;
  for (int i = 0; i < MAX_THREADS; i++) miniomp_parallel[i].tag_taskgroup_actual = -1;
}

void
fini_miniomp(void) {
  //destruir los threads de la pool de threads:
  for (int i=0; i<MAX_THREADS; i++){
	pthread_cancel(miniomp_threads[i]);
	pthread_join(miniomp_threads[i], NULL);
  }
  free(miniomp_threads);
  free(miniomp_parallel);

  // delete Pthread thread-specific data
  pthread_key_delete(miniomp_specifickey);

  // free other data structures allocated during library initialization
  pthread_mutex_destroy(&miniomp_default_lock);
  pthread_mutex_destroy(&lock_first_in_named_critical);
  for(int i = 0; i<MAX_THREADS; i++) sem_destroy(&sem_synchro[i]);
  free(sem_synchro);
  sem_destroy(&sem_threads_parallel_finalizados);

  pthread_key_delete(local_sense_key);

  //liberar la region conseguida con mallocs para los mutex de los named criticals:
  for(int i = 0; i < min(indice_vect_mutex_named_critical, MAX_NAMED_CRITICAL); i++){
	free(mutex_pointer_named_critical[i]);
  }

  //Destruccion de la cola de tasks:
  TQdestroy(miniomp_taskqueue);
}
