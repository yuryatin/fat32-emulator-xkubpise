#include "utils.h"
#include "fat32.h"
#include "format.h"
#include "emulator.h"

IsFormatted isFormatted = notFormatted;
boolean enforceAbsolutePath = True;
const char * fat32 = NULL;
char fat32ReadingErrors[FAT32ERRORS_SIZE];
FILE * volume;
char localFilesAndFolders[SECTOR_SIZE / FAT_ENTRY_SIZE][FULL_FILE_STRING_SIZE];

int main(int argc, char * argv[]) {
    success preFormatResult;
    if (argc < 2) { 
        puts("No path to (desired) FAT32 volume was provided. Exiting...");
        return 1; 
    }
    if (strcmp(argv[1], "-p") == 0) {
        enforceAbsolutePath = False;
        if (argc > 2) fat32 = argv[2];
        else {
            puts("No path to (desired) FAT32 volume was provided. Exiting...");
            return 1; 
        }
    } else {
        fat32 = argv[1];
        if (argc > 2) {
            if (strcmp(argv[2], "-p") == 0) {
                enforceAbsolutePath = False;
            }
        }
    }
    fat32_status_t check = checkFileStatus(fat32);
    switch (check)
    {
    case FAT32_NOT_FOUND:
        preFormatResult = preformat();
        if (preFormatResult == Failure) {
            puts("\nFAT32 volume pre-formatting failed.\n");
            return 1;
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
    isFormatted = isValidFAT32xkubpise(fat32);
    if(isFormatted <= notFormatted) {
        printf("\nFAT32 volume is \033[31mnot valid\033[0m for FAT32 emulator \033[34mxkubpise\033[0m\nThe following \033[31minconsistencies\033[0m have been detected:\n%s", fat32ReadingErrors);
        puts("");
        if (isFormatted == badSize) return 1;
        else {
            printf("This file is 20 MB in size, which is suitable for a FAT32 emulator \033[34mxkubpise\033[0m volume.\nHowever, it does not appear to be a valid volume for this emulator, or it may be corrupted.\n\nAre you sure you want to convert this file into a FAT32 emulator volume?\n\n\tThis will \033[31mDESTROY\033[0m all data in it!\n\nIf yes, please type:\n\"I understand that the data in this file will be LOST\" (without quotes):\n ");
            char answer[53];
            fgets(answer, sizeof(answer), stdin);
            if (strcmp(answer, "I understand that the data in this file will be LOST") != 0) {
                puts("\nThank you for your wise decision to preserve this file. Exiting...\n");
                return 1;
            } else {
                preFormatResult = preformat();
                if (preFormatResult == Failure) {
                    puts("\nFAT32 volume pre-formatting failed.\n");
                    return 1; 
                }
                skipRest();
            }
        }
    }
    if (isFormatted == formatted) {
        if (!checkFormatting()) {
            isFormatted = notFormatted;
            puts("\nThe volume is pre-initialized but not fully formatted.\nYou can use the emulator to format it now (command \"format\"), or you can format it using another tool.\n\n");
        }
    }
    emulate();
    return 0;
}