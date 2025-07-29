#include "emulator.h"
#include "fat32.h"
#include "format.h"

static char * username;
static char location[LOCATION_MAX_LENGTH] = "/";
FAT32Node currentNode;
int currentCluster = ROOT_CLUSTER;
extern char localFilesAndFolders[SECTOR_SIZE / FAT_ENTRY_SIZE][FULL_FILE_STRING_SIZE];
extern IsFormatted isFormatted;

static void printPrompt(void) {
    buildPathToRoot(currentCluster, location);
    printf("%s@xkubpise %s> ", username, location);
}

static void notFormattedMessage(void) {
    puts("The volume is pre-initialized but not fully formatted.\nYou can use the emulator to format it now (command \"format\")");
}

void emulate(void) {
    username = getenv("USER");
    struct winsize w;
    uint32_t newCluster;
    char input[INPUT_MAX_LENGTH];
    boolean inDir = False;
    int nInDir = 0;
    char lowerCaseName[FULL_FILE_STRING_SIZE];
    char * argument;
    char * pathArg;
    while(True) {
        argument = NULL;
        printPrompt();
        fgets(input, sizeof(input), stdin);
        argument = strtok(input, " \n");
        if (argument) {
            if (strcmp(argument, "exit") == 0) {
                puts("Emulation shuts down...");
                return;
            } else if (strcmp(argument, "quit") == 0) {
                puts("Emulation shuts down...");
                return;
            } else if (strcmp(argument, "q") == 0) {
                puts("Emulation shuts down...");
                return;
            } else if (strcmp(argument, "format") == 0) {
                if (format() == Failure) {
                    puts("\nFAT32 volume formatting failed\n");
                    continue;
                } else {
                    puts("\nPre-initialized FAT32 volume successfully formatted\n");
                    isFormatted = formatted;
                    currentCluster = ROOT_CLUSTER;
                    strcpy(location, "/");
                    puts("You can now use the emulator with the following commands:\n"
                        "pwd - print current working directory\n"
                        "ls (<directory>) or dir (<directory>) - list files and folders in the current or indicated directory\n"
                        "cd <directory> - change directory to <directory>\n"
                        "mkdir <folder_name> - create a new folder named <folder_name>\n"
                        "touch <file_name> - create a new file named <file_name>\n"
                        "exit, quit, q - exit the emulator");
                }
            } else if (strcmp(argument, "pwd") == 0) {
                if (!isFormatted) { notFormattedMessage(); continue; }
                buildPathToRoot(currentCluster, location);
                printf(" %s\n", location);
            } else if (strcmp(argument, "ls") == 0 || strcmp(argument, "dir") == 0) {
                if (!isFormatted) { notFormattedMessage(); continue; }
                pathArg = strtok(NULL, " \n");
                if (pathArg != NULL) {
                    newCluster = findClusterByFullPath(pathArg, currentCluster);
                    if (newCluster == 0) continue;
                } else newCluster = currentCluster;

                int nInDir = collectNamesInCluster(newCluster);
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
            } else if (strcmp(argument, "cd") == 0) {
                if (!isFormatted) { notFormattedMessage(); continue; }
                pathArg = strtok(NULL, " \n");
                uint32_t newCluster = findClusterByFullPath(pathArg, currentCluster);
                if (newCluster == 0) continue;
                currentCluster = newCluster;
            } else if (strcmp(argument, "mkdir") == 0) {
                if (!isFormatted) { notFormattedMessage(); continue; }
                inDir = False;
                char * newObj = strtok(NULL, " \n");
                if (newObj == NULL) {
                    printf("Usage: mkdir <folder_name>\n");
                    continue;
                }
                newObj[FILE_NAME_MAX_LENGTH] = '\0'; // Ensure null-termination
                if (!isValidShortNameAndUppercaseFile(newObj, itsFolder)) {
                    printf("Invalid folder name: %s\n", newObj);
                    continue;
                }
                nInDir = collectNamesInCluster(currentCluster);
                for (int i = 0; i < nInDir; ++i) {
                    if (strcmp(localFilesAndFolders[i], newObj) == 0) {
                        printf("Name %s already exists in the folder\n", newObj);
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
                if (createNewObject(newObj, newCluster, currentCluster, itsFolder) == Failure) {
                    printf("Failed to create folder %s\n", newObj);
                    continue;
                }
                printf("Folder %s created successfully\n", newObj);
            } else if (strcmp(argument, "touch") == 0) {
                if (!isFormatted) { notFormattedMessage(); continue; }
                inDir = False;
                char * newObj = strtok(NULL, " \n");
                if (newObj == NULL) {
                    printf("Usage: touch <file_name>\n");
                    continue;
                }
                newObj[FILE_AND_EXT_RAW_LENGTH + 1] = '\0';
                if (!isValidShortNameAndUppercaseFile(newObj, itsFile)) {
                    printf("Invalid file name: %s\n", newObj);
                    continue;
                }
                nInDir = collectNamesInCluster(currentCluster);
                for (int i = 0; i < nInDir; ++i) {
                    if (strcmp(localFilesAndFolders[i], newObj) == 0) {
                        printf("Name %s already exists in the folder\n", newObj);
                        inDir = True;
                        break;
                    }
                }
                if (inDir) continue;
                if (createNewObject(newObj, 0, currentCluster, itsFile) == Failure) {
                    printf("Failed to create file %s\n", newObj);
                    continue;
                }
                printf("File %s created successfully\n", newObj);
            } else {
                printf("Unknown command: %s\n", argument);
            }
        }
    }
}