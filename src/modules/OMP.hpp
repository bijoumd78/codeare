#ifdef _OPENMP
#include <omp.h>
#else
inline int  omp_get_thread_num() { return 0;}
inline int  omp_get_num_threads() { return 1;}
inline void omp_set_num_threads(const int) {};
#endif

