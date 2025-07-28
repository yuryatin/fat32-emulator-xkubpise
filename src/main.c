#include "utils.h"
#include "fat32.h"
#include "format.h"
#include "emulator.h"

const char * fat32 = NULL;
char fat32ReadingErrors[FAT32ERRORS_SIZE];
FILE * volume;
char localFilesAndFolders[SECTOR_SIZE / FAT_ENTRY_SIZE][FULL_FILE_STRING_SIZE];

int main(int argc, char *argv[]) {
    success preFormatResult, formatResult;
    if (argc > 1) fat32 = argv[1];
    else puts("No path to a configuration file was provided. Proceeding with Default parameters.");
    fat32_status_t check = checkFileStatus(fat32);
    switch (check)
    {
    case FAT32_NOT_FOUND:
        preFormatResult = preformat();
        if (preFormatResult == Failure) {
            puts("\nFAT32 volume pre-formatting failed.\n");
            return 1;
        } else {
            formatResult = format();
            if (formatResult == Failure) {
                puts("\nFAT32 volume formatting failed.\n");
                return 1;
            } else {
                puts("\nFAT32 volume pre-formatting and formatting completed successfully.\n");
            }
        }
        break;
    case FAT32_ERROR:
        return 1;
    case FAT32_OK:
        break;
    }
    if (volume) {
        fclose(volume);
        volume = NULL;
    }
    if(!isValidFAT32xkubpise(fat32)) {
        printf("\nFAT32 volume is \033[31mnot valid\033[0m for FAT32 emulator \033[34mxkubpise\033[0m\nThe following \033[31minconsistencies\033[0m have been detected:\n%s", fat32ReadingErrors);
        puts("");
        return 1;
    }
    emulate();
    return 0;
}