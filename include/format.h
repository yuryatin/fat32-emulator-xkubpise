#ifndef FORMAT_H_xkubpise
#define FORMAT_H_xkubpise

#include "utils.h"

#define FAT_ENTRY_MASK 0x0FFFFFFF

boolean checkFormatting(void);
success format(void);
success preformat(void);

#endif