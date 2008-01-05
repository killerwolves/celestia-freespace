#ifndef _BSP_H_
#define _BSP_H_

#include <stdint.h>
#include "common.h"

int bspdecode (uint8_t *buf, int len, MODEL *model);
int bsp_opeof (uint8_t *buf, int len, MODEL *model);
int bsp_defpoints (uint8_t *buf, int len, MODEL *model);
int bsp_flatpoly (uint8_t *buf, int len, MODEL *model);
int bsp_tmappoly (uint8_t *buf, int len, MODEL *model);
int bsp_sortnorm (uint8_t *buf, int len, MODEL *model);
int bsp_boundbox (uint8_t *buf, int len, MODEL *model);

#endif /* _BSP_H_ */
