#ifndef _CMOD_H_
#define _CMOD_H_

#include <stdint.h>
#include "common.h"

#define CELESTIAVERSION 141

#define TEXTURE_MAIN 1
#define TEXTURE_GLOW 2
#define TEXTURE_SHINE 4

#define TEXTURE_MAX_EXT_LEN 5
#define TEXTURE_MAX_SUFFIX_LEN 10
#define TEXTURE_DIR "../textures/medres"

int texture_exists (char *name);
int cmodprint (MODEL *model);

#endif /* _CMOD_H_ */
