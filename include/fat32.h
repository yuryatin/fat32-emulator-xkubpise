#ifndef FAT32_H_xkubpise
#define FAT32_H_xkubpise

#include "utils.h"

#define FILE_NAME_MAX_LENGTH 8
#define FILE_EXT_MAX_LENGTH 3
#define ENTRY_SIZE 32

typedef struct {
    char name[FILE_NAME_MAX_LENGTH + FILE_EXT_MAX_LENGTH];
    uint32_t firstCluster;
    boolean isDirectory;
    int parent;
} FAT32Node;

void initializeDotEntries(uint32_t cluster, uint32_t parentCluster);
int findFirstFreeEntry(int cluster);
void commentOnExtFlags(uint16_t bpb_ExtFlags);
boolean isValidFAT32xkubpise(const char * filename);

#endif