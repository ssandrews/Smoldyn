This README file pertains to the Smoldyn source distribution.

This distribution contains the Smoldyn source code, which should be able to compile and run on Mac, Linux, or Windows.  Building uses the CMake build system.  If you don't have CMake installed, you'll need to get it.

Hopefully, you can build as follows.  Change to the cmake directory (which is empty).  Enter "cmake ..", then "make", and then "sudo make install".  If this works, congratulations!

To build the utility programs wrl2smol and SmolCrowd, cd to the proper cmake directory: source/wrl2smol/cmake or source/SmolCrowd/cmake.  Enter "cmake..", "make", and "sudo make install".

Note that if cmake fails, or you want to configure again with new cmake options (for example with OPTION_NSV=ON and OPTION_VTK=ON to enable hybrid simulations), then it's generally easiest to clean out the cmake directory with "rm -r *" before running CMake again.

The following additional things are likely to be particularly useful:

SmoldynQuickGuide.pdf - a 2 page Smoldyn quick reference guide, in documentation directory
SmoldynUsersManual.pdf - the full Smoldyn User Manual, in documentation directory
examples - a folder of Smoldyn configuration files

------------- Necessary libraries -----------------

Smoldyn requires several widely used libraries, including libtiff and maybe zlib.  You might also need freeglut, libXmu, and libXi, which are all part of the OpenGL graphics suite.  Some systems don't have these, so you need to get them.  Use the yum package manager for Fedora linux and the apt-get package manager for Ubuntu.  For Mac, I prefer MacPorts, although Fink and HomeBrew are also supposed to be good.  You can also download libraries from their websites and compile yourself, which is usually fairly straightforward (./configure, make, sudo make install).

Be sure to get the developer library versions, if there is a choice, because these come with header files, which Smoldyn needs.  For example, entering "sudo yum install libtiff" on a Fedora system won't give you what you need; instead, enter "sudo yum install libtiff-devel".  Likewise, get zlib-devel, freeglut-devel, libXmu-devel, and libXi-devel.  The same advice probably holds for Ubuntu and other Linux flavors; I don't know about Mac.

If you think you have a library but don't know where it is on your system, search for it with: "find / -name tiff.h 2>/def/null" for the tiff.h header file, and similar for other libraries.  This might take a minute or so to run, but will show where all copies of that file are on your system.


-------- If building and installation failed ------

Most likely, you need some additional libraries, all of which are available on the web.  See the section above and also the Smoldyn user manual (in particular SmoldynCodeDoc.pdf).

If the "cmake .." building step completed successfully, but then "make" returns the error that there is no makefile, then look at the last line of the cmake output.  It will say where the build files were written to.  They are supposed to be in the cmake directory, but in at least one instance they got put in the Smoldyn directory intead.  If they are somewhere else, then navigate to that directory and enter "make" in that directory.

If Smoldyn appeared to compile and install, but it doesn't run, check the following:

Did the code get installed to the correct place?  Check by typing "ls /usr/local/bin" and looking for smoldyn.

Does your computer know where to look for programs?  Type "echo $PATH" to get a list of colon-separated places where the computer looks.  If /usr/local/bin isn't in this list, then you'll need to add it to your profile file.  See the Smoldyn documentation (or look on-line).

Is your system allowing you to run the code?  If you're told that permission was denied for running smoldyn, then your computer might not have realized that Smoldyn is an executable program.  Enter "chmod +x /usr/local/bin/smoldyn"

Feel free to e-mail support@smoldyn.org for assistance.

