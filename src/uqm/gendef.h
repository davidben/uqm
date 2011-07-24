#ifndef GENDEF_H
#define GENDEF_H

#include "planets/generate.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

const GenerateFunctions *getGenerateFunctions (BYTE Index);

#if defined(__cplusplus)
}
#endif

#endif  /* GENDEF_H */

