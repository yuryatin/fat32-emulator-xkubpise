#ifndef UTILS_H_xkubpise
#define UTILS_H_xkubpise

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#define FAT32ERRORS_SIZE 4 * 1024

#define SECTOR_SIZE 512
#define SECTORS_PER_CLUSTER 1
#define CLUSTER_SIZE (SECTOR_SIZE * SECTORS_PER_CLUSTER)
#define TOTAL_N_SECTORS (2 * 1024 * 20) // 20 MB
#define TOTAL_SIZE (SECTOR_SIZE * TOTAL_N_SECTORS) // 20 MB
#define N_RESERVED_SECTORS 32
#define N_FATS 2
#define FAT_SIZE (TOTAL_N_SECTORS * 4 / SECTOR_SIZE) // 320
#define FIRST_DATA_SECTOR (N_FATS * FAT_SIZE + N_RESERVED_SECTORS)
#define N_DATA_SECTORS (TOTAL_N_SECTORS - FIRST_DATA_SECTOR)
#define N_CLUSTERS (N_DATA_SECTORS / SECTORS_PER_CLUSTER)

typedef enum { False, True } boolean;

char safeChar(unsigned char c);
boolean checkFileStatus(const char * filename);

#endif