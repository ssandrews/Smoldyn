/*
 * StructuredGrid.h
 *
 * Copyright 2012 Martin Robinson
 *
 * This file is part of RD_3D.
 *
 * RD_3D is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RD_3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RD_3D.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: 2 Nov 2012
 *      Author: robinsonm
 */

#ifndef STRUCTUREDGRID_H_
#define STRUCTUREDGRID_H_

#include "Vector.h"
#include "Geometry.h"
#include "Log.h"
#include <vector>
#include <cmath>
#if defined(HAVE_VTK)
#include <vtkUnstructuredGrid.h>
#include <vtkSmartPointer.h>
#endif


namespace Kairos {
class StructuredGrid {
public:
	StructuredGrid():
#if defined(HAVE_VTK)
		vtk_grid(NULL),
#endif
		num_cells(0),
		low(Vect3d(0,0,0)),
		high(Vect3d(0,0,0)),
		domain_size(Vect3d(0,0,0)),
		cell_size(Vect3d(0,0,0)),
		inv_cell_size(Vect3d(0,0,0)),
		num_cells_along_axes(Vect3i(0,0,0)),
		num_cells_along_yz(0)
	{}

	StructuredGrid(const Vect3d& low, const Vect3d& high, const double max_grid_size):
		low(low),high(high),
		domain_size(high-low) {
		reset_domain(low, high, Vect3d(max_grid_size,max_grid_size,max_grid_size));
	}

	StructuredGrid(const Vect3d& low, const Vect3d& high, const Vect3d& max_grid_size):
		low(low),high(high),
		domain_size(high-low) {
		reset_domain(low, high, max_grid_size);
	}

	Vect3d get_random_point(const int i) const;

	template<unsigned int DIM>
	void get_slice(const AxisAlignedRectangle<DIM>& surface, std::vector<int>& indices) const;

	template<unsigned int DIM>
	void get_slice(const AxisAlignedPlane<DIM>& surface, std::vector<int>& indices) const;

	template<typename T>
	void get_slice(const T geometry, std::vector<int>& indices) const;

	template<typename T>
	void get_region(const T geometry, std::vector<int>& indices) const;

	void get_overlap(const Vect3d& low, const Vect3d& high, std::vector<int>& indicies, std::vector<double>& volume) const;

	void reset_domain(const Vect3d& low, const Vect3d& high, const Vect3d& max_grid_size);
	//void reset_boundary_conditions(const Vect6i& bc);

	inline const Vect3d& get_low() const {
		return low;
	}
	inline const Vect3d& get_high() const {
		return high;
	}
	inline double get_cell_volume(const int i) const {
		return cell_volume;
	}
	inline int size() const {
		return num_cells;
	}
	inline Vect3d get_cell_size() const {
		return cell_size;
	}
	inline Vect3d get_domain_size() const {
		return domain_size;
	}
	double get_tolerance() const {
		return tolerance;
	}
	Vect3i get_cell_indicies(const int i) const {
		return index_to_vect(i);
	}
	Vect3d get_cell_centre(const int i) const {
		return (index_to_vect(i) + Vect3d(0.5,0.5,0.5)) * (cell_size) + low;
	}
	Vect3d get_low_point(const int i) const {
		return index_to_vect(i) * cell_size + low;
	}
	Vect3d get_high_point(const int i) const {
		return (index_to_vect(i) + Vect3i(1,1,1)) * (cell_size) + low;
	}
	Vect3i get_cells_along_axes() const {
		return num_cells_along_axes;
	}
	const std::vector<int>& get_neighbour_indicies(const int i) const {
		return neighbours[i];
	}

	inline bool is_in(const Vect3d& r) const {
		return ((r >= low).all()) && ((r < high).all());
	}
	inline Vect3i get_cell_index_vector(const Vect3d &r) const {
			ASSERT(((r >= low).all()) && ((r < high).all()), "point "<<r<<" outside structured grid range!!!");
			return Vect3i((r-low) * (inv_cell_size));
	}
	inline int get_cell_index(const Vect3d &r) const {
		ASSERT(((r >= low).all()) && ((r < high).all()), "point "<<r<<" outside structured grid range!!!");
		const Vect3i celli((r-low) * (inv_cell_size));
		return vect_to_index(celli);
	}
	template<unsigned int DIM>
	AxisAlignedRectangle<DIM> get_intersection_of_cell(const int i, const AxisAlignedPlane<DIM>& geometry) const;
	template<unsigned int DIM>
	AxisAlignedRectangle<DIM> get_intersection_of_cell(const int i, const AxisAlignedRectangle<DIM>& geometry) const;
#if defined(HAVE_VTK)
	vtkSmartPointer<vtkUnstructuredGrid> get_vtk_grid();
#endif
	Rectangle get_face_between(const int i, const int j) const;
	double get_laplace_coefficient(const int i, const int j) const;
	double get_distance_between(const int i, const int j) const;
	const std::vector<double>& get_raw_positions() const {return raw_positions;}

private:
#if defined(HAVE_VTK)
	vtkSmartPointer<vtkUnstructuredGrid> vtk_grid;
#endif
	void calculate_neighbours();
	void fill_raw_positions();
	inline int vect_to_index(const int i, const int j, const int k) const {
		return i * num_cells_along_yz + j * num_cells_along_axes[2] + k;
	}
	inline int vect_to_index(const Vect3i& vect) const {
		return vect[0] * num_cells_along_yz + vect[1] * num_cells_along_axes[2] + vect[2];
	}
	inline int vect_to_index_nodes(const int i, const int j, const int k) const {
		return i * (num_cells_along_axes[1]+1)*(num_cells_along_axes[2]+1) + j * (num_cells_along_axes[2]+1) + k;
	}
	Vect3i index_to_vect(const int i) const {
		Vect3i ret;
		ret[2] = i%num_cells_along_axes[2];
		const int i2 = (int)floor(i/num_cells_along_axes[2]);
		ret[1] = i2%num_cells_along_axes[1];
		ret[0] = (int)floor(i2/num_cells_along_axes[1]);
		return ret;
	}

	int num_cells;
	Vect3d low,high,domain_size;
	Vect3d cell_size,inv_cell_size;
	Vect3i num_cells_along_axes;
	double cell_volume;
	int num_cells_along_yz;
	double tolerance;
	std::vector<std::vector<int> > neighbours;
	std::vector<double> raw_positions;
};
}
#include "StructuredGrid.impl.h"

#endif /* STRUCTUREDGRID_H_ */
