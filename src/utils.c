#include "utils.h"

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