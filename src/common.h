#ifndef _COMMON_H_
#define _COMMON_H_

#define _GNU_SOURCE

#include <stdint.h>
#include <math.h>
//#include <efence.h>

typedef struct {
    float r, g, b, a;
} RGBA;

typedef struct {
    float x, y, z, w;
} VECTOR_3D;

typedef struct {
    float u, v;
} VECTOR_UV;

typedef struct {
    VECTOR_3D pos;
    VECTOR_UV tex;
    VECTOR_3D norm;
} VERTEX_3D;

typedef struct {
    char *texture[4];
    char *normalmap;
    char *specularmap;
    char *emissivemap;
    RGBA diffuse;
    RGBA specular;
    RGBA emissive;
    float opacity;
    float specpower;
} MATERIAL;

typedef struct {
    VERTEX_3D vtx;
    MATERIAL mat;
    int polyn;
    int matn;
    int vtxn;
    int nverts;
} POLY_VTX;

struct MODEL_PATH;

struct MODEL_PATH {
    char *name;
    struct MODEL_PATH *parent;
    int nr_parents;
    int lod;
};
    

typedef struct {
    struct MODEL_PATH path;
    int nr_props;
    char **props;
    int nr_mat;
    MATERIAL *mat;
    int nr_vtx;
    VERTEX_3D *vtx;
    int nr_poly;
    int nr_polyvtx;
    POLY_VTX *polyvtx;
    float radius;
    VECTOR_3D offset;
    VECTOR_3D centre;
    VECTOR_3D axis;
    VECTOR_3D min, max;
    int movetype;
} MODEL;



#define ASSERT_PRINTF(i,j...) \
    do { \
        if (!(i)) { \
	    fprintf (stderr,j); \
	    return(1); \
	} \
    }while(0)

#define ASSERT_PERROR(i,j...) do{if(!(i)){fprintf(stderr,j);perror(" ");return(1);}}while(0)

#ifndef DEBUG
#define DEBUG_PRINTF(i...) (i)
#else
#define DEBUG_PRINTF(i...) (printf(i), fflush(stdout))
#endif

static inline uint16_t getuint16 (uint8_t *buf){
    return ((uint16_t)buf[0]) |
           ((uint16_t)buf[1] << 8);
}

static inline uint32_t getuint32 (uint8_t *buf){
    return ((uint32_t) getuint16 (buf)) |
           ((uint32_t) getuint16 (buf+2) << 16);
}

static inline float getfloat (uint8_t *buf){
    uint32_t bin;
    bin = getuint32 (buf);
    return *((float *)(&bin));
}

static inline VECTOR_3D getvec3d (uint8_t *buf){
    VECTOR_3D vec;
    vec.x = getfloat (buf);
    vec.y = getfloat (buf + 4);
    vec.z = getfloat (buf + 8);
    return vec;
}

static inline VECTOR_3D rotate_axis (VECTOR_3D p, VECTOR_3D a){
    VECTOR_3D ret;
    float coslat, sinlat, coslon, sinlon;
    float dist;

    if (a.x == 0 && a.y == 0 && a.z == 0){
        a.y = 1;
    }

    dist = sqrt (a.x * a.x + a.y * a.y + a.z * a.z);
    a.x /= dist;
    a.y /= dist;
    a.z /= dist;

    sinlat = a.y;
    coslat = sqrt (1 - a.y * a.y);
    if (isnan(coslat)){
        coslat = 0;
    }
    if (coslat <= fabs(a.z)){
        sinlon = (a.z < 0) ? -1 : 1;
	coslon = 0;
    } else {
        sinlon = a.z / coslat;
        coslon = sqrt (1 - sinlon * sinlon);
        if (isnan(coslon)){
            coslon = 0;
        }
	if (a.x > 0){
            coslon = -coslon;
        }
    }
    sinlat = -coslat;
    coslat = a.y;

    ret.z = p.z * coslon + p.x * sinlon;
    ret.x = p.x * coslon - p.z * sinlon;
    ret.y = p.y * coslat + ret.x * sinlat;
    ret.x = ret.x * coslat - p.y * sinlat;
    return ret;
}

#endif /* _COMMON_H_ */
