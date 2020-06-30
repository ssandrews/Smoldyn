Macintosh
---------

1. At the Smoldyn download webpage,
   http://www.smoldyn.org/download.html, download the latest Mac
   version.
2. Open your ``Terminal`` application, which is in your
   Applications/Utilities directory.
3. Change directories to this download directory (probably type
   ``cd    Desktop/smoldyn-2.xx-mac``, or something similar).
4. Type ``sudo ./install.sh`` and enter your computer password when
   prompted. If you are asked whether you want the installer to update
   your environment ``PATH`` variable, you should generally say yes
   (enter ``y``). This will add the directory ``/usr/local/bin`` to the
   list of places where your computer will look for executable files,
   which means that it will find Smoldyn correctly.
5. Test Smoldyn by typing ``smoldyn examples/S1\_intro/bounce3.txt``.

.. note:: If installation failed

   Type `smoldyn -V`. This should run Smoldyn just enough to print out the
   version number. If this works, then you have Smoldyn and it runs, but
   Smoldyn wasn't finding the input file.

   __Did the Smoldyn software get installed to the correct place?__
   Check by typing `ls /usr/local/bin` and see if smoldyn is in the directory.

   __Does your computer know where to look for programs?__
   Type `echo $PATH` to get a list of colon-separated places where the
   computer looks. If `/usr/local/bin` isn't in this list, then you need to
   add it to your profile file ([Google `edit path
   mac`](https://www.google.com/search?client=firefox-b-d&q=edit+path+mac))

   __Is your system allowing you to run the code?__ If you're told that
   permission was denied for running smoldyn, then your computer might not
   have realized that Smoldyn is an executable program. Enter `sudo chmod +x
   /usr/local/bin/smoldyn`.

   E-mail <support@smoldyn.org> for assistance.

Windows
-------

1. At the Smoldyn download webpage,
   http://www.smoldyn.org/download.html, download the latest Windows
   version. Your browser may warn you about the dangers of downloading
   software, but this file is almost certainly ok; I compiled it on a
   clean Windows computer using only files that I wrote myself and a few
   widely used libraries, so it is extremely unlikely that there is a
   virus in it.

**If you have administrator privileges**

2. Extract the zip file. Do this by right-clicking on the icon of the
   downloaded file and selecting ``extract to smoldyn-2.xx-windows``.
   This should extract the file to your home directory.

3. Open a ``Command Prompt`` application as administrator. You can find
   the command prompt by searching for it with the Start menu. Rather
   than left-clicking on the Command prompt result that appears, right
   click on it, and select ``run as administrator.`` The computer emits
   scary warnings, but reply yes anyhow.

4. Change directories to the Smoldyn directory (probably type
   ``cd    Downloads/smoldyn-2.xx-windows``, or something similar).

5. Type ``install``. This will copy the Smoldyn files to a new Smoldyn
   subdirectory of your ``C:\\Program Files`` directory. This will also
   update your ``%PATH%`` environment variable so your computer knows
   where to find the software. Note that it is possible for the
   installer to corrupt your ``PATH`` variable if it was unusually long
   (about 1024 characters). If this happens, revert the variable using
   the file ``PATH_old.txt``, where the installer saves the existing
   ``PATH`` variable before modifying it.

6. Exit the command prompt as administrator, and start a new command
   prompt, not as administrator.

7. Test Smoldyn by typing ``smoldyn examples/S1\_intro/bounce3.txt``.

**If you don’t have administrator privileges**

2. Extract the zip file to the desired location. Do this by
   right-clicking on the icon of the downloaded file and selecting
   ``extract file...`` and then enter the directory where you want the
   file.

3. Open a ``Command Prompt`` application. You can find it by searching
   for it with the Start menu.

4. In the Command Prompt, change directories to the Smoldyn download
   (use ``cd`` to change directories, and ``dir`` to list directory
   contents).

5. Test Smoldyn by typing ``smoldyn examples/S1\_intro/bounce3.txt``.
   Smoldyn should work just as well as if it was installed, but you will
   need to be in this directory to run it.

.. note:: If installation failed
   If you get errors due to missing dll files, look in the dll directory in
   the Smoldyn download. If the needed dll file is in there, then simply copy
   it to the same directory where the smoldyn.exe file is. E-mail
   <support@smoldyn.org> for assistance.

Compiling Smoldyn
-----------------

Compiling on Macintosh
~~~~~~~~~~~~~~~~~~~~~~

1. You will need a C compiler and the Make utility. To check if you have
   them, simply type ``gcc`` at a shell prompt. If it says
   ``command not found``, then you need to get it. To get it, go to
   http://developer.apple.com/xcode and click on the
   ``view in Mac App store button`` to be taken to the Xcode site in the
   Mac App store. Then, click on the ``Free`` button, register for a
   free Apple Developer Connection account if you don’t have one
   already, and click on the same button, which is now called
   ``Install App``. This will install XCode. However, it still won’t
   work properly. Next, start XCode and go to the ``Preferences...``
   menu item, click on ``downloads`` and install the
   ``Command line tools``.
2. OpenGL should already be installed on your computer. To check, type
   ``ls    /System/Library/Frameworks`` and you should see folders
   called GLUT.framework and OpenGL.framework. If they aren’t there,
   then you’ll need to get them.
3. You will need the CMake configuration software. To see if you already
   have it, type ``cmake``; this will produce the help information if
   you have it, or an error message if not. If you don’t have it, you
   need to download and install it.
4. Libtiff is a library that Smoldyn uses for saving tiff format images,
   which you probably do not have. It is not required for Smoldyn to
   run, but it necessary to save images. One way to install Libtiff is
   to download it from http://www.libtiff.org, uncompress it, and
   install it. To install it, start a terminal window, change to the
   libtiff directory, and follow the README instructions: type
   ``./configure``, then ``make``, then ``sudo make install`` and your
   password. This will install libtiff header files to
   /usr/local/include and libtiff library archives in /usr/local/lib.

!!! tip “Installing libtiff using MacPorts”

::

   Another method (but one which I think is harder) is to use MacPorts or Fink.
   For MacPorts, type `port search libtiff`. If you get the error message 
   `port: command not found`, then you don’t have MacPorts. If this is the case, then you
   can get MacPorts from <www.macports.org> and try again. When the command works,
   it should list a few packages, one of which is called `tiff @3.8.2 (graphics)`,
   or something very similar.  Install it by typing `sudo port install tiff`,
   followed by your password. This will install libtiff to
   `/opt/local/var/macports/software/`. 

   This is great, except that the Smoldyn build system prefers for libtiff to
   be in `/usr/local/lib`. The solution is to set `LIBTIFF_CFLAGS` and
   `LIBTIFF_LDFLAGS` manually when you type `./configure` for Smoldyn. This will
   override Smoldyn’s search for the libraries and will link them in properly.
   For Fink, exactly the same advice applies, except that Fink installs
   libraries to `/sw`. For example, if libtiff is installed to `/sw/local`, then
   configure with: 

   ```
   $ LIBTIFF_CFLAGS="-I/sw/local/include" LIBTIFF_LDFLAGS="-L/sw/local/lib -ltiff" ./configure
   ```

5. Install Smoldyn by changing to the ``cmake`` directory. Then type
   ``cmake ..``, then ``make``, and then ``sudo make install``, and
   finally your password. Some custom installation options can be
   selected with the ``cmake ..`` line if you want them; they are listed
   below in the sections on installing to a custom location and on
   installation problems, and also in the Smoldyn programmers manual. To
   clean up temporary files, *which is essential if you want to try
   building a second time*, first enter ``pwd`` and confirm that you are
   still in the ``cmake/`` directory (don’t continue if not!). Then,
   type ``rm -r \*`` to clear out all prior build stuff.

6. Test Smoldyn

   -  Type ``smoldyn -V`` to just print out the Smoldyn version number.
      If it doesn’t work, then the most likely problem is that your
      system is not set up to run programs that are in your
      /usr/local/bin directory, which is where Smoldyn is installed by
      default. To fix this temporarily, type
      ``export PATH=$PATH:/usr/local/bin``; to fix it permanently,
      although it will only take effect after you open a new terminal
      window, use emacs or some other editor to edit the file ~/.profile
      and add the line ``export     PATH=$PATH:/usr/local/bin``.
   -  Type ``smoldyn examples/S8\_reactions/lotvolt/lotvolt.txt`` to run
      a Lotka-Volterra simulation. If a graphics window doesn’t appear,
      then the OpenGL linking somehow failed. Otherwise, press ‘T’
      (upper-case) at some point during the simulation to save a
      tiff-format image of the graphical display. If it works, it will
      be saved to the current directory as OpenGL001.tif; if not, then
      the libtiff linking somehow failed.

Compiling options
~~~~~~~~~~~~~~~~~

Various building options are possible with the CMake build system, of
which the most important are as follows. In all cases, append these to
the ``cmake ..`` command.

-  ``-DOPTION_STATIC=ON`` Build using static libraries
-  ``-DCMAKE_BUILD_TYPE=...`` Choose CMake build type options are:

   -  None
   -  Debug
   -  Release (default)
   -  RelWithDebInfo
   -  MinSizeRel

-  ``-DOPTION_USE_OPENGL=OFF`` Build without graphics support
-  ``-DOPTION_USE_LIBTIFF=OFF`` Build without LibTiff support
-  ``-DOPTION_USE_ZLIB=OFF`` Build without ZLib support
-  ``-OPTION_TARGET_SMOLDYN=OFF`` Don’t build stand-alone Smoldyn
   program
-  ``-DOPTION_TARGET_LIBSMOLDYN=ON`` Build LibSmoldyn library
-  ``-DOPTION_NSV=ON`` Build with next subvolume support
-  ``-DOPTION_VTK=ON`` Build with VTK output support

By default, the Smoldyn build system installs Smoldyn to either the
``/usr`` or the ``/usr/local`` directories, depending on your system.
These are the standard places for programs like Smoldyn, but you will
need root access for the installation (typically only system
administrators have the necessary ``su`` or ``sudo`` access to install
to these locations). If you use a computer on a shared computer, you may
not have this access. If this is the case, then you will have to pick a
different install directory, such as ``~/usr``. There are standard
options to configure Smoldyn to install here, for the CMake build system

The drawback to installing in a non-standard location is that your
system may not find Smoldyn when you try to run it. To solve this, you
need to add the directory ``~/usr``, or wherever you installed Smoldyn,
to your ``PATH`` variable. This is explained above in instruction 5a for
the regular Macintosh installation, except that here you would add
``export PATH=$PATH:~/usr/bin``.

Compiling on a UNIX/Linux system
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For the most part, installing on a UNIX or Linux system is the same as
for Macintosh, described above. Following are a few Linux-specific
notes.

To download Smoldyn from a command line, type
``wget http://www.smoldyn.org/smoldyn-2.xx.tar.gz``, where the ``xx`` is
the current version number. Then unpack it with
``tar xzvf smoldyn-2.xx.tar.gz``.

For a full installation, you will need OpenGL and Libtiff. I don’t know
how to install them for all systems, but it turned out to be easy for my
Fedora release 7. I already had OpenGL, but not the OpenGL glut library
nor Libtiff. To install them, I entered
``sudo yum install freeglut-devel`` and ``sudo yum install libtiff``,
respectively, along with my password.

Ubuntu systems are slightly more finicky than others. First, you may
need to install several things as follows. Install a C++ compiler with
``sudo apt-get install g++``, install a Python header file with
``sudo apt-get install python-dev``, install the OpenGL glut library
with ``sudo apt-get install freeglut3-dev``, and install the libtiff
library with ``sudo apt-get install libtiff4-dev``.

Running Smoldyn remotely
~~~~~~~~~~~~~~~~~~~~~~~~

It can be helpful to have Smoldyn installed on computer A and run from
computer B. Running Smoldyn without graphics is trivial. Just ssh into
computer A as normal, and run Smoldyn with ``smoldyn filename.txt -t``,
where the -t flag implies text-only operation. If you want graphics
though, then log in with ``ssh -Y me@compA/directory`` and run Smoldyn
as normal. Graphics will be slow but should be functional.

Alternatively, I’ve found the free software TeamViewer to be a
convenient method for working on computers remotely. An advantage of
this method is that it works even if there are institutional firewalls
that prohibit remote computer access.

Building on Linux
-----------------

Fedora/CentOS
~~~~~~~~~~~~~

.. code:: bash

   $ sudo dnf install gcc-c++ cmake freeglut-devel libtiff-devel libXmu-devel libXi-devel

OpenSUSE
~~~~~~~~

.. code:: bash

   $ sudo zypper install gcc-c++ cmake freeglut-devel libtiff-devel libXmu-devel libXi-devel

Debian/Ubuntu and other derivatives
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

\```bash $ sudo apt install g++ cmake libtiff-dev libjpeg-dev
freeglut3-dev libxi-dev libxmu-dev
