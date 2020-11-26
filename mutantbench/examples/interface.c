#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c#9210560
char** strSplit(char* a_str, const char a_delim) {
    char** result = 0;
    size_t count = 0;
    char* tmp = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    while (*tmp) {
        if (a_delim == *tmp) {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;
    result = malloc(sizeof(char*) * count);

    if (result) {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

int MBDetectMutants(char *stringMutantLocations) {
    // TODO: add docstring, also for java file
    char ** mutantLocations = strSplit(stringMutantLocations, ',');
    for (int i = 0; *(mutantLocations + i); i++) {
        printf(*(mutantLocations + i));
    }
    return 0;
}

int main() {
    return MBDetectMutants("test, test2");
}