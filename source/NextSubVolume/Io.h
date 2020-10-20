/* 
 * io.h
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

#ifndef IO_KAIROS_H_
#define IO_KAIROS_H_

#if defined(HAVE_VTK)

#include <string>
#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>
#include "StructuredGrid.h"
#include "Species.h"


namespace Kairos {
	void write_grid(std::string filename, vtkUnstructuredGrid* grid);
	vtkSmartPointer<vtkUnstructuredGrid> get_vtk_grid(StructuredGrid& subvolumes,std::vector<Species*>& all_species);
	void write_points(std::string filename,
			const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z);
	void write_column_vectors(std::string filename, std::string header,std::vector<std::vector<double>* >& columns);
}


#endif /* VTK */
#endif /* IO_H_ */
