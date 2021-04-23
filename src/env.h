// Type declaration for Internal Control Variables (ICV) structure
typedef struct {
  int nthreads_var; // Contendra el valor del ultimo omp_set_num_threads que se haya hecho (o de la variable de entorno OMP_NUM_THREADS si no se ha hecho ningun set_num_threads)
  // Add other ICVs if needed
  int nthreads_en_este_instante; //Contendra el valor de threads en ese instante (por ejemplo: Valdrá '1' fuera de los parallel. En los parallel donde se haya especificado el numero de threads valdrá ese valor, en los parallels donde NO se haya especificado el numero de threads valdra igual a "nthreads_var").
} miniomp_icv_t;

// Global variable storing the ICV (internal control variables) supported in our implementation
extern miniomp_icv_t miniomp_icv;

// Functions implemented in this module
void parse_env(void);
