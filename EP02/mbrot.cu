#include <thrust/complex.h>
#include <stdio.h> 
#include <stdlib.h>
#include <png.h>
#include <sys/time.h>

#define M 200

#define DIE(...) { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
}

float points[4];  // c0_real, c0_image, c1_real, c1_image
int w, h;
char cpu_gpu[5];  // do the calculations in cpu or gpu  
int num_threads_arg;
char saida[256];

void application_error(char message[256]) {
  printf ("%s\n", message);
  exit(0);
}

void write_png_file(float *buffer) {
  int i, j;
  png_bytep row = NULL;
  png_structp png_ptr;
  png_infop info_ptr;

  /* Open file for writing (binary mode) */
  FILE *fp = fopen(saida, "wb");
  if (!fp) {
    DIE("write_png_file(): Could not open file ... ");
  }

  /* Initialize write structure */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    DIE("write_png_file(): png_ptr is NULL ... ");
  }
  
  /* Initialize info structure */
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    DIE("write_png_file(): info_ptr is NULL ... ");
  }

  /* Exception handling */
  if (setjmp(png_jmpbuf(png_ptr))) {
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    if (fp != NULL) fclose(fp);
    if (row != NULL) free(row);
    DIE("write_png_file(): libpng error ... ");
  }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, w, h,
			8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);

  row = (png_bytep) malloc(3 * w * sizeof(png_byte));

  /* write image data */
  for (i = 0; i < h; ++i) {
    for (j = 0; j < w; ++j) {
      row[3 * j] = buffer[i * w + j];
      row[3 * j + 1] = 0;
      row[3 * j + 2] =  0;
    }
    png_write_row(png_ptr, row);
  }

  /* End write */
  png_write_end(png_ptr, NULL);

  if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row); 
}

float test_complex_number(int k) {
  int j = k / w;
  int i = k % w;

  float dx = (points[2] - points[0]) / w ;
  float dy = (points[3] - points[1]) / h;
  float image = points[1] + dy * i;
  float real = points[0] + dx * j;

  thrust::complex<float> c = thrust::complex<float>(real, image);
  thrust::complex<float> z = thrust::complex<float>(0, 0);
  int it = 0;

  while (it < M && norm(z) < 2.0) {
    z = z * z + c;
    ++it;
  }

  if (it < M) {
    return (255 * it) / M;
  } 
  
  return 0;
}

float *cpu_create_image() {
  float *buffer = (float *) malloc(sizeof(float) * w * h);
  int k;

  if (buffer == NULL) {
    DIE("createImage(): could create buffer ... ");
  }

  #pragma omp parallel for num_threads(num_threads_arg) 
  for (k = 0; k < w * h; ++k)
    buffer[k] = test_complex_number(k);
 
  return buffer;
}

void cpu_solve() {
  float *buffer;
  buffer = cpu_create_image();
  write_png_file(buffer);
  free(buffer);
}

__global__
void gpu_create_image(float *buffer, int w, int h, float *points) {
  const int globalIndex = blockDim.x*blockIdx.x + threadIdx.x;
  
  if (globalIndex < w * h) {
    int j = globalIndex / w;
    int i = globalIndex % w;

    float dx = (points[2] - points[0]) / w ;
    float dy = (points[3] - points[1]) / h;
    float image = points[1] + dy * i;
    float real = points[0] + dx * j;

    thrust::complex<float> c = thrust::complex<float>(real, image);
    thrust::complex<float> z = thrust::complex<float>(0, 0);

    int it = 0;

    while (it < M && norm(z) < 2.0) {
      z = z * z + c;
      ++it;
    }

    if (it < M) buffer[globalIndex] = (255 * it) / M;
    else buffer[globalIndex] = 0;
  } 
}


void gpu_solve() {
  const int THREADS_PER_BLOCK = num_threads_arg;
  const int NUM_BLOCKS = (w * h + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
  float *buffer_cpu;
  float *buffer_gpu, *points_gpu;
 
  buffer_cpu = (float *) malloc(sizeof(float) * w * h);

  if (buffer_cpu == NULL) {
    DIE("gpu_solve(): buffer_cpu is NULL ...");
  }

  cudaMalloc(&buffer_gpu, w * h * sizeof(float));
  cudaMalloc(&points_gpu, 4 * sizeof(float));

  cudaMemcpy(points_gpu, points, 4 * sizeof(float), cudaMemcpyHostToDevice);

  gpu_create_image<<<NUM_BLOCKS, THREADS_PER_BLOCK>>>(buffer_gpu, w, h, points_gpu);
  cudaDeviceSynchronize();

  cudaMemcpy(buffer_cpu, buffer_gpu, w * h * sizeof(float), cudaMemcpyDeviceToHost);

  write_png_file(buffer_cpu);

  cudaFree(buffer_gpu);
  cudaFree(points);
}

int main(int argc, char **argv) {

  struct timeval start, end;

  // Read Arguments
  if (argc < 9 
  || sscanf(argv[1], "%f", &points[0])        != 1 
  || sscanf(argv[2], "%f", &points[1])        != 1
  || sscanf(argv[3], "%f", &points[2])        != 1 
  || sscanf(argv[4], "%f", &points[3])        != 1 
  || sscanf(argv[5], "%d", &w)                != 1 
  || sscanf(argv[6], "%d", &h)                != 1
  || sscanf(argv[7], "%s", cpu_gpu)           != 1 
  || sscanf(argv[8], "%d", &num_threads_arg)  != 1
  || sscanf(argv[9], "%s", saida)             != 1) {
    DIE("Invalid Arguments ...\n");
  }

  gettimeofday(&start, NULL);
  if (cpu_gpu[0] == 'c') cpu_solve();
  else gpu_solve();
  gettimeofday(&end, NULL);

  double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 10000000.0;
  printf("%.4lf\n", elapsed_time);

  return 0;
}