#include "libminiomp.h"
#include <stdlib.h>


int generador_tag_taskgroup; //su valor será usado como tag del siguiente taskgroup que haya. Su modificacion debe ser atómica

void
GOMP_taskwait (void)
{
	while (tasks_pendientes_finalizar > 0){
		__sync_synchronize(); 
		ejecutar_tasks_taskqueue(miniomp_taskqueue);
	} 
}

void
GOMP_taskgroup_start (void)
{
    int id = omp_get_thread_num();
    int tag_taskgroup = __sync_fetch_and_add(&generador_tag_taskgroup, 1);
    miniomp_parallel[id].tag_taskgroup_actual = tag_taskgroup; 
    miniomp_parallel[id].tasks_taskgroup_actual_por_finalizar_ejecucion = 0; 
}

void
GOMP_taskgroup_end (void)
{
    int id = omp_get_thread_num();
    while(miniomp_parallel[id].tasks_taskgroup_actual_por_finalizar_ejecucion > 0){
	__sync_synchronize(); 
        ejecutar_tasks_taskqueue(miniomp_taskqueue);
    } 
    miniomp_parallel[id].tag_taskgroup_actual = -1; 
}
