#include "utils.h"
#include "fat32.h"
#include "format.h"

const char * fat32 = NULL;
char fat32ReadingErrors[FAT32ERRORS_SIZE];
FILE * volume;

int main(int argc, char *argv[]) {
    boolean formatResult = False;
    if (argc > 1) fat32 = argv[1];
    else puts("No path to a configuration file was provided. Proceeding with Default parameters.");
    fat32_status_t check = checkFileStatus(fat32);
    switch (check)
    {
    case FAT32_NOT_FOUND:
        formatResult = preformat();
        if (!formatResult) {
            puts("\nFAT32 volume pre-formatting failed.\n");
            return 1;
        }
        break;
    case FAT32_ERROR:
        return 1;
    case FAT32_OK:
        break;
    }
    if(!isValidFAT32xkubpise(fat32)) {
        printf("\nFAT32 volume is \033[31mnot valid\033[0m for FAT32 emulator \033[34mxkubpise\033[0m\nThe following \033[31minconsistencies\033[0m have been detected:\n%s", fat32ReadingErrors);
        puts("");
        return 1;
    }
    return 0;
}