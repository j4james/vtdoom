// This file is necessary to bundle the PureDOOM framework as C code.
// See https://github.com/Daivuk/PureDOOM for more details.

#define DOOM_IMPLEMENTATION
#define DOOM_IMPLEMENT_MALLOC
#define DOOM_IMPLEMENT_FILE_IO
#define DOOM_IMPLEMENT_GETTIME
#define DOOM_IMPLEMENT_GETENV
#include "PureDOOM.h"
