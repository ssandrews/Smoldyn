set FROMDIR=%~dp0%
set DESTDIR=%PROGRAMFILES%\Smoldyn

echo "Installing Smoldyn"
if not exist "%DESTDIR%" mkdir "%DESTDIR%"
copy "%FROMDIR%\smoldyn.exe" "%DESTDIR%"
copy "%FROMDIR%\freeglut.dll" "%DESTDIR%"
xcopy "%FROMDIR%\BioNetGen" "%DESTDIR%" /s /e

echo "Installing SmolCrowd"
copy "%FROMDIR%\SmolCrowd.exe" "%DESTDIR%"

echo "Installing wrl2smol"
copy "%FROMDIR%\wrl2smol.exe" "%DESTDIR%"

echo "Installing Libsmoldyn"
if not exist "%DESTDIR%\include" mkdir "%DESTDIR%\include"
if not exist "%DESTDIR%\lib" mkdir "%DESTDIR%\lib"
copy "%FROMDIR%\include\libsmoldyn.h" "%DESTDIR%\include"
copy "%FROMDIR%\include\smoldyn.h" "%DESTDIR%\include"
copy "%FROMDIR%\include\smoldynconfigure.h" "%DESTDIR%\include"
copy "%FROMDIR%\lib\libsmoldyn_static.a" "%DESTDIR%\lib"
copy "%FROMDIR%\lib\libsmoldyn_shared.dll" "%DESTDIR%\lib"
copy "%FROMDIR%\lib\libsmoldyn_shared.dll.a" "%DESTDIR%\lib"

echo "Installation complete"


