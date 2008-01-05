#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <efence.h>
#include <math.h>
#include "bsp.h"
#include "common.h"

int bspdecode (uint8_t *buf, int len, MODEL *model){
    int type, size;
    int ret;
    int i, j, v;
    DEBUG_PRINTF ("# BSP Data (%d bytes)\n\n", len);
    while (len >= 8){
        type = getuint32 (buf);
        size = getuint32 (buf + 4);
        buf += 8;
	len -= 8;
	size -= 8;
        if (size < 0){
	    size = 0;
	}
	ret = 0;
        switch (type){
	    case 0: ret = bsp_opeof (buf, size, model); break;;
	    case 1: ret = bsp_defpoints (buf, size, model); break;
	    case 2: ret = bsp_flatpoly (buf, size, model); break;
	    case 3: ret = bsp_tmappoly (buf, size, model); break;
	    case 4: ret = bsp_sortnorm (buf, size, model); break;
	    case 5: ret = bsp_boundbox (buf, size, model); break;
	    default: DEBUG_PRINTF ("Unknown type %d\n");
	}
	buf += size;
	len -= size;
	DEBUG_PRINTF ("\n");
	if (ret != 0){
	    return ret;
	}
    }
    model->mat = malloc (model->nr_poly * sizeof(MATERIAL));
    ASSERT_PERROR (model->mat != NULL, "Unable to allocate memory for materials\n");
    memset (model->mat, 0, model->nr_poly * sizeof(MATERIAL));
    model->nr_mat = 0;

    for (i=0; i < model->nr_polyvtx; i++){
        for (j=0; j < model->nr_mat; j++){
	    if (!memcmp (&model->polyvtx[i].mat, &model->mat[j], sizeof(MATERIAL))){
	        break;
	    }
	}
	if (j == model->nr_mat){
	    model->mat[j] = model->polyvtx[i].mat;
	    model->nr_mat++;
	}
	model->polyvtx[i].matn = j;

    }

    DEBUG_PRINTF ("\n");
    DEBUG_PRINTF ("Materials: %d\n", model->nr_mat);
    DEBUG_PRINTF ("Vertices: %d\n", model->nr_vtx);
    DEBUG_PRINTF ("Polygons: %d\n", model->nr_poly);
    return 0;
}

int bsp_opeof (uint8_t *buf, int len, MODEL *model){
    DEBUG_PRINTF ("# BSP EOF: (%d bytes)\n", len);
    return 0;
}

int bsp_defpoints (uint8_t *buf, int len, MODEL *model){
    int i, j, n, p;
    int nverts;
    int npoints;
    VECTOR_3D pos, norm, rot, tmp;

    nverts = getuint32 (buf);
    npoints = getuint32 (buf + 4);
    p = getuint32 (buf + 8) - 8;

    DEBUG_PRINTF ("# Vertices: %d\n", npoints);

    model->nr_vtx = npoints;
    model->vtx = malloc (npoints * sizeof(VERTEX_3D) * 3);
    ASSERT_PERROR (model->vtx != NULL, "Unable to allocate memory for vertices\n");
    memset (model->vtx, 0, npoints * sizeof(VERTEX_3D) * 3);

    n = 0;
    rot = model->axis;
    if (rot.x == 0 && rot.y == 0 && rot.z == 0){
        rot.y = 1;
    }
    for (i=0; i<nverts; i++){
	pos = getvec3d (buf + p);
	for (j=0; j < buf[12 + i]; j++){
	    norm = getvec3d (buf + p + j * 12 + 12);
	    DEBUG_PRINTF ("#     [ %f %f %f ] [ %f %f %f ]\n",
	                  pos.x, pos.y, pos.z, 
			  norm.x, norm.y, norm.z);
	    model->vtx[n].pos = pos;
	    model->vtx[n].norm = norm;
	    n++;
	}
	p += (1 + j) * 12;
    }
    return 0;
}

int bsp_flatpoly (uint8_t *buf, int len, MODEL *model){
    int nverts;
    int i;
    POLY_VTX v;

    memset (&v, 0, sizeof(v));
    nverts = getuint32 (buf + 28);
    v.mat.diffuse.r = (float)buf[32] / 255;
    v.mat.diffuse.g = (float)buf[33] / 255;
    v.mat.diffuse.b = (float)buf[34] / 255;
    v.mat.diffuse.a = 1;
    //v.mat.emissive = v.mat.diffuse;
    v.mat.opacity = 1;
    model->polyvtx = realloc (model->polyvtx, (model->nr_polyvtx + nverts) * sizeof(POLY_VTX));
    ASSERT_PERROR (model->polyvtx != NULL, "Unable to extend polygon vertex list");
    v.polyn = model->nr_poly;
    v.nverts = nverts;
    model->nr_poly++;
    DEBUG_PRINTF ("# Flat Shaded Polygon: %d points, colour: [ %f %f %f ]\n",
                  nverts, v.mat.diffuse.r, 
		  v.mat.diffuse.g, v.mat.diffuse.b);
    for (i=0; i < nverts; i++){
        v.vtxn = i;
        v.vtx = model->vtx[getuint16 (buf + 36 + 4 * i + 2)];
        DEBUG_PRINTF ("#    [ %f %f %f ] [ %f %f %f ] [ %f %f ]\n",
                      v.vtx.pos.x, v.vtx.pos.y, v.vtx.pos.z,
		      v.vtx.norm.x, v.vtx.norm.y, v.vtx.norm.z,
		      v.vtx.tex.u, v.vtx.tex.v);
	model->polyvtx[model->nr_polyvtx + i] = v;
    }
    model->nr_polyvtx += nverts;

    return 0;
}

int bsp_tmappoly (uint8_t *buf, int len, MODEL *model){
    int nverts;
    int tmap_num;
    int i;
    POLY_VTX v;
    short lastnorm;

    memset (&v, 0, sizeof(v));
    nverts = getuint32 (buf + 28);
    tmap_num = getuint32 (buf + 32);
    DEBUG_PRINTF ("# Texture Mapped Polygon: %d points, texture #%d\n",
                  nverts, tmap_num);
    v.mat.diffuse.r = 1;
    v.mat.diffuse.g = 1;
    v.mat.diffuse.b = 1;
    v.mat.diffuse.a = 1;
    //v.mat.emissive = v.mat.diffuse;
    v.mat.opacity = 1;
    v.mat.texture[0] = (char *)(tmap_num + 1);
    model->polyvtx = realloc (model->polyvtx, (model->nr_polyvtx + nverts) * sizeof(POLY_VTX));
    ASSERT_PERROR (model->polyvtx != NULL, "Unable to extend polygon vertex list");
    v.polyn = model->nr_poly;
    v.nverts = nverts;
    model->nr_poly++;
    for (i=0; i < nverts; i++){
        v.vtxn = i;
        v.vtx = model->vtx[getuint16 (buf + 36 + 12 * i + 2)];
        v.vtx.tex.u = getfloat (buf + 36 + 12 * i + 4);
        v.vtx.tex.v = getfloat (buf + 36 + 12 * i + 8);
        DEBUG_PRINTF ("#    [ %f %f %f ] [ %f %f %f ] [ %f %f ]\n",
                      v.vtx.pos.x, v.vtx.pos.y, v.vtx.pos.z,
		      v.vtx.norm.x, v.vtx.norm.y, v.vtx.norm.z,
		      v.vtx.tex.u, v.vtx.tex.v);
	model->polyvtx[model->nr_polyvtx + i] = v;
    }
    model->nr_polyvtx += nverts;
    return 0;
}

int bsp_sortnorm (uint8_t *buf, int len, MODEL *model){
    DEBUG_PRINTF ("# BSP Sort Normals (%d bytes)\n", len);
    return 0;
}

int bsp_boundbox (uint8_t *buf, int len, MODEL *model){
    DEBUG_PRINTF ("# BSP Bounding Box (%d bytes)\n", len);
    return 0;
}

