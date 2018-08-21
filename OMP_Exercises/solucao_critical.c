#include <omp.h>
#include <stdio.h>

static long num_steps = 100000;
double step;
#define NUM_THREADS 2
int main ()
{
  double pi = 0.0;
  int nthreads;
  step = 1.0/(double) num_steps;
  omp_set_num_threads(NUM_THREADS);
  #pragma omp parallel default(none) shared(pi, nthreads) firstprivate(step, num_steps)
  {
    double x, sum = 0.0;
    int i, id,nthrds; 
    id = omp_get_thread_num();
    nthrds = omp_get_num_threads();
    if (id == 0) nthreads = nthrds;
    #pragma omp barrier
    for (i=id; i < num_steps; i+=nthreads){
      x = (i+0.5)*step;
      sum += 4.0/(1.0+x*x);
    }
    #pragma omp critical
      pi += sum * step;
    }
    printf("Pi %f\n", pi);
  return 0;
}


