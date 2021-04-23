#include <pthread.h>

// Declaration of array for storing pthread identifiers from pthread_create function
extern pthread_t *miniomp_threads;

// Type declaration for parallel descriptor (arguments needed to create pthreads). INFO RELATIVA A UN CIERTO THREAD.
typedef struct {
    void (*fn) (void *);
    void *fn_data;
    int id;
    int contSingle; //para contar en cuantos singles del parallel en el que se encuentra ha entrado un cierto thread
    int tag_taskgroup_actual; // tag del taskgroup en el que se encuentra un cierto thread (-1 si ninguno).
    unsigned tasks_taskgroup_actual_por_finalizar_ejecucion; // si estamos en un taskgroup, esto dirá cuántas tasks se han creado en el taskgroup que aún no han finalizado.
} miniomp_parallel_t;

extern miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
extern pthread_key_t miniomp_specifickey;

// Para que desde GOMP_parallel se pueda decir a un thread que empiece
extern sem_t *sem_synchro;

// Para que el ultimo thread del parallel le pueda decir al creador que ya han acabado:
extern sem_t sem_threads_parallel_finalizados;

// Para contar, en un cierto parallel, a cuántos singles distintos he entrado
extern int contSingle;

// Functions implemented in this module
void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags);
void* worker(void* args); 
