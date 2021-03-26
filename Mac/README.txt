This README file is for the Smoldyn distribution for Mac OS X.

This distribution contains pre-compiled Smoldyn software for running on standard Macs.  It assumes that you have an Intel Mac running OS 10.6 or higher, that your system is configured in a typical way, and that you want the default Smoldyn installation.

To install Smoldyn, open your "Terminal" application, which is in your Applications/Utilities directory.  Change directories to this download directory (probably type "cd Desktop/smoldyn-2.xx-mac", or something similar).  Then type "sudo ./install.sh", and enter your computer password when prompted.  If you get a warning about Python bindings not being installed, that's okay; just install pip at some point and re-run the install script.  If you are asked whether you want the installer to update your environment PATH variable, you should generally say yes (enter 'y').  This will add the directory /usr/local/bin to the list of places where your computer will look for executable files, which means that it will find Smoldyn correctly.  Next, test Smoldyn by typing "smoldyn examples/S1_intro/bounce3.txt".  If it runs, congratulations, you have a successful Smoldyn installation!  If not, see below.

Particularly useful files in the download directory:
SmoldynQuickGuide.pdf - a 2 page Smoldyn quick reference guide, in documentation directory
SmoldynManual.pdf - the full Smoldyn User Manual, in documentation directory
examples - a folder of Smoldyn configuration files


-------- What installing does -------

Default Smoldyn installation installs the following files:
- smoldyn executable to /usr/local/bin (this is the main Smoldyn software)
- SmolCrowd executable to /usr/local/bin (utility program creates crowded volumes)
- wrl2smol executable to /usr/local/bin (utility program converts VRML output to Smoldyn input)
- libsmoldyn.h header file to /usr/local/include (for calling Smoldyn as a library)
- smoldyn.h header file to /usr/local/include (for calling Smoldyn as a library)
- smoldynconfigure.h header file to /usr/local/include (for calling Smoldyn as a library)
- libsmoldyn_static.a static library to /usr/local/lib (for calling Smoldyn as a library)
- libsmoldyn_shared.dylib dynamic library to /usr/local/lib (for calling Smoldyn as a library)
- Minimal BioNetGen files in /usr/local/bin/BioNetGen
- Python bindings for Smoldyn to standard place using pip utility


--------------- Uninstalling Smoldyn -------------

To completely remove all of the Smoldyn files that the installer installed, run the uninstall.sh script.  This is a very simple script that deletes the installed files.  It also runs pip uninstall to remove the Python bindings.  This will not delete the download package from your computer and it doesn't reset the environment PATH variable.


-------- If installation failed ------

Did the code get installed to the correct place?  Check by typing "ls /usr/local/bin" and looking for smoldyn.

Does your computer know where to look for programs?  Type "echo $PATH" to get a list of colon-separated places where the computer looks.  If /usr/local/bin isn't in this list, then you'll need to add it to your profile file.  See the Smoldyn documentation (or look on-line).

Is your system allowing you to run the code?  If you're told that permission was denied for running smoldyn, then your computer might not have realized that Smoldyn is an executable program.  Enter "chmod +x /usr/local/bin/smoldyn"

E-mail support@smoldyn.org for assistance.

