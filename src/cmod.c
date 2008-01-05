#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <efence.h>
#include <math.h>
#include "cmod.h"
#include "common.h"

int cmodprint (MODEL *model){
    int i, j, n;
    struct MODEL_PATH *parent;
    char **names;
    FILE *f;
    int l;
    char *fname;
    VECTOR_3D size;
    VECTOR_3D centre;
    float radius;
    float rotate;
    char *movetype;
    VECTOR_3D min, max;

    /* We only want LOD0 */

    if (model->path.lod != 1){
        return 0;
    }

    /* We don't want destroyed stuff */

    if (strstr (model->path.name, "-destroyed")){
        return 0;
    }
    
    parent = &model->path;
    n = model->path.nr_parents;
    names = malloc ((n + 1) * sizeof(char *));
    ASSERT_PERROR (names != NULL, "Unable to allocate memory for model path");
    memset (names, 0, (n + 1) * sizeof(char *));

    parent = &model->path;
    l = 0;
    for (i = n; i >= 0; i--){
        names[i] = parent->name;
	DEBUG_PRINTF ("# Parent: %s\n", names[i]);
	l += strlen(names[i]) + 1;
        parent = parent->parent;
    }
    DEBUG_PRINTF ("# Length: %d\n", l);
    fname = malloc (l + 6);
    ASSERT_PERROR (fname != NULL, "Unable to allocate memory for model path");
    memset (fname, 0, l + 6);
    strcpy (fname, names[0]);
    for (i=2; i <= n; i++){
        strcat (fname, ".");
	strcat (fname, names[i]);
    }
    strcat (fname, ".cmod");
    free (names);
    f = fopen (fname, "wb");
    ASSERT_PERROR (f != NULL, "Unable to open output file");
    DEBUG_PRINTF ("# Starting model\n");
    fprintf (f, "#celmodel__ascii\n\n");
    fprintf (f, "# name: %s\n", fname);

    min = rotate_axis (model->min, model->axis);
    max = rotate_axis (model->max, model->axis);
    size.x = (model->max.x - model->min.x) / 2;
    size.y = (model->max.y - model->min.y) / 2;
    size.z = (model->max.z - model->min.z) / 2;
    centre.x = model->min.x + size.x;
    centre.y = model->min.y + size.y;
    centre.z = model->min.z + size.z;
    radius = size.x;
    radius = (size.y > radius) ? size.y : radius;
    radius = (size.z > radius) ? size.z : radius;
    switch (model->movetype){
        case -1: movetype = "None"; break;
	case 0: movetype = "Linear"; break;
	case 1: movetype = "Rotate"; break;
	case 2: movetype = "Turret"; break;
	default: movetype = "Unknown";
    }

    DEBUG_PRINTF ("# Geometric:\n"
                "# radius: %f\n"
		"# centre: [ %f %f %f ]\n"
		"# Celestia:\n"
		"# radius: %f\n"
		"# centre: [ %f %f %f ]\n"
		"# \n"
		"# offset: [ %f %f %f ]\n"
		"# axis: [ %f %f %f ]\n"
		"# movement: %s\n"
		"# Properties:\n", 
		model->radius,
		model->centre.z, model->centre.y, model->centre.x,
		radius, 
		centre.z, centre.y, centre.x,
		model->offset.z, model->offset.y, model->offset.x,
		model->axis.z, model->axis.y, model->axis.x,
		movetype);
    rotate = 8.64e24;
#ifdef DEBUGAXIS
    if (model->movetype == 1 || model->movetype == 2){
        rotate = 3.6;
    }
#endif
    DEBUG_PRINTF ("# Reading properties\n");
    for (i=0; i < model->nr_props; i++){
        fprintf (f, "#   %s\n", model->props[i]);
	if (!strncmp (model->props[i], "$rotate=", 8)){
	    rotate = strtod (model->props[i] + 8, NULL);
	}
    }

    DEBUG_PRINTF ("# Calculating orbital and rotational parameters\n");
    {
        float obliq, eqascnod;
	float rotofs = 0;
	float lat, lon, dist;
	VECTOR_3D a, c, o;

	a = model->axis;
	if (a.x == 0 && a.y == 0 && a.z == 0){
	    a.y = 1;
	}

	c = rotate_axis (centre, a);
	o = model->offset;
	dist = sqrt (a.x * a.x + a.y * a.y + a.z * a.z);
	a.x /= dist;
	a.y /= dist;
	a.z /= dist;
	obliq = asin(a.y);
	eqascnod = 0;
	if (isnan(obliq)){
	    obliq = (a.y < 0) ? -M_PI / 2 : M_PI / 2;
	}
	if (cos(obliq) > 0.0001){
	    if (cos(obliq) <= fabs(a.x)){
	        eqascnod = (a.x > 0) ? -M_PI / 2 : M_PI / 2;
	    } else {
	        eqascnod = -asin (a.x / cos(obliq));
	    }
	    if (isnan(eqascnod)){
	        eqascnod = (a.x > 0) ? -M_PI / 2 : M_PI / 2;
	    }
	}
	if (a.z < 0){
	    eqascnod = M_PI - eqascnod;
	}

	obliq = 90 - (obliq * 180 / M_PI);
	eqascnod *= 180 / M_PI;
	
	dist = sqrt (o.x * o.x + o.y * o.y + o.z * o.z);
	lat = lon = 0;
	if (dist > 0){
	    lat = asin (o.y / dist);
	    if (isnan(lat)){
	        lat = (o.y < 0) ? -M_PI / 2 : M_PI / 2;
	    }
	    if (cos(lat) <= fabs(o.x / dist)){
	        lon = (o.z > 0) ? -M_PI / 2 : M_PI / 2;
	    } else {
	        lon = -asin (o.z / dist / cos(lat));
	    }
	    if (isnan(lon)){
	        lon = (o.z > 0) ? -M_PI / 2 : M_PI / 2;
	    }
	}
	if (o.x > 0){
	    lon = M_PI - lon;
	}
	lat *= 180 / M_PI;
	lon *= 180 / M_PI;


	fprintf (f, "#\n"
	            "# Put this in your SSC file:\n"
	            "#\n");
	if (n <= 1){
	    fprintf (f, "##\"%%name%%\" \"%%parent%%\"\n"
			"##{\n"
			"##    Mesh \"%s\"\n"
			"##    MeshCenter [ %f %f %f ]\n"
			"##    Radius %f\n"
			"##    EllipticalOrbit {\n"
			"##        Period %%orbitperiod%%\n"
			"##        SemiMajorAxis %%orbitradius%%\n"
			"##        Eccentricity %%orbiteccentricity%%\n"
			"##        Inclination %%orbitinclination%%\n"
			"##        MeanAnomaly %%orbitmeananomaly%%\n"
			"##        AscendingNode %%orbitascendingnode%%\n"
			"##    }\n"
			"##    RotationPeriod 2.4e21\n"
			"##    Obliquity %%obliquity%%\n"
			"##    EquatorAscendingNode %%equatorascendingnode%%\n"
			"##}\n"
			"#\n",
			fname, -c.z, -c.y, -c.x, radius / 1000);
	} else {
	    fprintf (f, "##\"%s\" \"%%parent%%/%%name%%",
	             names[n]);
	    for (i=2; i < n; i++){
	        fprintf (f, "/%s", names[i]);
	    }
	    fprintf (f, "\"\n"
			"##{\n"
			"##    Mesh \"%s\"\n"
			"##    MeshCenter [ %f %f %f ]\n"
			"##    Radius %f\n"
			"##    EllipticalOrbit {\n"
			"##        Period 1e20\n"
			"##        SemiMajorAxis %f\n"
			"##        Eccentricity 0\n"
			"##        Inclination 90\n"
			"##        MeanAnomaly %f\n"
			"##        AscendingNode %f\n"
			"##    }\n"
			"###   FixedPosition [ %f %f %f ]\n"
			"##    RotationPeriod %g\n"
			"##    RotationOffset %f\n"
			"##    Obliquity %f\n"
			"##    EquatorAscendingNode %f\n"
			"##}\n"
			"#\n",
			fname,
			-c.z, -c.y, -c.x,
			radius / 1000,
			dist / 1000, lat, lon,
			o.z / 1000, 
			o.y / 1000,
			o.x / 1000,
			-rotate / 3600,
			rotofs, obliq, eqascnod);
	}
    }
    free (fname);
    DEBUG_PRINTF ("# Printing materials\n");
    fprintf (f, "\n");
    for (i=0; i<model->nr_mat; i++){
	RGBA c, e, s;
	float o, sp;
	char *em, *sm, *nm, *tm[4];
	DEBUG_PRINTF ("#  Material %d\n", i);
	c = model->mat[i].diffuse;
	e = model->mat[i].emissive;
	s = model->mat[i].specular;
	o = model->mat[i].opacity;
	sp = model->mat[i].specpower;
	em = model->mat[i].emissivemap;
	sm = model->mat[i].specularmap;
	nm = model->mat[i].normalmap;
	tm[0] = model->mat[i].texture[0];
	tm[1] = model->mat[i].texture[1];
	tm[2] = model->mat[i].texture[2];
	tm[3] = model->mat[i].texture[3];
        fprintf (f, "material # index %d\n", i);
	DEBUG_PRINTF ("#   Diffuse: %f %f %f\n", c.r, c.g, c.b);
	fprintf (f, " diffuse %f %f %f\n", c.r, c.g, c.b);
	DEBUG_PRINTF ("#   Opacity: %f\n", o);
	fprintf (f, " opacity %f\n", o);
        if (e.r != 0 || e.g != 0 || e.b != 0){
	    DEBUG_PRINTF ("#   Emissive: %f %f %f\n", e.r, e.g, e.b);
	    fprintf (f, " emissive %f %f %f\n", e.r, e.g, e.b);
	}
	if (s.r != 0 || s.g != 0 || s.b != 0){
	    DEBUG_PRINTF ("#   Specular: %f %f %f\n", s.r, s.g, s.b);
	    fprintf (f, " specular %f %f %f\n", s.r, s.g, s.b);
	}
	if (sp != 0){
	    DEBUG_PRINTF ("#   SpecPower: %f\n", sp);
	    fprintf (f, " specpower %f\n", sp);
	}
	if (tm[0] != NULL){
	    DEBUG_PRINTF ("#   Texture0: %s\n", tm[0]);
	    fprintf (f, " texture0 \"%s.*\"\n", tm[0]);
	    if (em == NULL){
	        DEBUG_PRINTF ("#   EmissiveMap: %s-glow\n", tm[0]);
	        fprintf (f, " EmissiveMap \"%s-glow.*\"\n", tm[0]);
	    }   
	    if (sm == NULL){
	        DEBUG_PRINTF ("#   SpecularMap: %s-shine\n", tm[0]);
	        fprintf (f, " SpecularMap \"%s-shine.*\"\n", tm[0]);
	    }   
	}
	if (tm[1] != NULL){
	    DEBUG_PRINTF ("#   Texture1: %s\n", tm[1]);
	    fprintf (f, " texture1 \"%s.*\"\n", tm[1]);
	}
	if (tm[2] != NULL){
	    DEBUG_PRINTF ("#   Texture2: %s\n", tm[2]);
	    fprintf (f, " texture2 \"%s.*\"\n", tm[2]);
	}
	if (tm[3] != NULL){
	    DEBUG_PRINTF ("#   Texture3: %s\n", tm[3]);
	    fprintf (f, " texture3 \"%s.*\"\n", tm[3]);
	}
	if (em != NULL){
	    DEBUG_PRINTF ("#   EmissiveMap: %s\n", em);
	    fprintf (f, " emissivemap \"%s.*\"\n", em);
	}
	if (sm != NULL){
	    DEBUG_PRINTF ("#   SpecularMap: %s\n", sm);
	    fprintf (f, " specularmap \"%s.*\"\n", sm);
	}
        if (nm != NULL){
	    DEBUG_PRINTF ("#   NormalMap: %s\n", nm);
	    fprintf (f, " normalmap \"%s.*\"\n", nm);
	}
	fprintf (f, "end_material\n\n");
    }
#ifdef DEBUGAXIS
    fprintf (f, "material\n"
                " emissive 1 0 0\n"
		" opacity 1\n"
		"end_material\n\n");
    fprintf (f, "material\n"
                " emissive 0 1 1\n"
		" opacity 1\n"
		"end_material\n\n");
    fprintf (f, "material\n"
                " emissive 0 1 0\n"
		" opacity 1\n"
		"end_material\n\n");
    fprintf (f, "material\n"
                " emissive 1 0 1\n"
		" opacity 1\n"
		"end_material\n\n");
    fprintf (f, "material\n"
                " emissive 0 0 1\n"
		" opacity 1\n"
		"end_material\n\n");
    fprintf (f, "material\n"
                " emissive 1 1 0\n"
		" opacity 1\n"
		"end_material\n\n");
#endif
    DEBUG_PRINTF ("# Printing Meshes\n");
    for (i=0; i < model->nr_mat; i++){
        int j;
	int n;
	POLY_VTX *v;
	VECTOR_3D a;

	a = model->axis;
	if (a.x == 0 && a.y == 0 && a.z == 0){
	    a.y = 1;
	}
	n = 0;
        for (j=0; j < model->nr_polyvtx; j++){
	    if (model->polyvtx[j].matn == i){
	        n++;
	    }
	}
	if (n == 0){
	    continue;
	}
	v = malloc (n * sizeof(POLY_VTX));
	ASSERT_PERROR (v != NULL, "Unable to allocate memory for polygons for mesh");
	memset (v, 0, n * sizeof(POLY_VTX));

        n = 0;

	for (j=0; j < model->nr_polyvtx; j++){
	    if (model->polyvtx[j].matn == i){
	        v[n] = model->polyvtx[j];
		n++;
	    }
	}       

	DEBUG_PRINTF ("#  Starting mesh %d\n", i);

	fprintf (f, "mesh\n"
	            "\n"
	            " vertexdesc\n"
	            "  position f3\n"
	            "  normal f3\n"
	            "  texcoord0 f2\n"
	            " end_vertexdesc\n"
		    "\n"
		    " vertices %d\n", n);
	for (j=0; j < n; j++){
	    VECTOR_3D vtx;
	    VECTOR_3D norm;
	    VECTOR_UV tex;

	    tex = v[j].vtx.tex;
	    vtx = rotate_axis (v[j].vtx.pos, a);
	    norm = rotate_axis (v[j].vtx.norm, a);

	    fprintf (f, "  %f %f %f %f %f %f %f %f\n",
		     vtx.z, vtx.y, vtx.x,
		     norm.z, norm.y, norm.x,
		     tex.u, tex.v);
	}
	DEBUG_PRINTF ("#   Vertices done\n");
	for (j=0; j < n; j+=v[j].nverts){
	    VECTOR_3D norm;
	    float t;
	    int line;
	    int vn;
            line = 0;
	    norm = v[j].vtx.norm;
	    t = sqrt (norm.x * norm.x + norm.y * norm.y + norm.z * norm.z);
	    if (v[j].nverts == 1){
	        line = 0;
	        fprintf (f, "\n points %d 1\n", v[j].matn);
	    } else if (t < 0.001 && v[j].vtx.tex.u == 0 && v[j].vtx.tex.v == 0){
	        line = 1;
	        fprintf (f, "\n linestrip %d %d\n ", 
	                 v[j].matn, v[j].nverts + 1);
	    } else {
	        line = 0;
	        fprintf (f, "\n trifan %d %d\n ", 
	                 v[j].matn, v[j].nverts);
	    }
	    for (vn = v[j].nverts - 1; vn >= 0; vn--){
	        fprintf (f, " %d", j + vn);
	    }
	    if (line == 1){
	        fprintf (f, " %d", j + v[j].nverts - 1);
	    }
	    fprintf (f, "\n");
	}
	DEBUG_PRINTF ("#   Polygons done\n");
	fprintf (f, "\n"
	            "end_mesh\n"
		    "\n");
	free(v);
    }
    DEBUG_PRINTF ("# Meshes done\n");
#ifdef DEBUGAXIS
    fprintf (f, "mesh\n"
                "\n"
		" vertexdesc\n"
		"  position f3\n"
		" end_vertexdesc\n"
		"\n"
		" vertices 7\n"
		"  0 0 0\n"
		"  %f 0 0\n"
		"  %f 0 0\n"
		"  0 %f 0\n"
		"  0 %f 0\n"
		"  0 0 %f\n"
		"  0 0 %f\n"
		"\n"
		" linelist %d 2\n"
		"  0 1\n"
		" linelist %d 2\n"
		"  0 2\n"
		" linelist %d 2\n"
		"  0 3\n"
		" linelist %d 2\n"
		"  0 4\n"
		" linelist %d 2\n"
		"  0 5\n"
		" linelist %d 2\n"
		"  0 6\n"
		"\n"
		"end_mesh\n\n",
		max.z, min.z, max.y, min.y, max.x, min.x,
		model->nr_mat, model->nr_mat + 1, model->nr_mat + 2,
		model->nr_mat + 3, model->nr_mat + 4, model->nr_mat + 5);
#endif
    DEBUG_PRINTF ("# cmod out\n");
    fclose (f);

    return 0;
}

