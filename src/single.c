#include "libminiomp.h"

// Declaratiuon of global variable for single work descriptor
miniomp_single_t miniomp_single; 

// This routine is called when first encountering a SINGLE construct. 
// Returns true if this is the thread that should execute the clause. (Debe retornar true el primero en entrar)
bool
GOMP_single_start (void) {
  if(omp_get_num_threads() == 1) 
	return true;
  int id = omp_get_thread_num();
  int singles_entrados = ++miniomp_parallel[id].contSingle;
  return __sync_bool_compare_and_swap(&contSingle, singles_entrados-1, singles_entrados); 
}
