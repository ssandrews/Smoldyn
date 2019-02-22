/*
 * StructuredGrid.cpp
 *
 *  Created on: 2 Nov 2012
 *      Author: robinsonm
 */

#include "StructuredGrid.h"
#if defined(HAVE_VTK)
#include <vtkHexahedron.h>
#endif
//extern "C"
//{
#include "random2.h"
//}

namespace Kairos {
void StructuredGrid::reset_domain(const Vect3d& _low, const Vect3d& _high, const Vect3d& max_grid_size) {
	high = _high;
	low = _low;
	num_cells_along_axes = ((high-low) / (max_grid_size) + Vect3d(0.5,0.5,0.5));
	cell_size = (high-low) / (num_cells_along_axes);
	tolerance = cell_size.minCoeff()/100000.0;
	cell_volume = cell_size.prod();
	//std::cout << "cell volume = " << cell_volume << " cell_size = " << cell_size<<std::endl;
	inv_cell_size = Vect3d(1,1,1) / (cell_size);
	num_cells_along_yz = num_cells_along_axes[2]*num_cells_along_axes[1];
	num_cells = num_cells_along_axes.prod();
	neighbours.resize(num_cells);
	calculate_neighbours();
	fill_raw_positions();
}

#if defined(HAVE_VTK)
vtkSmartPointer<vtkUnstructuredGrid> StructuredGrid::get_vtk_grid() {
	if (vtk_grid == NULL) {
		/*
		 * setup points
		 */
		vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();
		for (int i = 0; i <= num_cells_along_axes[0]; ++i) {
			for (int j = 0; j <= num_cells_along_axes[1]; ++j) {
				for (int k = 0; k <= num_cells_along_axes[2]; ++k) {
					Vect3d new_point = Vect3i(i,j,k) * (cell_size) + low;
					newPts->InsertNextPoint(new_point.data());
				}
			}
		}


		/*
		 * setup cells
		 */
		vtk_grid = vtkUnstructuredGrid::New();
		vtk_grid->Allocate(num_cells,num_cells);
		vtk_grid->SetPoints(newPts);
		for (int i = 0; i < num_cells_along_axes[0]; ++i) {
			for (int j = 0; j < num_cells_along_axes[1]; ++j) {
				for (int k = 0; k < num_cells_along_axes[2]; ++k) {
					vtkSmartPointer<vtkHexahedron> newHex = vtkSmartPointer<vtkHexahedron>::New();
					newHex->GetPointIds()-> SetNumberOfIds(8);

					newHex->GetPointIds()-> SetId(0,vect_to_index_nodes(i,j,k));
					newHex->GetPointIds()-> SetId(1,vect_to_index_nodes(i+1,j,k));
					newHex->GetPointIds()-> SetId(2,vect_to_index_nodes(i+1,j+1,k));
					newHex->GetPointIds()-> SetId(3,vect_to_index_nodes(i,j+1,k));
					newHex->GetPointIds()-> SetId(4,vect_to_index_nodes(i,j,k+1));
					newHex->GetPointIds()-> SetId(5,vect_to_index_nodes(i+1,j,k+1));
					newHex->GetPointIds()-> SetId(6,vect_to_index_nodes(i+1,j+1,k+1));
					newHex->GetPointIds()-> SetId(7,vect_to_index_nodes(i,j+1,k+1));

					vtk_grid->InsertNextCell(newHex->GetCellType(),newHex->GetPointIds());
				}
			}
		}
	}
	return vtk_grid;
}
#endif
Vect3d StructuredGrid::get_random_point(const int i) const {
	return low + (index_to_vect(i) + Vect3d(randCCD(),randCCD(),randCCD())) * (cell_size);
}

void StructuredGrid::get_overlap(const Vect3d& overlap_low, const Vect3d& overlap_high, std::vector<int>& indicies, std::vector<double>& volume) const {
	indicies.clear();
	volume.clear();
	if ((overlap_low >= high).any()) return;
	if ((overlap_high <= low).any()) return;
	Vect3d snap_low = overlap_low + Vect3d(tolerance,tolerance,tolerance);
	Vect3d snap_high = overlap_high - Vect3d(tolerance,tolerance,tolerance);
	for (int i = 0; i < 3; ++i) {
		if (snap_low[i] < low[i]) {
			snap_low[i] = low[i];
		}
		if (snap_high[i] > high[i]) {
			snap_high[i] = high[i]-tolerance;
		}
	}
	Vect3i lowi,highi;
	lowi = get_cell_index_vector(snap_low);
	highi = get_cell_index_vector(snap_high);


	const double inv_volume = 1.0/cell_size.prod();
	for (int i = lowi[0]; i <= highi[0]; ++i) {
		for (int j = lowi[1]; j <= highi[1]; ++j) {
			for (int k = lowi[2]; k <= highi[2]; ++k) {
				indicies.push_back(vect_to_index(i, j, k));
				Vect3d low_point = Vect3i(i,j,k) * (cell_size) + low;
				Vect3d high_point = low_point + cell_size;
				for (int i = 0; i < 3; ++i) {
					if (high_point[i] > overlap_high[i]) {
						high_point[i] = overlap_high[i];
					}
					if (low_point[i] < overlap_low[i]) {
						low_point[i] = overlap_low[i];
					}
				}
				volume.push_back((high_point-low_point).prod()*inv_volume);
			}
		}
	}
}

void StructuredGrid::calculate_neighbours() {
	for (int i = 0; i < num_cells_along_axes[0]; ++i) {
		for (int j = 0; j < num_cells_along_axes[1]; ++j) {
			for (int k = 0; k < num_cells_along_axes[2]; ++k) {

				const int cell_i = vect_to_index(Vect3i(i,j,k));
				std::vector<int>& neigh = neighbours[cell_i];

				neigh.clear();

				if (i != 0) neigh.push_back(vect_to_index(Vect3i(i-1,j,k)));
				if (i != num_cells_along_axes[0]-1) neigh.push_back(vect_to_index(Vect3i(i+1,j,k)));
				if (j != 0) neigh.push_back(vect_to_index(Vect3i(i,j-1,k)));
				if (j != num_cells_along_axes[1]-1) neigh.push_back(vect_to_index(Vect3i(i,j+1,k)));
				if (k != 0) neigh.push_back(vect_to_index(Vect3i(i,j,k-1)));
				if (k != num_cells_along_axes[2]-1) neigh.push_back(vect_to_index(Vect3i(i,j,k+1)));

			}
		}
	}
}

double StructuredGrid::get_laplace_coefficient(const int i, const int j) const {
	const int diff = j-i;
	if (diff == num_cells_along_yz) {
		return 1.0/pow(cell_size[0],2);
	} else if (diff == num_cells_along_axes[2]) {
		return 1.0/pow(cell_size[1],2);
	} else if (diff == 1) {
		return 1.0/pow(cell_size[2],2);
	} else if (diff == -num_cells_along_yz) {
		return 1.0/pow(cell_size[0],2);
	} else if (diff == -num_cells_along_axes[2]) {
		return 1.0/pow(cell_size[1],2);
	} else if (diff == -1) {
		return 1.0/pow(cell_size[2],2);
	}
	return 0;
}

double StructuredGrid::get_distance_between(const int i, const int j) const {
	const int diff = j-i;
	if (diff == num_cells_along_yz) {
		return cell_size[0];
	} else if (diff == num_cells_along_axes[2]) {
		return cell_size[1];
	} else if (diff == 1) {
		return cell_size[2];
	} else if (diff == -num_cells_along_yz) {
		return cell_size[0];
	} else if (diff == -num_cells_along_axes[2]) {
		return cell_size[1];
	} else if (diff == -1) {
		return cell_size[2];
	}
	return 0;
}

Rectangle StructuredGrid::get_face_between(const int i, const int j) const {
	Vect3i ii = index_to_vect(i);
	Vect3d clow = ii * (cell_size) + low;
	const int diff = j-i;
	if (diff == num_cells_along_yz) {
		Vect3d lower_left = clow + Vect3d(cell_size[0],0,0);
		Vect3d upper_left = clow + Vect3d(cell_size[0],cell_size[1],0);
		Vect3d lower_right = clow + Vect3d(cell_size[0],0,cell_size[2]);
		return Rectangle(lower_left,upper_left,lower_right);
	} else if (diff == num_cells_along_axes[2]) {
		Vect3d lower_left = clow + Vect3d(0,cell_size[1],0);
		Vect3d lower_right = clow + Vect3d(cell_size[0],cell_size[1],0);
		Vect3d upper_left= clow + Vect3d(0,cell_size[1],cell_size[2]);
		return Rectangle(lower_left,upper_left,lower_right);
	} else if (diff == 1) {
		Vect3d lower_left = clow + Vect3d(0,0,cell_size[2]);
		Vect3d upper_left = clow + Vect3d(cell_size[0],0,cell_size[2]);
		Vect3d lower_right = clow + Vect3d(0,cell_size[1],cell_size[2]);
		return Rectangle(lower_left,upper_left,lower_right);
	} else if (diff == -num_cells_along_yz) {
		Vect3d lower_left = clow + Vect3d(0,0,0);
		Vect3d lower_right = clow + Vect3d(0,cell_size[1],0);
		Vect3d upper_left = clow + Vect3d(0,0,cell_size[2]);
		return Rectangle(lower_left,upper_left,lower_right);
	} else if (diff == -num_cells_along_axes[2]) {
		Vect3d lower_left = clow + Vect3d(0,0,0);
		Vect3d upper_left = clow + Vect3d(cell_size[0],0,0);
		Vect3d lower_right = clow + Vect3d(0,0,cell_size[2]);
		return Rectangle(lower_left,upper_left,lower_right);
	} else if (diff == -1) {
		Vect3d lower_left = clow + Vect3d(0,0,0);
		Vect3d lower_right = clow + Vect3d(cell_size[0],0,0);
		Vect3d upper_left = clow + Vect3d(0,cell_size[1],0);
		return Rectangle(lower_left,upper_left,lower_right);
	} else {
		ERROR("cells are not adjacent");
	}
	return Rectangle(Vect3d(),Vect3d(),Vect3d());
}

void StructuredGrid::fill_raw_positions() {
	raw_positions.resize(3*num_cells);
	for (int i = 0; i < num_cells_along_axes[0]; ++i) {
		for (int j = 0; j < num_cells_along_axes[1]; ++j) {
			for (int k = 0; k < num_cells_along_axes[2]; ++k) {
				const int index = vect_to_index(i,j,k);
				const Vect3d position = (Vect3d(i,j,k) + Vect3d(0.5,0.5,0.5)) * (cell_size) + low;
				for (int d = 0; d < 3; ++d) {
					raw_positions[index*3 + d] = position[d];
				}
			}
		}
	}
}


/*void StructuredGrid::reset_boundary_conditions(const Vect6i& bc) {
	for (int i = 0; i < num_cells_along_axes[0]; ++i) {
		int x_neigh[3];
		x_neigh[0] = i-1;
		x_neigh[1] = i;
		x_neigh[2] = i+1;
		if (i==0) {
			if (bc[0] < 0) {
				x_neigh[0] = i;
			} else if (bc[0] > 0) {
				x_neigh[0] = num_cells_along_axes[0]-1;
			}
		}
		if (i==num_cells_along_axes[0]-1) {
			if (bc[1] < 0) {
				x_neigh[2] = i;
			} else if (bc[1] > 0) {
				x_neigh[2] = 0;
			}
		}

		for (int j = 0; j < num_cells_along_axes[1]; ++j) {
			int y_neigh[3];
			y_neigh[0] = j-1;
			y_neigh[1] = j;
			y_neigh[2] = j+1;
			if (j==0) {
				if (bc[2] < 0) {
					y_neigh[0] = j;
				} else if (bc[2] > 0) {
					y_neigh[0] = num_cells_along_axes[1]-1;
				}
			}
			if (j==num_cells_along_axes[1]-1) {
				if (bc[3] < 0) {
					y_neigh[2] = j;
				} else if (bc[3] > 0) {
					y_neigh[2] = 0;
				}
			}
			for (int k = 0; k < num_cells_along_axes[2]; ++k) {
				int z_neigh[3];
				z_neigh[0] = k-1;
				z_neigh[1] = k;
				z_neigh[2] = k+1;
				if (k==0) {
					if (bc[4] < 0) {
						z_neigh[0] = k;
					} else if (bc[4] > 0) {
						z_neigh[0] = num_cells_along_axes[2]-1;
					}
				}
				if (k==num_cells_along_axes[2]-1) {
					if (bc[5] < 0) {
						z_neigh[2] = k;
					} else if (bc[5] > 0) {
						z_neigh[2] = 0;
					}
				}

				const int cell_i = vect_to_index(Vect3i(i,j,k));
				std::vector<int>& neigh = neighbours[cell_i];
				//std::vector<double>& neigh_dist = neighbour_distances[cell_i];
				neigh.clear();
				neighbour_distances.clear();
//				for (int di = 0; di < 3; ++di) {
//					for (int dj = 0; dj < 3; ++dj) {
//						for (int dk = 0; dk < 3; ++dk) {
//							neigh.push_back(vect_to_index(Vect3i(x_neigh[di],y_neigh[dj],z_neigh[dk])));
//						}
//					}
//				}

				neigh.push_back(vect_to_index(Vect3i(x_neigh[1],y_neigh[0],z_neigh[1])));
				neigh.push_back(vect_to_index(Vect3i(x_neigh[1],y_neigh[2],z_neigh[1])));
				neigh.push_back(vect_to_index(Vect3i(x_neigh[1],y_neigh[1],z_neigh[0])));
				neigh.push_back(vect_to_index(Vect3i(x_neigh[1],y_neigh[1],z_neigh[2])));
				neigh.push_back(vect_to_index(Vect3i(x_neigh[0],y_neigh[1],z_neigh[1])));
								neigh.push_back(vect_to_index(Vect3i(x_neigh[2],y_neigh[1],z_neigh[1])));


				neighbour_distances.push_back(cell_size[1]);
				neighbour_distances.push_back(cell_size[1]);
				neighbour_distances.push_back(cell_size[2]);
				neighbour_distances.push_back(cell_size[2]);
				neighbour_distances.push_back(cell_size[0]);
								neighbour_distances.push_back(cell_size[0]);
			}
		}
	}
}*/
}
