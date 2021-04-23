#include <pthread.h>

// Default lock for unnamed critical sections
extern pthread_mutex_t miniomp_default_lock;
extern pthread_mutex_t miniomp_named_lock;

extern pthread_mutex_t lock_first_in_named_critical; 

extern int indice_vect_mutex_named_critical;
extern pthread_mutex_t* mutex_pointer_named_critical[64];

// Default barrier within a parallel region
extern pthread_barrier_t miniomp_barrier;

// Functions implemented in this module
void GOMP_critical_start (void);
void GOMP_critical_end (void);
void GOMP_critical_name_start (void **pptr);
void GOMP_critical_name_end (void **pptr);
void GOMP_barrier(void);

typedef struct{
	int han_entrado;
	int num_threads_barrier;
	int flag;
} mibarrier_t;

extern mibarrier_t mibarrier;
extern pthread_key_t local_sense_key; // "sense" variable (local por thread) para el "sense reverse barrier" casero.

