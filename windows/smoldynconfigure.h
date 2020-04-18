/* Smoldyn configuration file for input to CMake.  */
/* Written by Steve Andrews, 4/12/12.  */

/* Define to 1 if this build is for Apple */
/* #undef APPLE_BUILD */

/* Define to 1 if this build is for Unix/Linux */
/* #undef NIX_BUILD */

/* Define to 1 if this build is for Windows */
#define WINDOWS_BUILD

/* Define to 1 if you have the <GL/gl.h> header file. */
#define HAVE_GL_GL_H

/* Define to 1 if you have the <OpenGL/gl.h> header file. */
/* #undef HAVE_OPENGL_GL_H */

/* Define to 1 if you have the <GL/glu.h> header file. */
#define HAVE_GL_GLU_H

/* Define to 1 if you have the <OpenGL/glu.h> header file. */
/* #undef HAVE_OPENGL_GLU_H */

/* Define to 1 if you have the <GL/glut.h> header file. */
/* #undef HAVE_GL_GLUT_H */

/* Define to 1 if you have the <GLUT/glut.h> header file. */
/* #undef HAVE_GLUT_GLUT_H */

/* Define to 1 if you have the <GL/freeglut.h> header file. */
#define HAVE_GL_FREEGLUT_H

/* Define to 1 if you have the <memory.h> header file. */
/* #undef HAVE_MEMORY_H */

/* Whether to compile Smoldyn with OpenGL support */
#define HAVE_OPENGL

/* Whether to compile Smoldyn with LibTiff support */
/* #undef HAVE_LIBTIFF */

/* Whether to compile Smoldyn with Zlib support */
/* #undef HAVE_ZLIB */

/* Whether to compile Smoldyn with lattice support */
/* #undef OPTION_LATTICE */

/* Whether to compile Smoldyn with vtk support */
/* #undef HAVE_VTK */

/* Whether to compile Smoldyn with nsv support */
/* #undef OPTION_NSV */

/* Whether to compile Smoldyn with pde support */
/* #undef OPTION_PDE */

/* Whether to compile Smoldyn with vtk support */
/* #undef OPTION_VTK */

/* Define to the version of this package. */
#define VERSION "2.61"

/* Define the path for the BNG2.pl program */
#define BNG2_PATH "C:\\Program Files\\Smoldyn\\BioNetGen\\BNG2.pl"

/* Whether to compile Smoldyn for VCell */
/* #undef OPTION_VCELL */
