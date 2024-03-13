#!/bin/bash

echo "Uninstalling Smoldyn"
sudo rm /usr/local/bin/smoldyn
sudo rm -r /usr/local/bin/BioNetGen

echo "Uninstalling SmolCrowd"
sudo rm /usr/local/bin/SmolCrowd

echo "Uninstalling wrl2smol"
sudo rm /usr/local/bin/wrl2smol

echo "Uninstalling Libsmoldyn"
sudo rm /usr/local/include/libsmoldyn.h
sudo rm /usr/local/include/smoldyn.h
sudo rm /usr/local/include/smoldynconfigure.h
sudo rm /usr/local/lib/libsmoldyn_static.a
sudo rm /usr/local/lib/libsmoldyn_shared.dylib

echo "Uninstalling Python bindings"
if command -v pip &> /dev/null; then
	pip uninstall smoldyn
fi

echo "Uninstallation complete"
