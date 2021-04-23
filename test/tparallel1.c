#include <stdio.h>
#include <stdlib.h>
#include <omp.h>	/* OpenMP */

int first=0, second=0;

int foo() {
    int i, x = 1023;
    #pragma omp parallel firstprivate(x) reduction(+:first) if(x>0) num_threads(2) //nota el num_threads=2
    {
    x++; 
    first += x; //first += numthreads*x -> first = 2 * 1024 = 2048
    printf("omp_num_threads en el primer parallel  (DEBERIA SER 2): %d\n", omp_get_num_threads());
    } //barrera implícita

    printf("omp_num_threads fuera de los parallel (DEBERIA SER 1) : %d\n", omp_get_num_threads());

    #pragma omp parallel firstprivate(x) reduction(+:first) if(0) //por el if(0), esto solo lo ejecuta 1 thread
    {
    x++; 
    first += x; //first = 2048 + 1024 = 3096
    }

    

    #pragma omp parallel private(i) shared(first) reduction(+:second) 
    {
    printf("omp_num_threads en el tercer parallel  (DEBERIA SER 4): %d\n", omp_get_num_threads());
    second = first; //second = 3096
    for (i = 0; i < 16; i++)
        second++; // second = second + numthreads*16 -> second = 4*(3072 + 16) -> second = 12352
    }

    printf("omp_num_threads fuera de los parallel (DEBERIA SER 1) : %d\n", omp_get_num_threads());

    omp_set_num_threads(6);
    #pragma omp parallel
    {
    printf("Thread %d finished the execution of foo\n", omp_get_thread_num()); //6 threads
    printf("omp_num_threads en el tercer parallel  (DEBERIA SER 6): %d\n", omp_get_num_threads());
    }

    printf("omp_num_threads fuera de los parallel (DEBERIA SER 1) : %d\n", omp_get_num_threads());

    return(x);
}

int main(int argc, char *argv[]) {
    printf("first = %d, second = %d, x = %d\n", first, second, foo());
}
