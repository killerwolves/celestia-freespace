#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "galaxy.h"

GAL_RDD galactic_centre = {
    266.41683333333333333330, -29.00780555555555555555, 24800
};

GAL_ROT rot_eqt2ecl = {
    {0, 0, 0},
    {1, 0, 0},
    -23.4392911
};

GAL_ROT rot_ecl2gal = {
    {0, 0, 0},
    {0.866295,0.491168,0.091031},
    -176
};

static MATRIX3D eqt2gal;
static MATRIX3D gal2ecl;
static long double pi;

MATRIX3D rot2mat (GAL_ROT rot){
    MATRIX3D ret;
    int i;
    long double ca, sa, nca;
    long double ax, ay, az, al;
    long double ox, oy, oz;
 
    sa = sinl(rot.angle * pi / 180);
    ca = cosl(rot.angle * pi / 180);
    nca = 1 - ca;

    ax = rot.axis.x;
    ay = rot.axis.y;
    az = rot.axis.z;
    ox = rot.origin.x;
    oy = rot.origin.y;
    oz = rot.origin.z;
    al = sqrtl (ax * ax + ay * ay + az * az);
    ax /= al;
    ay /= al;
    az /= al;
    ret.xx = nca * ax * ax + ca;
    ret.xy = nca * ay * ax - sa * az;
    ret.xz = nca * az * ax + sa * ay;
    ret.xw = ret.xx * ox + ret.xy * oy + ret.xz * oz;
    ret.yx = nca * ax * ay + sa * az;
    ret.yy = nca * ay * ay + ca;
    ret.yz = nca * az * ay - sa * ax;
    ret.yw = ret.yx * ox + ret.yy * oy + ret.yz * oz;
    ret.zx = nca * ax * az - sa * ay;
    ret.zy = nca * ay * az + sa * ax;
    ret.zz = nca * az * az + ca;
    ret.zw = ret.zx * ox + ret.zy * oy + ret.zz * oz;
    return ret;
}

MATRIX3D matmul (MATRIX3D b, MATRIX3D a){
    MATRIX3D ret;
    ret.xx = a.xx * b.xx + a.xy * b.yx + a.xz * b.zx;
    ret.xy = a.xx * b.xy + a.xy * b.yy + a.xz * b.zy;
    ret.xz = a.xx * b.xz + a.xy * b.yz + a.xz * b.zz;
    ret.xw = a.xx * b.xw + a.xy * b.yw + a.xz * b.zw + a.xw;
    ret.yx = a.yx * b.xx + a.yy * b.yx + a.yz * b.zx;
    ret.yy = a.yx * b.xy + a.yy * b.yy + a.yz * b.zy;
    ret.yz = a.yx * b.xz + a.yy * b.yz + a.yz * b.zz;
    ret.yw = a.yx * b.xw + a.yy * b.yw + a.yz * b.zw + a.yw;
    ret.zx = a.zx * b.xx + a.zy * b.yx + a.zz * b.zx;
    ret.zy = a.zx * b.xy + a.zy * b.yy + a.zz * b.zy;
    ret.zz = a.zx * b.xz + a.zy * b.yz + a.zz * b.zz;
    ret.zw = a.zx * b.xw + a.zy * b.yw + a.zz * b.zw + a.zw;
    return ret;
}

GAL_XYZ transform (GAL_XYZ p, MATRIX3D m){
    GAL_XYZ ret;
    ret.x = m.xx * p.x + m.xy * p.y + m.xz * p.z + m.xw;
    ret.y = m.yx * p.x + m.yy * p.y + m.yz * p.z + m.yw;
    ret.z = m.zx * p.x + m.zy * p.y + m.zz * p.z + m.zw;
    return ret;
}

GAL_XYZ rdd2xyz (GAL_RDD p){
    GAL_XYZ ret;
    long double sra, cra, sdec, cdec;
 
    /* Right Ascension is in Degrees */
    sra = sinl (p.ra * pi / 180);
    cra = cosl (p.ra * pi / 180);
    /* Declination is in Degrees */
    sdec = sinl ((180 - p.dec) * pi / 180);
    cdec = cosl ((180 - p.dec) * pi / 180);

    ret.x = -p.dist * cdec * cra;
    ret.z = p.dist * cdec * sra;
    ret.y = p.dist * sdec;
    return ret;
}

GAL_RZD xyz2rzd (GAL_XYZ p){
    GAL_RZD ret;
    long double ra;

    ret.z = -p.y;
    ret.dist = sqrtl (p.x * p.x + p.z * p.z);
    ra = 0;
    if (ret.dist >= 1e-16){
        ra = acosl(p.z / ret.dist);
	if (isnan (ra)){
	    ra = (p.z > 0) ? 0 : pi;
	}
	if (p.x < 0){
	    ra = (pi * 2) - ra;
	}
    }
    ret.ra = ra * 180 / pi;
    return ret;
}

GAL_XYZ rzd2xyz (GAL_RZD p){
    GAL_XYZ ret;
    long double ra;
    
    ret.y = -p.z;
    ra = p.ra * pi / 180;
    ret.x = p.dist * sinl(ra);
    ret.z = p.dist * cosl(ra);
    return ret;
}

GAL_RZD eqtrdd2galrzd (GAL_RDD p){
    GAL_RZD ret;
    GAL_XYZ tmp;
    tmp = rdd2xyz (p);
    tmp = transform (tmp, eqt2gal);
    ret = xyz2rzd (tmp);
    return ret;
}

GAL_XYZ galrzd2eclxyz (GAL_RZD p){
    GAL_XYZ ret;
    ret = rzd2xyz (p);
    ret = transform (ret, gal2ecl);
    return ret;
}

static void setup_eqt2gal (void){
    MATRIX3D e, g, ig;
    GAL_XYZ gc;
    GAL_XYZ tmp;
    GAL_ROT inv_rot;
    /* We want the math processor's idea of PI */
    pi = atanl(1) * 4;
    e = rot2mat (rot_eqt2ecl);
    gc = rdd2xyz (galactic_centre);
    gc = transform (gc, e);
    g = rot2mat (rot_ecl2gal);
    eqt2gal = matmul (e, g);
    inv_rot = rot_ecl2gal;
    inv_rot.angle *= -1;
    gal2ecl = rot2mat (inv_rot);
    gal2ecl.xw += gc.x;
    gal2ecl.yw += gc.y;
    gal2ecl.zw += gc.z;
    gc = transform (gc, g);
    eqt2gal.xw -= gc.x;
    eqt2gal.yw -= gc.y;
    eqt2gal.zw -= gc.z;
    tmp.x = tmp.y = tmp.z = 0;
    tmp = transform (tmp, eqt2gal);
    gc = transform (tmp, gal2ecl);
}

int get_name_f3 (FILE *f, char *name, int len, long double *v1, 
                  long double *v2, long double *v3){
    int c, q, p;
    memset (name, 0, 65);
    p = 0;
    q = 2;
    while (!(isspace(c = fgetc (f)) && q == 0)){
	if (q == 2){
	    if (!isspace(c)){
		q = 0;
	    } else {
		continue;
	    }
	}
	if (c == EOF){
	    return -1;
	}
	if (q == 4){
	    if (c == '\n'){
	        q = 2;
	    }
	    continue;
	}
	if (q != 1 && c == '#'){
	    q = 4;
	    continue;
	}
	if (c == '"'){
	    q ^= 1;
	    continue;
	}
	if (p < len){
	    name[p] = c;
	}
	p++;
    }
    if (fscanf (f, "%llf %llf %llf", 
		v1, v2, v3) != 3){
	fprintf (stderr, "Improperly formatted coordinates\n");
	return -1;
    }
    return 0;
}

void print_eqtrdd2galrzd (FILE *fin, FILE *fout){
    GAL_RDD pin;
    GAL_RZD pout;
    char name[65];
    setup_eqt2gal();
    while (!feof (fin)){
        if (get_name_f3 (fin, name, 64, &pin.ra, 
	                 &pin.dec, &pin.dist)){
	    exit(1);
	}
	pout = eqtrdd2galrzd (pin);
	fprintf (fout, "\"%s\" %20.15Lf %20.15Lf %20.15Lf\n",
	        name, pout.ra, pout.dist, pout.z);
    }
}

void print_galrzd2eclxyz (FILE *fin, FILE *fout){
    GAL_RZD pin;
    GAL_XYZ pout;
    char name[65];
    setup_eqt2gal();
    while (!feof (fin)){
        if (get_name_f3 (fin, name, 64, &pin.ra, 
	                 &pin.dist, &pin.z)){
	    exit(1);
	}
	pout = galrzd2eclxyz (pin);
	fprintf (fout, "\"%s\" %20.15Lf %20.15Lf %20.15Lf\n",
	        name, pout.x, pout.y, pout.z);
    }
}

int main (int argc, char **argv){
    if (strstr(argv[0], "eqt2gal")){
        print_eqtrdd2galrzd (stdin, stdout);
    } else if (strstr(argv[0], "gal2ecl")){
        print_galrzd2eclxyz (stdin, stdout);
    }
    return 0;
}

