MODELS := $(patsubst %.pof,%.cmod,$(wildcard *.pof))

.PHONY: all models clean

all: models

models: $(MODELS)

%.cmod: %.pof ../src/pof2cmod
	../src/pof2cmod $<

clean:
	rm -f $(MODELS) $(patsubst %.cmod,%.*cmod,$(MODELS)) $(patsubst %.cmod,%-fused.cmod,$(MODELS))
