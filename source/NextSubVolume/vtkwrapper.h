
#ifndef VTKWRAPPER_H_
#define VTKWRAPPER_H_

#ifdef VTKWRAPPER_CPP
	#include <vtkUnstructuredGrid.h>
	#include <vtkRenderWindow.h>
	#include <vtkRenderer.h>
	#include <vtkRenderWindowInteractor.h>
	#include <vtkGlyph3D.h>
#else
	typedef struct vtkUnstructuredGrid vtkUnstructuredGrid;	// a warning arises here because this was decared as a class previously.  However, changing this to a class casuss many more errors.
	typedef struct vtkRenderer vtkRenderer;
	typedef struct vtkGlyph3D vtkGlyph3D;
	typedef struct vtkRenderWindowInteractor vtkRenderWindowInteractor;

#endif



#ifdef __cplusplus
extern "C" {
#endif


extern vtkUnstructuredGrid* vtkCreateMolGrid();
extern void vtkAddPoint(vtkUnstructuredGrid* grid,
					double x, double y, double z,
					long int serno, int species);

extern void vtkWriteGrid(const char *nm, const char *type, int timestep, vtkUnstructuredGrid* grid);
extern void vtkDeleteGrid(vtkUnstructuredGrid* grid);
extern vtkGlyph3D *vtkCreateParticleGlyphs(vtkRenderer *renderer);
//extern void vtkUpdateParticleGlyphs(vtkUnstructuredGrid* grid, vtkGlyph3D *glyphs);
//extern vtkGlyph3D *vtkCreateCompartments(vtkRenderer *renderer);
//extern void vtkUpdateCompartments(vtkUnstructuredGrid* grid, vtkGlyph3D *glyphs);
//extern void vtkCreateRenderWindow();
//extern void vtkRender();


#ifdef __cplusplus
}
#endif

#endif /* VTKWRAPPER_H_ */
