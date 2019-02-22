/*
 * species.h
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
 *  Created on: 11 Oct 2012
 *      Author: robinsonm
 */

#ifndef SPECIES_H_
#define SPECIES_H_

#include <vector>
#include "Vector.h"
#include "StructuredGrid.h"


namespace Kairos {

class Species {
public:
	Species(const int id, double D, const StructuredGrid* grid):D(D),grid(grid),id(id)  {
		clear();
	}
	void clear() {
		copy_numbers.assign(grid->size(),0);
	}
	void get_concentration(const StructuredGrid& calc_grid, std::vector<double>& concentration) const;
	std::string get_status_string();

	double D;
	double step_length;
	std::vector<int> copy_numbers;
	std::vector<Vect3d> particles;
	std::vector<Vect3d> particlesx;
	const StructuredGrid* grid;
	const int id;
};

}

#endif /* SPECIES_H_ */
