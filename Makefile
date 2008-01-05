SRC_FILES  := src/*.[ch]

.PHONY : all clean models textures programs

all: programs

programs:
	$(MAKE) -C src all

clean:
	rm -f shipyard.ssc
	$(MAKE) -C textures/medres clean
	$(MAKE) -C models clean
	$(MAKE) -C src clean

models:
	$(MAKE) -C models

textures:
	$(MAKE) -C textures/medres

