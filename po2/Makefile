po_files := $(wildcard *.po)
domain   := celestia_constellations
pot_file := $(domain).pot

all: po-update

$(pot_file): $(pot_file)-update

$(pot_file)-update:
	PATH=${PATH}:$(shell pwd) make -f Rules-data $@
	xgettext --keyword=_ --keyword=i18n \
	         --sort-by-file --qt --add-comments \
	         --default-domain=$(domain) --package-name=celestia --package-version=1.7.0 \
	         --msgid-bugs-address=team@celestia.space \
	         --copyright-holder="Celestia Development Team" \
	         --output=$(pot_file) \
	         --from-code=utf-8 \
	         ../data/constellations.cpp

po-update: $(po_files)

%.po: $(pot_file)
	msgmerge --update $@ $(pot_file)

clean:
	make -f Rules-data $@
