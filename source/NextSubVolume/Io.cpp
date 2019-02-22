/* 
 * io.cpp
 *
 * Copyright 2012 Martin Robinson
 *
 * This file is part of PDE_BD.
 *
 * PDE_BD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PDE_BD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PDE_BD.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: Feb 16, 2013
 *      Author: mrobins
 */
#if defined(HAVE_VTK)

#include "Io.h"
#include "Kairos.h"

#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>


#include <vtkVersion.h>
#include <iostream>
#include <sstream>
#include <fstream>

namespace Kairos {

void write_grid(std::string filename, vtkUnstructuredGrid* grid) {
	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
			vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
#if VTK_MAJOR_VERSION > 5
	writer->SetInputData(grid);
#else
	writer->SetInput(grid);
#endif
	writer->SetDataModeToBinary();
	writer->SetFileName(filename.c_str());
	writer->Write();
}

vtkSmartPointer<vtkUnstructuredGrid> get_vtk_grid(StructuredGrid& subvolumes,std::vector<Species*>& all_species) {

	vtkSmartPointer<vtkUnstructuredGrid> vtk_grid = subvolumes.get_vtk_grid();

	/*
	 * setup scalar data
	 */
	for (unsigned int i = 0; i < all_species.size(); ++i) {
		vtkSmartPointer<vtkIntArray> newScalars = vtkSmartPointer<vtkIntArray>::New();
		const int data_size = all_species[i]->copy_numbers.size();
		ASSERT(data_size==vtk_grid->GetNumberOfCells(),"Species data does not match size of vtk_grid");
		newScalars->SetArray(all_species[i]->copy_numbers.data(),data_size,1);

		std::ostringstream ss;
		ss << "Concentration (id = " << all_species[i]->id << ")";
		newScalars->SetName(ss.str().c_str());
		vtk_grid->GetCellData()->AddArray(newScalars);
		//vtk_grid->GetCellData()->SetScalars(newScalars);
	}

	return vtk_grid;
}

void write_points(std::string filename, const std::vector<double>& x,
		const std::vector<double>& y, const std::vector<double>& z) {
	/*
	 * setup points
	 */
	vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
	const int n = x.size();
	for (int i = 0; i < n; i++) {
		newPts->InsertNextPoint(x[i],y[i],z[i]);
		cells->InsertNextCell(1);
		cells->InsertCellPoint(i);
	}

	vtkSmartPointer<vtkUnstructuredGrid> vtk_grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	vtk_grid->SetPoints(newPts);
	vtk_grid->SetCells(1,cells);

	vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
			vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();
#if VTK_MAJOR_VERSION > 5
	writer->SetInputData(vtk_grid);
#else
	writer->SetInput(vtk_grid);
#endif
	writer->SetDataModeToBinary();
	writer->SetFileName(filename.c_str());
	writer->Write();
}

void write_column_vectors(std::string filename, std::string header, std::vector<std::vector<double>* >& columns) {
	std::ofstream f;
	f.open (filename.c_str());
	f << header << std::endl;
	for (unsigned int i = 0; i < columns[0]->size(); ++i) {
		for (unsigned int j = 0; j < columns.size()-1; ++j) {
			f << columns[j]->at(i) << " ";
		}
		f << (*(columns.end()-1))->at(i) << std::endl;
	}
	f.close();
}

}
#endif
