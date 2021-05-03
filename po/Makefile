po_files := $(wildcard *.po)
domain   := celestia
pot_file := $(domain).pot

all: po-update

$(pot_file): $(pot_file)-update

$(pot_file)-update:
	PATH=${PATH}:$(shell pwd) make -f Rules-data $@
	PATH=${PATH}:$(shell pwd) make -f Rules-win-resource $@
	PATH=${PATH}:$(shell pwd) make -f Rules-qt $@
	xgettext --keyword=_ --keyword=i18n \
	         --keyword=i18nc:1c,2 --keyword=NC_:1c,2 --keyword=C_:1c,2 \
	         --sort-by-file --qt --add-comments \
	         --default-domain=$(domain) --package-name=celestia --package-version=1.7.0 \
	         --msgid-bugs-address=team@celestia.space \
	         --copyright-holder="Celestia Development Team" \
	         --output=$(pot_file) \
	         --from-code=utf-8 \
	         ../data/data.cpp \
	         $$(find ../src -path ../src/celestia/kde -prune -path ../src/tools -prune -o -name \*.cpp -o -name \*.h -o -name \*.m -o -name \*.mm)

po-update: $(po_files)

%.po: $(pot_file)
	msgmerge --update $@ $(pot_file)

clean:
	make -f Rules-data $@
	make -f Rules-win-resource $@
	make -f Rules-qt $@
