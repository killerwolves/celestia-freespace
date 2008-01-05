#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pam.h>
#include <math.h>
#include <string.h>

int main (int argc, char **argv){
    struct pam inpam, outpam;
    tuplen pix, *outpix, **inpix;
    int i, j, x, y;
    int ox, oy;
    double dx, dy;
    double ix, iy;
    double sx, sy;
    double lat, lon;
    double rlat, rlon, roll, max_angle, cos_max_angle;
    double t;
    int p;
    double m;
    FILE *infile;

    /* Zero the pam structures */
    memset (&inpam, 0, sizeof(struct pam));
    memset (&outpam, 0, sizeof(struct pam));
    
    /* Initialize LibNetPBM */
    pnm_init (&argc, argv);

    /* TODO: take command line arguments for rlat, rlon and roll */
    rlat = rlon = roll = 0;

    /* TODO: take command line arguments for max_angle */
    max_angle = 89;

    /* TODO: take command line arguments for width, height, maxval and input file */
    /* Setup the output parameters */
    outpam.file = stdout;
    outpam.width = 4096;
    outpam.height = 2048;
    outpam.maxval = 255;
    outpam.format = PAM_FORMAT;

    /* TODO: take command line arguments for input file */
    infile = stdin;
    
    /* read in the image, exit if this failed */
    inpix = pnm_readpamn (infile, &inpam, PAM_STRUCT_SIZE(tuple_type));
    if (inpix == NULL){
        perror ("Unable to read input image");
	exit(1);
    }
    outpam.len = outpam.size = PAM_STRUCT_SIZE(tuple_type);

    switch (inpam.depth){
        case 1:
	case 2:
	    outpam.depth = 2;
            strncpy (outpam.tuple_type, "GRAYSCALE_ALPHA", PAM_MEMBER_SIZE(tuple_type));
	    break;
	case 3:
	case 4:
	    outpam.depth = 4;
            strncpy (outpam.tuple_type, "RGB_ALPHA", PAM_MEMBER_SIZE(tuple_type));
	    break;
	default:
	    outpam.depth = inpam.depth;
            strncpy (outpam.tuple_type, inpam.tuple_type, PAM_MEMBER_SIZE(tuple_type));
    }

    /* Convert degrees into radians */
    rlat *= M_PI / 180;
    rlon *= M_PI / 180;
    roll *= M_PI / 180;
    max_angle *= M_PI / 180;
    cos_max_angle = cos(max_angle);

    /* Allocate the output array */
    outpix = pnm_allocpamrown (&outpam);
    if (outpix == NULL){
        perror ("Unable to allocate memory for one row of output image");
	pnm_freepamarrayn (inpix, &inpam);
	exit(1);
    }

    pix = malloc (sizeof(*pix) * outpam.depth);
    if (pix == NULL){
        perror ("Unable to allocate memory for a single pixel");
	pnm_freepamrown (outpix);
	pnm_freepamarrayn (inpix, &inpam);
	exit(1);
    }

    pnm_writepaminit (&outpam);

    sx = M_PI * 2 / outpam.width;
    sy = M_PI / outpam.height;

    for (oy = 0; oy < outpam.height; oy++){
        for (ox = 0; ox < outpam.width; ox++){
	    for (p=0; p<outpam.depth; p++){
	        pix[p] = 0;
	    }
	    lat = (M_PI * 0.5) - ((double)oy * sy) - rlat;
	    lon = (M_PI * 0.5) + ((double)ox * sx) - rlon;
	    ix = sin(lon) * cos(lat);
	    iy = -sin(lat);
	    ix = (0.5 * ((ix * cos(roll) + iy * sin(roll)) + 1)) * (double)inpam.width;
	    iy = (0.5 * ((iy * cos(roll) - ix * sin(roll)) + 1)) * (double)inpam.height;
	    m = 0;
	    if (cos(lat) * cos(lon) > cos_max_angle){
		for (i=0; i<2; i++){
		    for (j=0; j<2; j++){
			x = floor(ix + (double)j - 0.5);
			if (x < 0 || x >= inpam.width){
			    continue;
			}
			dx = 1.0 - fabs(j - fmod((ix + 0.5), 1));
			y = floor(iy + (double)i - 0.5);
			if (y < 0 || y >= inpam.height){
			    continue;
			}
			dy = 1.0 - fabs(i - fmod((iy + 0.5), 1));
			if (inpam.depth != outpam.depth){
			    pix[outpam.depth - 1] += dx * dy;
			}

			for (p=0; p<inpam.depth; p++){
			    pix[p] += inpix[y][x][p] * dx * dy;
			}
			m += dx * dy;
		    }
		}
	    }
	    if (m > 1e-6){
	        t = 1 / m;
		for (p=0; p<(outpam.depth - 1); p++){
		    pix[p] *= t;
		}
	    }
            for (p=0; p<outpam.depth; p++){
	        outpix[ox][p] = pix[p];
	    }
	}
	pnm_writepamrown (&outpam, outpix);
    }

    free (pix);
    pnm_freepamrown (outpix);
    pnm_freepamarrayn (inpix, &inpam);
    
    return 0;
}

