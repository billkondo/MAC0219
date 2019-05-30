NVCC = nvcc
CC = gcc
CCFLAGS_CUDA=-g -G 
CCFLAGS = -Wall  
LDFLAGS = -Xcompiler -fopenmp -lpng

mbrot: mbrot.cu
	$(NVCC) $(CCFLAGS_CUDA) mbrot.cu -o mbrot $(LDFLAGS)

seq: seq.c
		$(CC) $(CCFLAGS) seq.c -o seq -lpng -lm

