UPPERCASE   := $(wildcard *[A-Z]*.*)
JPGTEXTURES := $(patsubst %.pcx,%.jpg,$(wildcard *.pcx))
PNGTEXTURES := $(patsubst %.pcx,%.png,$(wildcard *.pcx))

.PHONY: all textures lowercase clean

all: lowercase textures

lowercase: $(UPPERCASE)
	for i in $^; do mv "$$i" "$$(tr '[A-Z]' '[a-z]' <<<"$$i")"; done

textures: $(JPGTEXTURES) $(PNGTEXTURES)

%.jpg: %.pcx
	if [ ! -f $*.dds ]; then \
	  convert $< -resize '$(shell identify -format "w=l(%w-1)/l(2)+1;h=l(%h-1)/l(2)+1;scale=0;w=2^(w/1);h=2^(h/1);w;h" metaldamagegrid.pcx | bc -l | tr '\n' 'x' | sed 's/x$$//')!' $@; \
	fi

%.png: %.pcx %.jpg
	if pcxtoppm <$< | ppmtoxpm 2>/dev/null | grep -o ' c green"'; then \
	  convert $< -transparent '#00ff00' -resize '$(shell identify -format "w=l(%w-1)/l(2)+1;h=l(%h-1)/l(2)+1;scale=0;w=2^(w/1);h=2^(h/1);w;h" metaldamagegrid.pcx | bc -l | tr '\n' 'x' | sed 's/x$$//')!' $@; \
	  rm -f $*.jpg; \
	fi

clean:
	rm -f $(JPGTEXTURES) 
	rm -f $(PNGTEXTURES)
