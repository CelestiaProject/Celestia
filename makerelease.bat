cd src
nmake /f winbuild.mak CFG=Release %1
cd ..
copy src\celestia\Release\celestia.exe .

