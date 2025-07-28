#include "utils.h"

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "fat32.h"

void trimAndPrintName(const unsigned char * entry) {
    char name[FILE_NAME_MAX_LENGTH + 1 + FILE_EXT_MAX_LENGTH + 1] = {0}; // 8 + 1 + 3 + 1 (dot and null)
    int i;
    for (i = 0; i < 8 && entry[i] != ' '; ++i) {
        name[i] = entry[i];
    }
    if (entry[FILE_NAME_MAX_LENGTH] != ' ') {
        name[i++] = '.';
        for (int j = FILE_NAME_MAX_LENGTH; j < (FILE_NAME_MAX_LENGTH + FILE_EXT_MAX_LENGTH) && entry[j] != ' '; ++j) {
            name[i++] = entry[j];
        }
    }
    name[i] = '\0';
    printf("Name: %s\n", name);
}

void formatShortName(const char * fileName, unsigned char * entryName) {
    memset(entryName, 0x20, 11); // filled with ASCII spaces

    const char * dot = strchr(fileName, '.');
    int nameLen = dot ? (dot - fileName) : strlen(fileName);
    int extLen = dot ? strlen(dot + 1) : 0;

    for (int c = 0; c < nameLen && c < 8; ++c)
        entryName[c] = toupper((unsigned char)fileName[c]);

    for (int c = 0; c < extLen && c < 3; ++c)
        entryName[8 + c] = toupper((unsigned char)dot[1 + c]);
}


// Allowed special characters in short (8.3) names
boolean isValidShortChar(char c) {
    return (boolean) (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
            c == '!' || c == '#'  || c == '$' || c == '%' ||
            c == '&' || c == '\'' || c == '(' || c == ')' ||
            c == '-' || c == '@'  || c == '^' || c == '_' ||
            c == '`' || c == '{'  || c == '}' || c == '~';
}

// We check if the name fits 8.3 and convert to uppercase in-place
boolean isValidShortNameAndUppercaseFile(char * name) {
    size_t len = strlen(name);
    size_t dotIndex = len;
    
    for (size_t i = 0; i < len; ++i) {
        if (name[i] == '.') {
            dotIndex = i;
            break;
        }
    }
    if (dotIndex == 0 || dotIndex > 8) return False;

    // Extension must be <= 3 characters
    size_t extLen = (dotIndex == len) ? 0 : len - dotIndex - 1;
    if (extLen > 3) return False;

    // I validate and convert name part
    for (size_t i = 0; i < dotIndex; ++i) {
        char c = name[i];
        if (islower(c)) c = toupper(c);
        if (!isValidShortChar(c)) return False;
        name[i] = c;
    }

    // I validate and convert extension (if present)
    for (size_t i = dotIndex + 1; i < len; ++i) {
        char c = name[i];
        if (islower(c)) c = toupper(c);
        if (!isValidShortChar(c)) return False;
        name[i] = c;
    }
    return True;
}

boolean isValidShortNameAndUppercaseFolder(char * name) {
    size_t len = strlen(name);
    for (size_t i = 0; i < len; ++i) {
        char c = name[i];
        if (islower(c)) c = toupper(c);
        if (!isValidShortChar(c)) return False;
        name[i] = c;
    }
    return True;
}

success writeSector(uint32_t sector, const void * data) {
    if (fseek(volume, sector * SECTOR_SIZE, SEEK_SET) != 0) return Failure;
    if (fwrite(data, 1, SECTOR_SIZE, volume) != SECTOR_SIZE) return Failure;
    return Success;
}

success writeSectors(uint32_t startSector, const void * data, size_t count) {
    if (fseek(volume, startSector * SECTOR_SIZE, SEEK_SET) != 0) return Failure;
    if (fwrite(data, SECTOR_SIZE, count, volume) != count) return Failure;
    return Success;
}

char safeChar(unsigned char c) {
    return (isprint(c) && c != '\0') ? c : '.';
}

fat32_status_t checkFileStatus(const char * filename) {
    struct stat buffer;
    if (stat(filename, &buffer) != 0) {
        if (errno == ENOENT) {            
            printf("File %s does not seem yet to exist\nCreating new FAT32 volume at %s...\n", filename, filename);
            volume = fopen(filename, "wb");
            if (!volume) {
                perror("Failed to create new FAT32 volume");
                return FAT32_ERROR;
            }

            if (fseek(volume, TOTAL_SIZE - 1, SEEK_SET) != 0) {
                perror("Failed to allocated 20 MB to new FAT32 volume. Closing ...");
                fclose(volume);
                return FAT32_ERROR;
            }
        
            if (fwrite("", 1, 1, volume) != 1) {
                perror("Failed to allocated 20 MB to new FAT32 volume. Closing ...");
                fclose(volume);
                return FAT32_ERROR;
            }
            return FAT32_NOT_FOUND;
        } else {
            printf("File with the path %s can't be accessed\n", filename);
            return FAT32_ERROR;
        }
    }
    if (!S_ISREG(buffer.st_mode)) {
        printf("%s is not a regular file\n", filename);
        return FAT32_ERROR;
    }
    if (buffer.st_size == 0) {
        printf("File %s is empty\nAre you certain that you want to use it as a new volume for FAT32 emulator \033[34mxkubpise\033[0m (Y/n):", filename);
        // Add reading user input and formatting if requested
        return FAT32_ERROR;
    }

    FILE * f = fopen(filename, "rb");
    if (!f) {
        switch (errno) {
            case EACCES:
                printf("No permission to read %s\n", filename);
                return FAT32_ERROR;
            case ENFILE:
                printf("System limit on open files reached\n");
                return FAT32_ERROR;
            case ETXTBSY:
                printf("File is currently busy/executed and cannot be opened\n");
                return FAT32_ERROR;
            case EISDIR:
                printf("%s is a directory, not a file\n", filename);
                return FAT32_ERROR;
            case ENAMETOOLONG:
                printf("Filename too long: %s\n", filename);
                return FAT32_ERROR;
            case ENOSPC:
                printf("No space left on device\n");
                return FAT32_ERROR;
            case EROFS:
                printf("Read-only filesystem error\n");
                return FAT32_ERROR;
            default:
                printf("Other error %d opening %s: %s\n", errno, filename, strerror(errno));
                return FAT32_ERROR;
        }
    }
    fclose(f);
    return FAT32_OK;
}