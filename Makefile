SRC_FILES  := src/*.[ch]
MODELS     := $(patsubst %.pof,%.cmod,$(wildcard models/*.pof)) $(wildcard models/*.cmod)

.PHONY : all clean models textures programs shipyard

all: programs

programs:
	$(MAKE) -C src all

clean:
	rm -f shipyard.ssc
	$(MAKE) -C textures/medres clean
	$(MAKE) -C models clean
	$(MAKE) -C src clean

models: programs
	$(MAKE) -C models

textures: programs
	$(MAKE) -C textures/medres

shipyard: models textures shipyard.ssc

shipyard.ssc: $(MODELS)
	./shipyard.sh
