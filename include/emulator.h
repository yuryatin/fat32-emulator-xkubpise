#ifndef EMULATOR_H_xkubpise
#define EMULATOR_H_xkubpise

#include <unistd.h>
#include <sys/ioctl.h>
#include "utils.h"

#define LOCATION_MAX_LENGTH (1024 * 4)
#define INPUT_MAX_LENGTH 512

void emulate(void);

#endif