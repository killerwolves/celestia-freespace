#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//#include <efence.h>
#include <math.h>
#include "cmod.h"
#include "pof.h"
#include "common.h"

int main (int argc, char **argv){
    uint8_t *buf;
    POF_MODEL model;
    MODEL fused;
    int len;
    int i;
    FILE *f;
    char *ext;

    if (argc == 2 && !strcmp(argv[1], "-")){
        argv[1] = NULL;
    } else if (argc == 1){
        argc = 2;
	argv[1] = NULL;
    }
    for (i=1; i < argc; i++){
        memset (&model, 0, sizeof(model));
	memset (&fused, 0, sizeof(fused));
        model.path.parent = NULL;
        model.path.nr_parents = 0;
        if (argv[i] == NULL){
       	    f = stdin;
            model.path.name = "(stdin)";
	} else {
	    f = fopen (argv[i], "rb");
	    ext = strchr(argv[i],'.');
	    if (ext != NULL){
	        *ext = 0;
	    }
	    model.path.name = argv[i];
	}
	if (f != NULL){
	    fseek (f, 0, SEEK_END);
            len = ftell(f);
            ASSERT_PERROR (len >= 8, "Could not seek in file");
            fseek (f, 0, SEEK_SET);
            buf = malloc(len + 4096);
            ASSERT_PERROR (buf != NULL, "Could not allocate memory for file");
            fread (buf, 1, len, f);
	    if (f != stdin){
	        fclose (f);
	    }
            if (pofdecode (buf, len, &model)){
                continue;
            }
            free (buf);
            for (i=0; i < model.nr_submodels; i++){
                cmodprint (&model.submodels[i]);
            }

	    pof_fusemodel (&fused, &model);

	    cmodprint (&fused);

            pof_freesubmodel (&fused);
	    pof_freemodel (&model);
	}
    }
    return 0;
}

