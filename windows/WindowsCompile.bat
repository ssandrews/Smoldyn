echo "Compiling Smoldyn files"
g++ -Wall -Ilib Smoldyn\smolbng.c -c
g++ -Wall -Ilib Smoldyn\smolboxes.c -c
g++ -Wall -D FREEGLUT_STATIC -Ilib -I"C:\Program Files (x86)\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0\mingw32\include" -ISmoldyn Smoldyn\smolcmd.c -c
g++ -Wall -Ilib Smoldyn\smolcomparts.c -c
g++ -Wall -D FREEGLUT_STATIC -Ilib -I"C:\Program Files (x86)\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0\mingw32\include" -ISmoldyn Smoldyn\smoldyn.c -c
g++ -Wall -Ilib Smoldyn\smolfilament.c -c
g++ -Wall -D FREEGLUT_STATIC -Ilib -I"C:\Program Files (x86)\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0\mingw32\include" -ISmoldyn Smoldyn\smolgraphics.c -c
g++ -Wall -Ilib Smoldyn\smollattice.c -c
g++ -Wall -Ilib Smoldyn\smolmolec.c -c
g++ -Wall -Ilib Smoldyn\smolport.c -c
g++ -Wall -Ilib Smoldyn\smolreact.c -c
g++ -Wall -Ilib Smoldyn\smolrule.c -c
g++ -Wall -D FREEGLUT_STATIC -Ilib -I"C:\Program Files (x86)\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0\mingw32\include" -ISmoldyn Smoldyn\smolsim.c -c
g++ -Wall -Ilib Smoldyn\smolsurface.c -c
g++ -Wall -Ilib Smoldyn\smolwall.c -c

echo "Compiling library files"
g++ -Wall -Ilib lib/Geometry.c -c
g++ -Wall -Ilib lib/List.c -c
g++ -Wall -Ilib lib/math2.c -c
g++ -Wall -D FREEGLUT_STATIC -Ilib -I"C:\Program Files (x86)\mingw-w64\i686-8.1.0-posix-dwarf-rt_v6-rev0\mingw32\include" -ISmoldyn lib/opengl2.c -c
g++ -Wall -Ilib lib/parse.c -c
g++ -Wall -Ilib lib/queue.c -c
g++ -Wall -Ilib lib/random2.c -c
g++ -Wall -Ilib lib/Rn.c -c
g++ -Wall -Ilib lib/RnSort.c -c
g++ -Wall -Ilib lib/rxn2Dparam.c -c
g++ -Wall -Ilib lib/rxnparam.c -c
g++ -Wall -Ilib lib/SFMT/SFMT.c -c
g++ -Wall -Ilib -ISmoldyn lib/SimCommand.c -c
g++ -Wall -Ilib lib/Sphere.c -c
g++ -Wall -Ilib lib/string2.c -c
g++ -Wall -Ilib lib/SurfaceParam.c -c
g++ -Wall -Ilib lib/Zn.c -c

echo "Linking Smoldyn"
g++ -static smolbng.o smolboxes.o smolcmd.o smolcomparts.o smoldyn.o smolfilament.o smolgraphics.o smollattice.o smolmolec.o smolport.o smolreact.o smolrule.o smolsim.o smolsurface.o smolwall.o Geometry.o List.o math2.o opengl2.o parse.o queue.o random2.o Rn.o RnSort.o rxn2Dparam.o rxnparam.o SFMT.o SimCommand.o Sphere.o string2.o SurfaceParam.o Zn.o -o smoldyn.exe -lfreeglut_static -lopengl32 -lgdi32 -lwinmm -lglu32

echo "Making libsmoldyn"
ar rcs libsmoldyn_static.lib smolbng.o smolboxes.o smolcmd.o smolcomparts.o smoldyn.o smolfilament.o smolgraphics.o smollattice.o smolmolec.o smolport.o smolreact.o smolrule.o smolsim.o smolsurface.o smolwall.o Geometry.o List.o math2.o opengl2.o parse.o queue.o random2.o Rn.o RnSort.o rxn2Dparam.o rxnparam.o SFMT.o SimCommand.o Sphere.o string2.o SurfaceParam.o Zn.o
g++ -shared -o libsmoldyn_shared.dll smolbng.o smolboxes.o smolcmd.o smolcomparts.o smoldyn.o smolfilament.o smolgraphics.o smollattice.o smolmolec.o smolport.o smolreact.o smolrule.o smolsim.o smolsurface.o smolwall.o Geometry.o List.o math2.o opengl2.o parse.o queue.o random2.o Rn.o RnSort.o rxn2Dparam.o rxnparam.o SFMT.o SimCommand.o Sphere.o string2.o SurfaceParam.o Zn.o -lfreeglut_static -lopengl32 -lgdi32 -lwinmm -lglu32

echo "Compiling SmolCrowd"
g++ -Wall -Ilib Smoldyn\SmolCrowd.c -c

echo "Linking SmolCrowd"
g++ -static SmolCrowd.o Geometry.o math2.o random2.o Rn.o RnSort.o SFMT.o -o SmolCrowd.exe

echo "Compiling wrl2smol"
g++ -Wall -Ilib Smoldyn\wrl2smol.c -c

echo "Linking wrl2smol"
g++ -static wrl2smol.o -o wrl2smol.exe
