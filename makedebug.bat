cd src
nmake /f winbuild.mak CFG=Debug %1
cd ..
copy src\celestia\Release\celestia.exe .

