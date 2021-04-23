
// Type declaration for single work sharing descriptor
typedef struct {
    int counter;
    int num_threads;
} miniomp_single_t; // IGNORARLA. NO SE LE ACABA DANDO USO.

// Declaration of global variable for single work descriptor
extern miniomp_single_t miniomp_single;

// Functions implemented in this module
bool GOMP_single_start (void);
