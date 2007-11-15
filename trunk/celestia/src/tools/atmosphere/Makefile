CXX = g++
CXXFLAGS = -O3 -Wall # -msse -msse2
INSTALL = /usr/bin/install

# tools will be installed into
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

all:	scattersim

scattersim:	scattersim.cpp
	$(CXX) $(CXXFLAGS) -I../.. scattersim.cpp -o scattersim -lpng
clean:
	rm -f   *.o
install:
	$(INSTALL) scattersim $(bindir)	
uninstall:
	rm -f $(bindir)/scattersim

