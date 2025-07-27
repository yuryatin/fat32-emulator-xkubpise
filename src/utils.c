#include "utils.h"

char safeChar(unsigned char c) {
    return (isprint(c) && c != '\0') ? c : '.';
}

boolean checkFileStatus(const char * filename) {
    struct stat buffer;
    if (stat(filename, &buffer) != 0) {
        if (errno == ENOENT) {
            printf("File %s does not seem to exist\n", filename);
            return False;
        } else {
            printf("File with the path %s can't be accessed\n", filename);
            return False;
        }
    }
    if (!S_ISREG(buffer.st_mode)) {
        printf("%s is not a regular file\n", filename);
        return False;
    }
    if (buffer.st_size == 0) {
        printf("File %s is empty\nAre you certain that you want to use it as a new volume for FAT32 emulator \033[34mxkubpise\033[0m (Y/n):", filename);
        // Add reading user input and formatting if requested
        return False;
    }

    FILE * f = fopen(filename, "rb");
    if (!f) {
        switch (errno) {
            case EACCES:
                printf("No permission to read %s\n", filename);
                return False;
            case ENFILE:
                printf("System limit on open files reached\n");
                return False;
            case ETXTBSY:
                printf("File is currently busy/executed and cannot be opened\n");
                return False;
            case EISDIR:
                printf("%s is a directory, not a file\n", filename);
                return False;
            case ENAMETOOLONG:
                printf("Filename too long: %s\n", filename);
                return False;
            case ENOSPC:
                printf("No space left on device\n");
                return False;
            case EROFS:
                printf("Read-only filesystem error\n");
                return False;
            default:
                printf("Other error %d opening %s: %s\n", errno, filename, strerror(errno));
                return False;
        }
    }
    fclose(f);
    return True;
}