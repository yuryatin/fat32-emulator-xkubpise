#ifndef FAT32_H_xkubpise
#define FAT32_H_xkubpise

#include "utils.h"

#define FILE_NAME_MAX_LENGTH 8
#define FILE_EXT_MAX_LENGTH 3
#define FILE_AND_EXT_RAW_LENGTH (FILE_NAME_MAX_LENGTH + FILE_EXT_MAX_LENGTH)
#define FULL_FILE_STRING_SIZE (FILE_AND_EXT_RAW_LENGTH + 2)
#define ENTRY_SIZE 32
#define MAX_PATH 1024

typedef struct {
    char name[FILE_AND_EXT_RAW_LENGTH];
    uint32_t firstCluster;
    boolean isDirectory;
    int parent;
} FAT32Node;

void buildPathToRoot(uint32_t currentCluster, char * outPath);
uint32_t findClusterByFullPath(const char * inputPath, uint32_t currentCluster);
uint32_t findSubdirectoryCluster(const char * name, uint32_t cluster);
uint32_t findFreeCluster();
success createNewObject(const char * objectName, int firstCluster, int parentCluster, boolean isFolder);
int collectNamesInCluster(int cluster);
void initializeDotEntries(uint32_t cluster, uint32_t parentCluster);
int findFirstFreeEntry(int cluster);
void commentOnExtFlags(uint16_t bpb_ExtFlags);
boolean isValidFAT32xkubpise(const char * filename);

#endif