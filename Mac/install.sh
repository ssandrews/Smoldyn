#!/bin/bash

echo "Installing Smoldyn"
mkdir -p /usr/local/bin
cp bin/smoldyn /usr/local/bin
chmod +x /usr/local/bin/smoldyn
cp -r bin/BioNetGen /usr/local/bin
chmod +x /usr/local/bin/BioNetGen/BNG2.pl
chmod +x /usr/local/bin/BioNetGen/Perl2

echo "Installing SmolCrowd"
cp bin/SmolCrowd /usr/local/bin
chmod +x /usr/local/bin/SmolCrowd

echo "Installing wrl2smol"
cp bin/wrl2smol /usr/local/bin
chmod +x /usr/local/bin/wrl2smol

echo "Installing Libsmoldyn"
mkdir -p /usr/local/include
mkdir -p /usr/local/lib
cp include/libsmoldyn.h /usr/local/include
cp include/smoldyn.h /usr/local/include
cp include/smoldynconfigure.h /usr/local/include
cp lib/libsmoldyn_static.a /usr/local/lib
cp lib/libsmoldyn_shared.dylib /usr/local/lib

echo "Installing Python bindings"
if command -v pip &> /dev/null; then
	pip install bin/smoldyn*.whl
else
 echo "WARNING: Python bindings not installed because pip utility not found"
fi

echo "Installation complete"

if [[ $PATH != */usr/local/bin* ]]
then
	echo "Your environment PATH variable does not include /usr/local/bin so it needs updating."
	echo "Do you want this installer to fix it (y/n)?"
	read yesno
	if [ $yesno = "y" ] ; then
		touch ~/.profile
		echo "export PATH=/usr/local/bin:\$PATH" >> ~/.profile
		source ~/.profile
		echo "Update complete.  Your updated PATH variable is:"
		echo $PATH
	fi
fi
