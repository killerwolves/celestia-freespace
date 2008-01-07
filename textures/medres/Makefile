UPPERCASE   := $(wildcard *[A-Z]*.*)
PCXTEXTURES := $(patsubst %.pcx,%.jpg,$(wildcard *.pcx))

.PHONY: all textures lowercase clean

all: lowercase textures

lowercase: $(UPPERCASE)
	for i in $^; do mv "$$i" "$$(tr '[A-Z]' '[a-z]' <<<"$$i")"; done

textures: $(PCXTEXTURES)

%.jpg: %.pcx
	convert $< -resize '$(shell convert -identify $< '' | grep -o '[0-9]*x[0-9]*[+][0-9]*[+][0-9]*' | tr 'x+' ' ' | sed 's@\(.*\) \(.*\) \(.*\) \(.*\)@w=l(\1-1)/l(2)+1;h=l(\2-1)/l(2)+1;scale=0;w=2^(w/1);h=2^(h/1);w;h@' | bc -l | tr '\n' 'x' | sed 's/x$$//')!' $@

clean:
	rm -f $(PCXTEXTURES)
