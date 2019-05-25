#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <math.h>

int num_processos;
int num_pontos;

int* integration_interval_start;
int* integration_interval_end;

double *pi;

void application_error(char message[256]) {
    printf ("%s\n", message);

    if (integration_interval_start != NULL) free(integration_interval_start);
    if (integration_interval_end != NULL) free(integration_interval_end);
    if (pi != NULL) free(pi);

    exit(0);
}

void* create_shared_memory(size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  return mmap(NULL, size, protection, visibility, -1, 0);
}


// For each process, calculates the start and the end of integration interval.
// The work is being split as evenly as possible between processes.
void prepare_work() {
    int i;
    int threads_with_one_more_work = num_pontos % num_processos;

    integration_interval_start = (int *) malloc(num_processos * sizeof(int));
    integration_interval_end = (int *) malloc(num_processos * sizeof(int));


    for (i = 0; i < num_processos; ++i) {
        int work_size = num_pontos / num_processos;

        if (i < threads_with_one_more_work) {
            ++work_size;
        }

        integration_interval_start[i] = i * work_size;
        integration_interval_end[i] = (i + 1) * work_size;
    }
}

void solve() {
    int i, pid;

    pi = (double *) create_shared_memory(sizeof(double));

    for (i = 0; i < num_processos; ++i) {
        pid = fork();

        if (pid < 0) {
            application_error("Could not create process ...");

            // TODO clear shared memory
        } else if (pid == 0) {
            // Process code 

            double interval_size = 1.0 / num_pontos;
            double acc = 0; // process`s local integration variable
            int k;

            // integrates f(x) = sqrt(1 - x ^ 2) in [ integration_interval_start[i] , integration_interval_end[i] [
            for (k = integration_interval_start[i]; k < integration_interval_end[i]; ++k) {
                double x = (k * interval_size) + interval_size / 2;
                acc += sqrt(1 - (x * x)) * interval_size;
            }

            pi[0] += acc;

            exit(0);
        }
    }

    for (i = 0; i < num_processos; ++i)
        wait(NULL);
}

int main(int argc, char **argv) {

    // Arguments
    if (argc != 3 
    || sscanf(argv[1], "%d", &num_processos)    != 1 
    || sscanf(argv[2], "%d", &num_pontos)       != 1) {
        application_error("Invalid Arguments ...\n");
    }

    prepare_work();
    solve();

    printf("%.12lf\n", pi[0] * 4);

    free(integration_interval_start);
    free(integration_interval_end);
    munmap(pi, sizeof(double));
    return 0;
}