#include <complex.h>
#include <stdio.h> 
#include <stdlib.h>

#include <png.h>

#define M 100

float c0_real, c0_image;
float c1_real, c1_image;
int w, h;
char cpu_gpu[5];
int num_threads;
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
    application_error("write_png_file(): Could not open file ... ");
  }

  /* Initialize write structure */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    application_error("write_png_file(): png_ptr is NULL ... ");
  }
  
  /* Initialize info structure */
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    application_error("write_png_file(): info_ptr is NULL ... ");
  }

  /* Exception handling */
  if (setjmp(png_jmpbuf(png_ptr))) {
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    if (fp != NULL) fclose(fp);
    if (row != NULL) free(row);
    application_error("write_png_file(): libpng error ... ");
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

float test_complex_number(float real, float image) {
  float complex c = real + image * I;
  float complex z = 0 + 0 * I;
  int it = 0;

  while (it < M && cabs(z) < 2.0) {
    z = z * z + c;
    ++it;
  }

  if (it < M) {
    return (255 * it) / M;
  } 
  
  return 0;
}

float *createImage() {
  float *buffer = (float *) malloc(sizeof(float) * w * h);

  int i, j;
  float real, image;
  float dx = (c1_real - c0_real) / w ;
  float dy = (c1_image - c0_image) / h;

  if (buffer == NULL) {
    application_error("createImage(): could create buffer ... ");
  }

  
  for (i = 0; i < h; ++i) {
    image = c0_image + dy * i;
    for (j = 0; j < w; ++j) {
      real = c0_real + dx * j;
      buffer[i * w + j] = test_complex_number(real, image);
    }
  }

  return buffer;
}

int main(int argc, char **argv) {

  /* Arguments */
  if (argc < 9 
  || sscanf(argv[1], "%f", &c0_real)      != 1 
  || sscanf(argv[2], "%f", &c0_image)     != 1
  || sscanf(argv[3], "%f", &c1_real)      != 1 
  || sscanf(argv[4], "%f", &c1_image)     != 1 
  || sscanf(argv[5], "%d", &w)            != 1 
  || sscanf(argv[6], "%d", &h)            != 1
  || sscanf(argv[7], "%s", cpu_gpu)       != 1 
  || sscanf(argv[8], "%d", &num_threads)  != 1
  || sscanf(argv[9], "%s", saida)         != 1) {
    application_error("Invalid Arguments ...\n");
  }

  float *buffer;
  buffer = createImage();
  write_png_file(buffer);
  free(buffer);

  return 0;
}