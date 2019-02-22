#ifndef STRUCTUREDGRID_IMPL_H_
#define STRUCTUREDGRID_IMPL_H_

namespace Kairos {


template<unsigned int DIM>
AxisAlignedRectangle<DIM> StructuredGrid::get_intersection_of_cell(const int i,
		const AxisAlignedPlane<DIM>& geometry) const {
//	const double dim_map[3][2] = {{1,2},{0,2},{0,1}};		// commented out because variable is unused
	Vect3d low_point = index_to_vect(i) * (cell_size)+low;
	Vect3d high_point = low_point + cell_size;
	low_point[DIM] = geometry.get_coord();
	high_point[DIM] = low_point[DIM];
	const int norm = geometry.get_normal();
	return AxisAlignedRectangle<DIM>(low_point, high_point, norm);
}

template<unsigned int DIM>
AxisAlignedRectangle<DIM> StructuredGrid::get_intersection_of_cell(const int i,
		const AxisAlignedRectangle<DIM>& geometry) const {
//	const double dim_map[3][2] = {{1,2},{0,2},{0,1}};			// commented out because variable is unused
	Vect3d low_point = index_to_vect(i) * (cell_size)+low;
	Vect3d high_point = low_point + cell_size;
	low_point[DIM] = geometry.get_coord();
	high_point[DIM] = low_point[DIM];
	const int norm = geometry.get_normal();
	return AxisAlignedRectangle<DIM>(low_point, high_point, norm);
}

template<unsigned int DIM>
void StructuredGrid::get_slice(const AxisAlignedRectangle<DIM>& surface, std::vector<int>& indices) const {
	const double coord_index_double = (surface.get_coord()-low[DIM])*inv_cell_size[DIM];
	const int coord_index_int = int(floor(coord_index_double + surface.get_normal()*tolerance));
	if ((coord_index_int < 0) || (coord_index_int > num_cells_along_axes[DIM]-1)) return;
	static const int dim_map[][2] = {{1,2}, {0,2}, {0,1}};

	Vect3d low = surface.get_low();
	low[dim_map[DIM][0]] += tolerance;
	low[dim_map[DIM][1]] += tolerance;
	Vect3i min_index = get_cell_index_vector(low);

	Vect3d high = surface.get_high();
	high[dim_map[DIM][0]] -= tolerance;
	high[dim_map[DIM][1]] -= tolerance;
	Vect3i max_index = get_cell_index_vector(high);

	ASSERT(max_index[DIM]==min_index[DIM],"surface not an axis aligned rectangle");

	int num_indicies = (max_index[dim_map[DIM][0]] - min_index[dim_map[DIM][0]] + 1) *
			(max_index[dim_map[DIM][1]] - min_index[dim_map[DIM][1]] + 1);

	const int size = indices.size() + num_indicies;
	int ret_index = indices.size();

	indices.resize(size);
	Vect3i vect_index(0,0,0);
	vect_index[DIM] = coord_index_int;
	for (int i = min_index[dim_map[DIM][0]]; i <= max_index[dim_map[DIM][0]]; ++i) {
		vect_index[dim_map[DIM][0]] = i;
		for (int j = min_index[dim_map[DIM][1]]; j <= max_index[dim_map[DIM][1]]; ++j) {
			vect_index[dim_map[DIM][1]] = j;
			const int index = vect_to_index(vect_index);
			ASSERT(ret_index < size,"return index is out of bounds");
			indices[ret_index] = index;
			ret_index++;
		}
	}
}


template<unsigned int DIM>
void StructuredGrid::get_slice(const AxisAlignedPlane<DIM>& surface, std::vector<int>& indices) const {
	const double coord_index_double = (surface.get_coord()-low[DIM])*inv_cell_size[DIM];
	const int coord_index_int = int(floor(coord_index_double + surface.get_normal()*tolerance));

	if ((coord_index_int < 0) || (coord_index_int > num_cells_along_axes[DIM]-1)) return;
	static const int dim_map[][2] = {{1,2}, {0,2}, {0,1}};
	int ret_index = indices.size();
	const int size = ret_index + num_cells_along_axes[dim_map[DIM][0]]*num_cells_along_axes[dim_map[DIM][1]];

	indices.resize(size);
	Vect3i vect_index(0,0,0);
	vect_index[DIM] = coord_index_int;

	for (int i = 0; i < num_cells_along_axes[dim_map[DIM][0]]; ++i) {
		vect_index[dim_map[DIM][0]] = i;
		for (int j = 0; j < num_cells_along_axes[dim_map[DIM][1]]; ++j) {
			vect_index[dim_map[DIM][1]] = j;
			const int index = vect_to_index(vect_index);
			ASSERT(ret_index < size,"return index is out of bounds");
			indices[ret_index] = index;
			ret_index++;
		}
	}
}

template<typename T>
void StructuredGrid::get_slice(const T geometry, std::vector<int>& indices) const {
	indices.clear();
	const int nedges = 12;
	const int edges[nedges][2][3] = {{{0,0,0},{0,0,1}},
			              {{0,0,0},{0,1,0}},
			              {{0,0,0},{1,0,0}},
			              {{0,0,1},{0,1,1}},
			              {{0,0,1},{1,0,0}},
			              {{0,1,0},{1,1,0}},
			              {{0,1,0},{0,1,1}},
			              {{1,0,0},{1,1,0}},
			              {{1,0,0},{1,0,1}},
			              {{0,1,1},{1,1,1}},
			              {{1,1,0},{1,1,1}},
			              {{1,0,1},{1,1,1}}};

	for (int i = 0; i < num_cells; ++i) {
		const Vect3d low_point = index_to_vect(i) * (cell_size)+low;
		for (int j = 0; j < nedges; ++j) {
			const Vect3d p1 = low_point + Vect3d(edges[j][0][0],edges[j][0][1],edges[j][0][2]) * (cell_size);
			const Vect3d p2 = low_point + Vect3d(edges[j][1][0],edges[j][1][1],edges[j][1][2]) * (cell_size);
			if (geometry.lineXsurface(p1,p2)) {
				indices.push_back(i);
				break;
			}

		}
	}
}

template<typename T>
void StructuredGrid::get_region(const T geometry, std::vector<int>& indices) const {
	indices.clear();
	for (int i = 0; i < num_cells; ++i) {
		Vect3d low_point = index_to_vect(i) * (cell_size)+low;

		for (int i = 0; i < 2; ++i) {
			for (int j = 0; j < 2; ++j) {
				for (int k = 0; k < 2; ++k) {
					const Vect3d test_point = low_point + Vect3d(i,j,k) * (cell_size);
					const double dist = geometry.is_in(test_point);
					if (dist < 0) {
						indices.push_back(i);
						continue;
					}
				}
			}
		}
	}
}

}

#endif /* STRUCTUREDGRID_IMPL_H_ */
