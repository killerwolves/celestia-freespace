#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <efence.h>
#include <math.h>
#include "pof.h"
#include "bsp.h"
#include "common.h"

struct POF_FCC_HANDLER {
    uint8_t *fcc;
    int (*handler)(uint8_t *buf, int len, POF_MODEL *model);
} pof_fcc_handler[] = {
    { "TXTR", pof_textures },
    { "OHDR", pof_header },
    { "HDR2", pof_header },
    { "SOBJ", pof_object },
    { "OBJ2", pof_object },
    { "TMIS", pof_turret },
    { "TGUN", pof_turret },
    { NULL, pof_unknown }
};

int pofdecode (uint8_t *buf, int len, POF_MODEL *model){
    uint32_t ftype;
    uint32_t htype;
    uint32_t hlen;
    off_t offset;
    int i;
    ftype = getuint32 (buf);
    ASSERT_PRINTF (ftype == getuint32 ("PSPO"), "Not a POF model\n");
    model->version = getuint32 (buf + 4);
    DEBUG_PRINTF ("# POF version %d\n", model->version);
    for (offset = 8; 
         (htype = getuint32 (buf + offset)) != 0 &&
	 (hlen = getuint32 (buf + offset + 4)) >= 4 &&
	 offset < len;
	 offset += hlen + 8){
        for (i=0; 
	     pof_fcc_handler[i].fcc != NULL &&
	     getuint32 (pof_fcc_handler[i].fcc) != htype;
	     i++);
	if (pof_fcc_handler[i].handler (buf + offset + 8, hlen, model)){
	    return -1;
	}
    }

    return 0;
}

int pof_textures (uint8_t *buf, int len, POF_MODEL *model){
    int i, j, l, p, lt, n;
    char **tmp;
    /* Number of textures */
    n = getuint32 (buf);
    buf += 4;
    len -= 4;
    DEBUG_PRINTF ("# %d Textures:\n", n);

    /* Allocate and zero string pointer list */
    tmp = malloc (n * sizeof(char *));
    ASSERT_PERROR (tmp != NULL, "Unable to allocate memory for texture name list\n");
    memset (tmp, 0, n * sizeof(char *));

    /* Get the texture names */
    for (i = 0; i < n; i++){
           
        l = getuint32 (buf);
	buf += 4;
	len -= 4;
	tmp[i] = malloc (l + 1);
	ASSERT_PERROR (tmp[i] != NULL, "Unable to allocate memory for texture name\n");
	memset (tmp[i], 0, l + 1);
        strncpy (tmp[i], buf, l);
	/* strtolower */
	for (j=0; tmp[i][j] != 0; j++){
	    tmp[i][j] = tolower (tmp[i][j]);
	}

	DEBUG_PRINTF ("# %10d: %s\n", i, tmp[i]);
	buf += l;
	len -= l;
    }
    DEBUG_PRINTF ("# End of Textures\n\n");
    model->nr_textures = n;
    model->textures = tmp;
    return 0;
}

int pof_header (uint8_t *buf, int len, POF_MODEL *model){
    int flags;
    float radius;
    int nr_submodels;
    int nr_debris;
    int i;

    if (getuint32 (buf - 8) == getuint32 ("OHDR")){
        model->nr_submodels = getuint32 (buf);
        radius = getfloat (buf + 4);
	buf += 8;
	len -= 8;
	if (model->version >= VERSION_FREESPACE){
	    flags = getuint32 (buf);
	    buf += 4;
	    len -= 4;
	}
    } else {
        radius = getfloat (buf);
        flags = getuint32 (buf + 4);
        model->nr_submodels = getuint32 (buf + 8) + 1;
        buf += 12;
        len -= 12;
    }
    DEBUG_PRINTF ("# radius: %f\n", radius);
    DEBUG_PRINTF ("# %d submodels\n", model->nr_submodels);

    /* Allocate and zero memory for the submodels */
    model->submodels = malloc(model->nr_submodels * sizeof(MODEL));
    ASSERT_PERROR (model->submodels != 0, "Unable to allocate memory for models");
    memset (model->submodels, 0, model->nr_submodels * sizeof(MODEL));

    /* We don't need the Bounding box */
    buf += 24;
    len -= 24;

    /* Get the Levels of Detail */
    model->nr_lod = getuint32 (buf);
    DEBUG_PRINTF ("# Number of Levels of Detail: %d\n", model->nr_lod);
    model->lod = malloc(sizeof(int) * model->nr_lod);
    ASSERT_PERROR (model->lod != 0, "Unable to allocate memory for LOD list");
    buf += 4;
    len -= 4;
    for (i=0; i < model->nr_lod; i++){
        model->lod[i] = getuint32 (buf);
	model->submodels[model->lod[i]].path.lod = i + 1;
	buf += 4;
	len -= 4;
    }

    /* We don't want the debris */
    nr_debris = getuint32 (buf);
    DEBUG_PRINTF ("# Pieces of debris: %d\n", nr_debris);
    buf += (nr_debris + 1) * 4;

    /* Mass and its effects */
    if (model->version < 1903){
        /* No mass, centre of mass, an no inertia vectors */
	DEBUG_PRINTF ("# No mass\n");
    } else if (model->version < 2009){
        float mass;
        float masscor;
	VECTOR_3D com;
	VECTOR_3D iv[3];
        /* Volume-based mass */
	mass = pow(getfloat(buf), 2/3) * 4.65;
	masscor = mass / getfloat(buf);
	com = getvec3d (buf + 4);
	iv[0] = getvec3d (buf + 16);
	iv[1] = getvec3d (buf + 28);
	iv[2] = getvec3d (buf + 40);

	DEBUG_PRINTF ("# Mass: %f\n"
	              "# Centre of Mass: [ %f %f %f ]\n"
	              "# Right/Left Inertial Vector: [ %f %f %f ]\n"
	              "# Up/Down Inertial Vector: [ %f %f %f ]\n"
		      "# Fore/Aft Inertial Vector: [ %f %f %f ]\n",
		      mass,
		      com.x, com.y, com.z,
		      iv[0].x * masscor, iv[0].y * masscor, iv[0].z * masscor,
		      iv[1].x * masscor, iv[1].y * masscor, iv[1].z * masscor,
		      iv[2].x * masscor, iv[2].y * masscor, iv[2].z * masscor);
	buf += 52;
	len -= 52;
    } else {
        float mass;
	VECTOR_3D com;
	VECTOR_3D iv[3];
        /* Area-based mass */
	mass = getfloat(buf);
	com = getvec3d (buf + 4);
	iv[0] = getvec3d (buf + 16);
	iv[1] = getvec3d (buf + 28);
	iv[2] = getvec3d (buf + 40);

	DEBUG_PRINTF ("# Mass: %f\n"
	              "# Centre of Mass: [ %f %f %f ]\n"
	              "# Right/Left Inertial Vector: [ %f %f %f ]\n"
	              "# Up/Down Inertial Vector: [ %f %f %f ]\n"
		      "# Fore/Aft Inertial Vector: [ %f %f %f ]\n",
		      mass,
		      com.x, com.y, com.z,
		      iv[0].x, iv[0].y, iv[0].z,
		      iv[1].x, iv[1].y, iv[1].z,
		      iv[2].x, iv[2].y, iv[2].z);
	buf += 52;
	len -= 52;
    }

    /* We don't want the Cross-section */
    if (model->version >= 2014){
        int nr_xs;
        nr_xs = getuint32 (buf);
	DEBUG_PRINTF ("# Number of cross-sections: %d\n", nr_xs);
	if (nr_xs <= 0){
	    nr_xs = 0;
	}
	buf += (nr_xs * 2 + 1) * 4;
    }

    /* Look at all the pretty lights */
    if (model->version >= 2007){
        int n;
	int type;
	MODEL *m;
	POLY_VTX v;
	DEBUG_PRINTF ("# Setting up lights around model\n");
	memset (&v, 0, sizeof(v));
	n = model->nr_submodels - 1;
	m = &model->submodels[n];
        m->nr_polyvtx = getuint32 (buf);
	m->nr_poly = m->nr_polyvtx;
	m->min.x = m->min.y = m->min.z = (1.0 / 0.0);
	m->max.x = m->max.y = m->max.z = -(1.0 / 0.0);
	if (m->nr_poly > 0 && model->nr_lod >= 1){
            m->path.name = malloc (strlen("lights")+1);
	    ASSERT_PERROR (m->path.name != NULL, "Unable to allocate memory for name `lights'");
	    strcpy (m->path.name, "lights");
	    m->path.parent = &model->submodels[model->lod[0]].path;
	    m->path.nr_parents = 2;
	    m->path.lod = 1;
	    DEBUG_PRINTF ("# %d Lights:\n", m->nr_poly);
	    buf += 4;
	    
	    m->polyvtx = malloc(m->nr_poly * sizeof(POLY_VTX));
	    ASSERT_PERROR (m->polyvtx != NULL, "Unable to allocate memory for lights");
	    memset (m->polyvtx, 0, m->nr_poly * sizeof(POLY_VTX));

            m->mat = malloc(sizeof(MATERIAL));
	    ASSERT_PERROR (m->mat != NULL, "Unable to allocate memory for light material");
	    memset (m->mat, 0, sizeof(MATERIAL));
	    m->nr_mat = 1;

	    v.mat.emissive.r = 1;
	    v.mat.emissive.g = 1;
	    v.mat.emissive.b = 1;
	    v.mat.emissive.a = 1;
	    v.mat.opacity = 1;
	    m->mat[0] = v.mat;
	    v.matn = 0;
	    v.nverts = 1;
	    v.vtxn = 0;

	    for (i=0; i<m->nr_poly; i++){
		v.polyn = i;
		v.vtx.pos = getvec3d (buf);
		type = getuint32 (buf + 12);
		if (v.vtx.pos.x > m->max.x){
		    m->max.x = v.vtx.pos.x;
		}
		if (v.vtx.pos.y > m->max.y){
		    m->max.y = v.vtx.pos.y;
		}
		if (v.vtx.pos.z > m->max.z){
		    m->max.z = v.vtx.pos.z;
		}
		if (v.vtx.pos.x < m->min.x){
		    m->min.x = v.vtx.pos.x;
		}
		if (v.vtx.pos.y < m->min.y){
		    m->min.y = v.vtx.pos.y;
		}
		if (v.vtx.pos.z < m->min.z){
		    m->min.z = v.vtx.pos.z;
		}
		DEBUG_PRINTF ("# %10d: [ %f %f %f ] type %d\n",
			      i, v.vtx.pos.x, v.vtx.pos.y, v.vtx.pos.z,
			      type);
		m->polyvtx[i] = v;
		buf += 16;
	    }
	    DEBUG_PRINTF ("# End of Lights\n");
	}
    }
    DEBUG_PRINTF ("\n");
    return 0;
}

int pof_object (uint8_t *buf, int len, POF_MODEL *model){
    uint32_t n;
    uint32_t l;
    int i;
    int m;
    int axis;
    int movetype;
    int nchunks;
    int bsp_data_size;
    int parent;
    char *name;

    /* Submodel number */
    n = getuint32 (buf);
    buf += 4;
    len -= 4;
    ASSERT_PRINTF (n < model->nr_submodels, "Submodel number out of range\n");
    DEBUG_PRINTF ("# Submodel %d\n", n);
    if (getuint32 (buf - 8) == getuint32 ("SOBJ")){
        /* Parent submodel */
        parent = getuint32 (buf);
	buf += 4;
	len -= 4;
	DEBUG_PRINTF ("# Parent: %d\n", parent);

	/* Descent had the normal and point for clip planes. We don't need these */
	if (model->version < VERSION_FREESPACE){
	    buf += 24;
	    len -= 24;
	}

	/* Offset of model */
	model->submodels[n].offset = getvec3d (buf);
	buf += 12;
	len -= 12;
	DEBUG_PRINTF ("# Offset: [ %f %f %f ]\n", 
	              model->submodels[n].offset.x, model->submodels[n].offset.y, model->submodels[n].offset.z);

	/* Radius of submodel */
	model->submodels[n].radius = getfloat (buf);
	buf += 4;
	len -= 4;
	DEBUG_PRINTF ("Radius: %f\n", model->submodels[n].radius);

	/* TODO: Pointers into IDTA block */
        if (model->version < VERSION_FREESPACE){
            buf += 4;
	    len -= 4;
	}
    } else {
	/* Radius of submodel */
	model->submodels[n].radius = getfloat (buf);
	buf += 4;
	len -= 4;
	DEBUG_PRINTF ("Radius: %f\n", model->submodels[n].radius);

        /* Parent submodel */
        parent = getuint32 (buf);
	buf += 4;
	len -= 4;
	DEBUG_PRINTF ("# Parent: %d\n", parent);

	/* Offset of model */
	model->submodels[n].offset = getvec3d (buf);
	buf += 12;
	len -= 12;
	DEBUG_PRINTF ("# Offset: [ %f %f %f ]\n", 
	              model->submodels[n].offset.x, model->submodels[n].offset.y, model->submodels[n].offset.z);
    }

    /* Point to the parent object */
    if (parent < 0 || parent >= model->nr_submodels){
        model->submodels[n].path.parent = &model->path;
    } else {
        model->submodels[n].path.parent = &model->submodels[parent].path;
    }
    model->submodels[n].path.nr_parents = model->submodels[n].path.parent->nr_parents + 1;
    if (model->submodels[n].path.lod == 0){
        model->submodels[n].path.lod = model->submodels[n].path.parent->lod;
    }
    
    /* Geometric centre */
    model->submodels[n].centre = getvec3d (buf);
    buf += 12;
    len -= 12;
    DEBUG_PRINTF ("# Geometric Centre: [ %f %f %f ]\n", 
                  model->submodels[n].centre.x, model->submodels[n].centre.y, model->submodels[n].centre.z);
    
    /* bounding box */
    model->submodels[n].min = getvec3d (buf);
    model->submodels[n].max = getvec3d (buf + 12);
    buf += 24;
    len -= 24;

    /* Submodel name */
    l = getuint32 (buf);
    if (l != 0){
        model->submodels[n].path.name = malloc (l + 1);
        ASSERT_PERROR (model->submodels[n].path.name != NULL, "Unable to allocate memory for submodel name");
        memcpy (model->submodels[n].path.name, buf + 4, l);
        model->submodels[n].path.name[l] = 0;
    } else {
        model->submodels[n].path.name = malloc (16);
        ASSERT_PERROR (model->submodels[n].path.name != NULL, "Unable to allocate memory for submodel name");
	snprintf (model->submodels[n].path.name, 15, "_%d_", n);
    }

    DEBUG_PRINTF ("# Name: %s\n", model->submodels[n].path.name);
    buf += l + 4;
    len -= l + 4;

    /* Properties */
    l = getuint32 (buf);
    buf += 4;
    len -= 4;
    {
        char *tmp;
	char **tmpptr;
	tmp = malloc (l + 1);
        ASSERT_PERROR (tmp != NULL, "Unable to allocate memory for submodel properties");
        memcpy (tmp, buf, l);
	buf += l;
	len -= l;
        tmp[l] = 0;
	m = 0;
	for (i=0; i <= l; i++){
	    if (tmp[i] == '\r' || tmp[i] == '\n'){
	        tmp[i] = '\0';
	    }
	    if (tmp[i] == '\0' && tmp[i-1] != '\0'){
	        m++;
	    }
	}
	
	tmpptr = malloc ((m + 1) * sizeof(char *));
	ASSERT_PERROR (tmpptr != NULL, "Unable to allocate memory for submodel properties list");
	memset (tmpptr, 0, (m + 1) * sizeof(char *));

	tmpptr[0] = tmp;
	m = 1;
	for (i=1; i < l; i++){
	    if (tmp[i-1] == '\0' && tmp[i] != '\0'){
	        tmpptr[m] = tmp + i;
		m++;
	    }
	}
	model->submodels[n].props = tmpptr;
	model->submodels[n].nr_props = m;
    }

    /* Ignore the movement type, but keep the movement axis */
    movetype = getuint32 (buf);
    axis = getuint32 (buf + 4);
    buf += 8;
    len -= 8;
    switch (movetype){
        case -1: DEBUG_PRINTF ("# Movement Type: None\n"); break;
	case 0:  DEBUG_PRINTF ("# Movement Type: Linear\n"); break;
	case 1:  DEBUG_PRINTF ("# Movement Type: Rotating\n"); break;
	case 2:  DEBUG_PRINTF ("# Movement Type: Turret\n");
    }
    switch (axis){
	case 0: model->submodels[n].axis.x = 1; break;
	case 1: model->submodels[n].axis.z = 1; break;
	case 2: model->submodels[n].axis.y = 1;
    }
    model->submodels[n].movetype = movetype;
    DEBUG_PRINTF ("# Movement Axis: [ %f %f %f ]\n",
                  model->submodels[n].axis.x, model->submodels[n].axis.y, model->submodels[n].axis.z);
    
    /* nchunks */
    nchunks = getuint32 (buf);
    buf += 4;
    len -= 4;
    ASSERT_PRINTF (nchunks == 0, "Cannot handle chunked models\n");

    /* BSP data */
    bsp_data_size = getuint32 (buf);
    buf += 4;
    len -= 4;
    ASSERT_PRINTF (bsp_data_size <= len, "BSP Data end past end of block\n");

    DEBUG_PRINTF ("\n");
    if (bspdecode (buf, bsp_data_size, &model->submodels[n])){
        return -1;
    }

    /* Fill in the texture names */
    for (i=0; i<model->submodels[n].nr_mat; i++){
        m = (int)model->submodels[n].mat[i].texture[0];
	if (m != 0 && m <= model->nr_textures){
	    model->submodels[n].mat[i].texture[0] = model->textures[m-1];
	    if (!strcmp (model->textures[m-1], "invisible")){
	        model->submodels[n].mat[i].opacity = 0;
	    }
	}
    }

    return 0;
}

int pof_turret (uint8_t *buf, int len, POF_MODEL *model){
    int nr_banks;
    int nr_slots;
    int n;
    int i;
    MODEL *sm;

    /* Number of turrets */
    nr_banks = getuint32 (buf);
    buf += 4;
    len -= 4;

    for (i=0; i<nr_banks; i++){
        /* Subobject Number */
        n = getuint32 (buf);
	buf += 8;
	len -= 8;
	ASSERT_PRINTF (n >= 0 && n < model->nr_submodels, "Turret Submodel out of range\n");
	sm = &model->submodels[n];
	sm->axis = getvec3d (buf);
	nr_slots = getuint32 (buf + 12);
	buf += 16 + nr_slots * 12;
	len -= 16 + nr_slots * 12;
	if (sm->movetype == 1){
	    sm->movetype = 2;
	}
    }
    return 0;
}

int pof_unknown (uint8_t *buf, int len, POF_MODEL *model){
    int i, j;
    uint8_t c;
    DEBUG_PRINTF ("# hdr = \"%c%c%c%c\", len = %d\n", 
            buf[-8], buf[-7], buf[-6], buf[-5], len);
    /*
    for (i = 0; i < len; i += 16){
        DEBUG_PRINTF ("# %08X  ", i);
        for (j = 0; j < 16; j++){
	    if ((i + j) >= len){
	        DEBUG_PRINTF ("   ");
	    } else {
                DEBUG_PRINTF ("%02X ", (int)buf[i + j]);
	    }
	}
	DEBUG_PRINTF (" ");
	for (j = 0; j < 16; j++){
	    if ((i + j) >= len){
	        DEBUG_PRINTF (" ");
	    } else {
	        c = buf[i + j];
		if (c < ' ' || c >= 127){
		    c = '.';
		}
		DEBUG_PRINTF ("%c", c);
	    }
	}
	DEBUG_PRINTF ("\n");
    }
    */
    return 0;
}

int pof_fusemodel (MODEL *out, POF_MODEL *in){
    int i;
    int j;
    int p;
    MODEL *parent;
    int o, n;
    POLY_VTX *vin, *vout;

    vout = NULL;
    o = 0;
    out->max.x = out->max.y = out->max.z = -(1.0/0.0);
    out->min.x = out->min.y = out->min.z = (1.0/0.0);

    DEBUG_PRINTF ("# Fusing submodels into one model\n");
    for (i=0; i < in->nr_submodels; i++){
        /* We only want LOD 0, and we don't want destroyed subsystems */
        if (in->submodels[i].path.lod != 1 || strstr (in->submodels[i].path.name, "-destroyed")){
	    continue;
	}
	n = in->submodels[i].nr_polyvtx;
	vin = in->submodels[i].polyvtx;
        vout = realloc (vout, (o + n) * sizeof(POLY_VTX));
	ASSERT_PERROR (vout != NULL, "Unable to allocate memory for fused model");
	memcpy (vout + o, vin, n * sizeof(POLY_VTX));
	DEBUG_PRINTF ("#  Setting offset for submodel %d (%d polygons)", i, n);
	parent = &in->submodels[i];
	for (p = 0; p < in->submodels[i].path.nr_parents; p++){
	    for (j=0; j < n; j++){
		if ((j & 63) == 0){
		    DEBUG_PRINTF ("\n#   ");
		}
		DEBUG_PRINTF (".");
		vout[o + j].vtx.pos.x += parent->offset.x;
		vout[o + j].vtx.pos.y += parent->offset.y;
		vout[o + j].vtx.pos.z += parent->offset.z;
	    }
	    parent = (MODEL *)parent->path.parent;
	}
        DEBUG_PRINTF ("\n#      OK\n");
	o += n;
    }
    out->polyvtx = vout;
    out->nr_polyvtx = o;
    DEBUG_PRINTF ("#     DONE\n# Counting polygons  ");
    j = -1;
    for (i=0; i < out->nr_polyvtx; i++){
	if (vout[i].vtx.pos.x > out->max.x){
	    out->max.x = vout[i].vtx.pos.x;
	}
	if (vout[i].vtx.pos.y > out->max.y){
	    out->max.y = vout[i].vtx.pos.y;
	}
	if (vout[i].vtx.pos.z > out->max.z){
	    out->max.z = vout[i].vtx.pos.z;
	}
	if (vout[i].vtx.pos.x < out->min.x){
	    out->min.x = vout[i].vtx.pos.x;
	}
	if (vout[i].vtx.pos.y < out->min.y){
	    out->min.y = vout[i].vtx.pos.y;
	}
	if (vout[i].vtx.pos.z < out->min.z){
	    out->min.z = vout[i].vtx.pos.z;
	}
        if (out->polyvtx[i].vtxn == 0){
	    j++;
	}
	out->polyvtx[n].polyn = j;
    }
    out->nr_poly = j + 1;
    DEBUG_PRINTF (" OK\n# Enumerating materials  ");

    out->mat = malloc (out->nr_poly * sizeof(MATERIAL));
    ASSERT_PERROR (out->mat != NULL, "Unable to allocate memory for materials\n");
    memset (out->mat, 0, out->nr_poly * sizeof(MATERIAL));
    out->nr_mat = 0;

    for (i=0; i < out->nr_polyvtx; i++){
        for (j=0; j < out->nr_mat; j++){
	    if (!memcmp (&out->polyvtx[i].mat, &out->mat[j], sizeof(MATERIAL))){
	        break;
	    }
	}
	if (j == out->nr_mat){
	    out->mat[j] = out->polyvtx[i].mat;
	    out->nr_mat++;
	}
	out->polyvtx[i].matn = j;

    }
    DEBUG_PRINTF (" OK\n# Filling in textures  ");
    for (i=0; i<out->nr_mat; i++){
        n = (int)out->mat[i].texture[0];
	if (n != 0 && n <= in->nr_textures){
	    out->mat[i].texture[0] = in->textures[n-1];
	    if (!strcmp (in->textures[n-1], "invisible")){
	        out->mat[i].opacity = 0;
	    }
	}
    }
    DEBUG_PRINTF (" OK\n# Setting path  ");
    out->path.name = malloc(strlen(in->path.name) + strlen("-fused") + 1);
    ASSERT_PERROR (out->path.name != NULL, "Unable to allocate memory for fused model name");
    strcpy (out->path.name, in->path.name);
    strcat (out->path.name, "-fused");
    out->path.parent = NULL;
    out->path.nr_parents = 0;
    out->path.lod = 1;
    DEBUG_PRINTF (" DONE\n");
    return 0;
}

int pof_freesubmodel (MODEL *model){
    int i;
    free (model->polyvtx);
    model->polyvtx = NULL;
    model->nr_polyvtx = 0;
    free (model->vtx);
    model->vtx = NULL;
    free (model->mat);
    model->mat = NULL;
    free (model->path.name);
    model->path.name = NULL;
    if (model->props != NULL){
        free (model->props[0]);
	free (model->props);
	model->props = NULL;
    }
    model->path.parent = NULL;
    return 0;
}

int pof_freemodel (POF_MODEL *model){
    int i;
    for (i=0; i < model->nr_submodels; i++){
        pof_freesubmodel (&model->submodels[i]);
    }
    free (model->submodels);
    model->submodels = NULL;
    model->nr_submodels = 0;
    for (i=0; i < model->nr_textures; i++){
        free (model->textures[i]);
	model->textures[i] = NULL;
    }
    free (model->textures);
    model->textures = NULL;
    model->nr_textures = 0;
    free (model->lod);
    model->lod = NULL;
    free (model->lights);
    model->lights = NULL;
    return 0;
}

