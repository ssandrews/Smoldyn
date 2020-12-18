#!/bin/bash

echo "Uninstalling Smoldyn"
rm /usr/local/bin/smoldyn
rm -r /usr/local/bin/BioNetGen

echo "Uninstalling SmolCrowd"
rm /usr/local/bin/SmolCrowd

echo "Uninstalling wrl2smol"
rm /usr/local/bin/wrl2smol

echo "Uninstalling Libsmoldyn"
rm /usr/local/include/libsmoldyn.h
rm /usr/local/include/smoldyn.h
rm /usr/local/include/smoldynconfigure.h
rm /usr/local/lib/libsmoldyn_static.a
rm /usr/local/lib/libsmoldyn_shared.dylib

echo "Uninstalling Python bindings"
if command -v pip &> /dev/null; then
	pip uninstall smoldyn
fi

echo "Uninstallation complete"
