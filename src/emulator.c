#include "emulator.h"

static char * username;
static char location[LOCATION_MAX_LENGTH] = "/";

void printPrompt(void) {
    printf("%s@xkubpise %s %% ", username, location);
}

void emulate(void) {
    username = getenv("USER");
    char input[INPUT_MAX_LENGTH];
    printPrompt();
    scanf("%99s", input);
    puts("Emulation shuts down...");
}