#include "globals.hpp"

time_t __logger_time = 0;
Settings settings;

int main() {
    loadSettings();

    free(settings.EXE_PATH);
    free(settings.EXE_ARGS);
    return EXIT_SUCCESS;
}