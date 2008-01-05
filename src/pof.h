#ifndef _POF_H_
#define _POF_H_

#include <stdint.h>
#include "common.h"

#define VERSION_FREESPACE 1000

typedef struct {
    struct MODEL_PATH path;
    int version;
    int nr_textures;
    char **textures;
    int nr_submodels;
    MODEL *submodels;
    int *submodel_parents;
    int nr_lod;
    int *lod;
    int nr_lights;
    VECTOR_3D *lights;
} POF_MODEL;

int pofdecode (uint8_t *buf, int len, POF_MODEL *model);
int pof_unknown (uint8_t *buf, int len, POF_MODEL *model);
int pof_textures (uint8_t *buf, int len, POF_MODEL *model);
int pof_header (uint8_t *buf, int len, POF_MODEL *model);
int pof_object (uint8_t *buf, int len, POF_MODEL *model);
int pof_turret (uint8_t *buf, int len, POF_MODEL *model);

int pof_fusemodel (MODEL *out, POF_MODEL *in);

int pof_freesubmodel (MODEL *model);
int pof_freemodel (POF_MODEL *model);


#endif /* _POF_H_ */
