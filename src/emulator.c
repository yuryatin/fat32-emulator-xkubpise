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
    char newDir[FILE_NAME_MAX_LENGTH+1];
    boolean inDir = False;
    int nInDir = 0;
    int j;
    char lowerCaseName[FILE_NAME_MAX_LENGTH + FILE_EXT_MAX_LENGTH + 2];
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
        } else if (strcmp(input, "ls") == 0) {
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
            scanf("%8s", newDir);
            if (!isValidShortNameAndUppercaseFile(newDir) && strcmp(newDir, ".") && strcmp(newDir, "..")) {
                printf("Impossible folder name: %s\n", newDir);
                continue;
            }
            nInDir = collectNamesInCluster(currentCluster);
            for (j = 0; j < nInDir; ++j) {
                if (strcmp(localFilesAndFolders[j], newDir) == 0) {
                    inDir = True;
                    break;
                }
            }
            if (inDir) {
                uint32_t newCluster = findSubdirectoryCluster(newDir, currentCluster);
                if (newCluster == 0) {
                    printf("No folder %s in this directory\n", newDir);
                    continue;
                }
                currentCluster = newCluster;
            } else {
                printf("Folder %s not found in the current directory\n", newDir);
            }
        } else if (strcmp(input, "mkdir") == 0) {
            inDir = False;
            scanf("%8s", newDir);
            if (!isValidShortNameAndUppercaseFile(newDir)) {
                printf("Invalid folder name: %s\n", newDir);
                continue;
            }
            nInDir = collectNamesInCluster(currentCluster);
            for (int i = 0; i < nInDir; ++i) {
                if (strcmp(localFilesAndFolders[i], newDir) == 0) {
                    printf("Name %s already exists in the folder\n", newDir);
                    inDir = True;
                    break;
                }
            }
            if (inDir) continue;
            uint32_t newCluster = findFreeCluster();
            if (newCluster == 0) {
                printf("No free clusters available to create a new folder\n");
                continue;
            }
            if (createNewFolder(newDir, newCluster, currentCluster) == Failure) {
                printf("Failed to create folder %s\n", newDir);
                continue;
            }
            printf("Folder %s created successfully\n", newDir);
        } else if (strcmp(input, "touch") == 0) {
            char newFile[INPUT_MAX_LENGTH];
            scanf("%12s", newFile);
            //displayFileContent(file, &currentNode);
        } else {
            printf("Unknown command: %s\n", input);
        }
        skipRest();
    }
}