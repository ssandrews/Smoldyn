@echo off
set FROMDIR=%~dp0%
set DESTDIR=%PROGRAMFILES%\Smoldyn\

echo Installing from '%FROMDIR%' to '%DESTDIR%'
echo Installing Smoldyn
if not exist "%DESTDIR%" mkdir "%DESTDIR%"
copy "%FROMDIR%smoldyn.exe" "%DESTDIR%"

echo Installing BioNetGen
if not exist "%DESTDIR%\BioNetGen\" mkdir "%DESTDIR%\BioNetGen\"
xcopy "%FROMDIR%BioNetGen" "%DESTDIR%\BioNetGen" /s /e /q

echo Installing SmolCrowd
copy "%FROMDIR%SmolCrowd.exe" "%DESTDIR%"

echo Installing wrl2smol
copy "%FROMDIR%wrl2smol.exe" "%DESTDIR%"

echo Installing Libsmoldyn
if not exist "%DESTDIR%include" mkdir "%DESTDIR%include"
if not exist "%DESTDIR%lib" mkdir "%DESTDIR%lib"
copy "%FROMDIR%include\libsmoldyn.h" "%DESTDIR%include"
copy "%FROMDIR%include\smoldyn.h" "%DESTDIR%include"
copy "%FROMDIR%include\smoldynconfigure.h" "%DESTDIR%include"
copy "%FROMDIR%lib\libsmoldyn_static.lib" "%DESTDIR%lib"
copy "%FROMDIR%lib\libsmoldyn_shared.dll" "%DESTDIR%lib"

echo Installing Python bindings
where pip /q
if %ERRORLEVEL% == 0 (pip install "%FROMDIR%bin\smoldyn-2.67-cp39-cp39-win_amd64.whl") else (echo WARNING: Not installing Python bindings because pip cannot be found)

echo Adding Smoldyn directory to local and system path
echo.%PATH% > "%DESTDIR%PATH_old.txt"
echo.%PATH% | find /I "Smoldyn">Nul && (echo.already in PATH) || (setx /m PATH "%PATH%;%DESTDIR%;")
set PATH=%PATH%;%DESTDIR%;

echo Installation complete


