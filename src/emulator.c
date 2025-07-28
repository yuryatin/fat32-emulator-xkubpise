#include "emulator.h"
#include "fat32.h"

static char * username;
static char location[LOCATION_MAX_LENGTH] = "/";
FAT32Node currentNode;
int currentCluster = ROOT_CLUSTER;
extern char localFilesAndFolders[SECTOR_SIZE / FAT_ENTRY_SIZE][FULL_FILE_STRING_SIZE];

void printPrompt(void) {
    buildPathToRoot(currentCluster, location);
    printf("%s@xkubpise %s> ", username, location);
}

void emulate(void) {
    username = getenv("USER");
    struct winsize w;
    char input[INPUT_MAX_LENGTH];
    char newDirPath[512];
    char newObj[FULL_FILE_STRING_SIZE];
    boolean inDir = False;
    int nInDir = 0;
    //int j;
    char lowerCaseName[FULL_FILE_STRING_SIZE];
    while(True) {
        printPrompt();
        scanf("%99s", input);
        if (strcmp(input, "exit") == 0) {
            puts("Emulation shuts down...");
            return;
        } else if (strcmp(input, "quit") == 0) {
            puts("Emulation shuts down...");
            return;
        } else if (strcmp(input, "q") == 0) {
            puts("Emulation shuts down...");
            return;
        } else if (strcmp(input, "pwd") == 0) {
            buildPathToRoot(currentCluster, location);
            printf("%s@xkubpise %s> ", username, location);
        } else if (strcmp(input, "ls") == 0 || strcmp(input, "dir") == 0) {
            int nInDir = collectNamesInCluster(currentCluster);
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
                for (int i = 0; i < nInDir; ++i) {
                    toLowerRegister(localFilesAndFolders[i], lowerCaseName);
                    if(i % (w.ws_col / 16) == 0 && i) puts("");
                    printf("%16s", lowerCaseName);
                }
            } else {
                for (int i = 0; i < nInDir; ++i) {
                    toLowerRegister(localFilesAndFolders[i], lowerCaseName);
                    printf("%16s", lowerCaseName);
                }   
            }
            puts("");
        } else if (strcmp(input, "cd") == 0) {
            inDir = False;
            scanf("%511s", newDirPath);
            /*if (!isValidShortNameAndUppercaseFile(newObj, itsFolder) && strcmp(newObj, ".") && strcmp(newObj, "..")) {
                printf("Impossible folder name: \033[31m%s\033[0m\n", newObj);
                skipRest(); continue;
            }
            nInDir = collectNamesInCluster(currentCluster);
            for (j = 0; j < nInDir; ++j) {
                if (strcmp(localFilesAndFolders[j], newObj) == 0) {
                    inDir = True;
                    break;
                }
            }
            if (inDir) { */
            uint32_t newCluster = findClusterByFullPath(newDirPath, currentCluster);
            if (newCluster == 0) { skipRest(); continue; }
            currentCluster = newCluster;
            /*} else {
                printf("Folder %s not found in the current directory\n", newObj);
            }*/
        } else if (strcmp(input, "mkdir") == 0) {
            inDir = False;
            scanf("%8s", newObj);
            if (!isValidShortNameAndUppercaseFile(newObj, itsFolder)) {
                printf("Invalid folder name: %s\n", newObj);
                skipRest(); continue;
            }
            nInDir = collectNamesInCluster(currentCluster);
            for (int i = 0; i < nInDir; ++i) {
                if (strcmp(localFilesAndFolders[i], newObj) == 0) {
                    printf("Name %s already exists in the folder\n", newObj);
                    inDir = True;
                    break;
                }
            }
            if (inDir) { skipRest(); continue; }
            uint32_t newCluster = findFreeCluster();
            if (newCluster == 0) {
                printf("No free clusters available to create a new folder\n");
                skipRest(); continue;
            }
            if (createNewObject(newObj, newCluster, currentCluster, itsFolder) == Failure) {
                printf("Failed to create folder %s\n", newObj);
                skipRest(); continue;
            }
            printf("Folder %s created successfully\n", newObj);
        } else if (strcmp(input, "touch") == 0) {
            inDir = False;
            scanf("%12s", newObj);
            if (!isValidShortNameAndUppercaseFile(newObj, itsFile)) {
                printf("Invalid file name: %s\n", newObj);
                skipRest(); continue;
            }
            nInDir = collectNamesInCluster(currentCluster);
            for (int i = 0; i < nInDir; ++i) {
                if (strcmp(localFilesAndFolders[i], newObj) == 0) {
                    printf("Name %s already exists in the folder\n", newObj);
                    inDir = True;
                    break;
                }
            }
            if (inDir) { skipRest(); continue; }
            if (createNewObject(newObj, 0, currentCluster, itsFile) == Failure) {
                printf("Failed to create file %s\n", newObj);
                skipRest(); continue;
            }
            printf("File %s created successfully\n", newObj);
        } else {
            printf("Unknown command: %s\n", input);
        }
        skipRest();
    }
}