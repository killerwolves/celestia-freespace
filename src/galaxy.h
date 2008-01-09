#ifndef _GALAXY_H_
#define _GALAXY_H_

#include <math.h>
#include <stdint.h>

typedef struct {
    long double ra, dec, dist;
} GAL_RDD;

typedef struct {
    long double x, y, z;
} GAL_XYZ;

typedef struct {
    long double ra, z, dist;
} GAL_RZD;

typedef struct {
    GAL_XYZ origin;
    GAL_XYZ axis;
    long double angle;
} GAL_ROT;

typedef struct {
    long double xx, xy, xz, xw;
    long double yx, yy, yz, yw;
    long double zx, zy, zz, zw;
} MATRIX3D;

MATRIX3D rot2mat (GAL_ROT rot);
MATRIX3D matmul (MATRIX3D b, MATRIX3D a);
GAL_XYZ transform (GAL_XYZ p, MATRIX3D m);
GAL_XYZ rdd2xyz (GAL_RDD p);
GAL_RZD xyz2rzd (GAL_XYZ p);
GAL_XYZ rzd2xyz (GAL_RZD p);
GAL_RZD eqtrdd2galrzd (GAL_RDD p);
GAL_XYZ galrzd2eclxyz (GAL_RZD p);
void print_eqtrdd2galrzd (FILE *fin, FILE *fout);
void print_galrzd2eclxyz (FILE *fin, FILE *fout);

#endif /* _GALAXY_H_ */
