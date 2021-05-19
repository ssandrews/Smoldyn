
#define VTKWRAPPER_CPP
#include "vtkwrapper.h"
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <vtkPointData.h>
#include <vtkPoints.h>

#include <vtkSphereSource.h>

#include <sstream>

vtkRenderer* renderer;
vtkRenderWindow* render_window;
vtkRenderWindowInteractor* render_window_interator;

vtkUnstructuredGrid*
vtkCreateMolGrid()
{
    vtkUnstructuredGrid* grid = vtkUnstructuredGrid::New();
    vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkIntArray> newInt;

    newInt = vtkSmartPointer<vtkIntArray>::New();
    newInt->SetName("id");
    grid->GetPointData()->AddArray(newInt);

    newInt = vtkSmartPointer<vtkIntArray>::New();
    newInt->SetName("species");
    grid->GetPointData()->AddArray(newInt);

    grid->SetPoints(newPts);
    grid->SetCells(1, cells);

    return grid;
}

void
vtkAddPoint(vtkUnstructuredGrid* grid,
            double x,
            double y,
            double z,
            long int serno,
            int species)
{

    const int i = grid->GetPoints()->InsertNextPoint(x, y, z);
    grid->GetCells()->InsertNextCell(1);
    grid->GetCells()->InsertCellPoint(i);
    grid->GetCellTypesArray()->InsertNextTuple1(1);
    int serno_int = int(serno);
    vtkIntArray::SafeDownCast(grid->GetPointData()->GetArray(0))
      ->InsertNextTupleValue(&serno_int);
    vtkIntArray::SafeDownCast(grid->GetPointData()->GetArray(1))
      ->InsertNextTupleValue(&species);
}

void
vtkWriteGrid(const char* nm,
             const char* type,
             int timestep,
             vtkUnstructuredGrid* grid)
{

    vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
      vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();

#if VTK_MAJOR_VERSION > 5
    writer->SetInputData(grid);
#else
    writer->SetInput(grid);
#endif

    writer->SetDataModeToBinary();

    char buffer[100];
    sprintf(buffer, "%s%s%05d.vtu", nm, type, timestep);

    writer->SetFileName(buffer);
    writer->Write();
}

void
vtkDeleteGrid(vtkUnstructuredGrid* grid)
{
    grid->Delete();
}

/*
extern vtkGlyph3D* vtkCreateParticleGlyphs(vtkRenderer* renderer) {
    vtkGlyph3D* glyph = vtkGlyph3D::New();
    vtkSphereSource *sphere = vtkSphereSource::New();;
    glyph->SetSourceConnection(sphere->GetOutputPort());
    glyph->SetScaleModeToScaleByScalar();
    glyph->SetColorModeToColorByScale();
    return glyph;
}

extern void vtkUpdateParticleGlyphs(vtkUnstructuredGrid* grid,
        vtkGlyph3D* glyph) {
    glyph->SetInput(grid);
}

extern void vtkCreateRenderWindow() {
    renderer = vtkRenderer::New();
    render_window = vtkRenderWindow::New();
    render_window->AddRenderer(renderer);
    render_window_interator = vtkRenderWindowInteractor::New();
    render_window_interator->SetRenderWindow(render_window);
}


extern void vtkRender() {
}
*/

