# Additional Makefile rule to extract strings from qt's rc and ui files.

Makefile: Rules-qt.mak

celestia.pot-update: ../src/celestia/qt/rc.cpp

../src/celestia/qt/rc.cpp:
	extractrc.pl ../src/celestia/qt/*.rc > ../src/celestia/qt/rc.cpp
	extractrc.pl ../src/celestia/qt/*.ui >> ../src/celestia/qt/rc.cpp

clean: clean-qt

clean-qt:
	rm -f ../src/celestia/qt/rc.cpp
