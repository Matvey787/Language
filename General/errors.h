#ifndef ERRORS
#define ERRORS

enum error {
    NO_ERRORS = 0,
    SPLIT_ON_STRS_FAIL = 1,
    OPEN_FILE_FAIL = 2,
    ALLOCATE_MEMORY_FAIL = 3,
    NULLPTR_ERROR = 4
};

const int UNDEFINED_VAR  = 999;
const int UNDEFINED_FUNC = 999; 
#endif