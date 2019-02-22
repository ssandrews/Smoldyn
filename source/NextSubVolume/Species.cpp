/*
 * species.cpp
 *
 *  Created on: 11 Oct 2012
 *      Author: robinsonm
 */

#include "Species.h"
#include <sstream>

namespace Kairos {

void Species::get_concentration(const StructuredGrid& calc_grid,
		std::vector<double>& concentration) const {

	const int n = calc_grid.size();
	concentration.assign(calc_grid.size(),0);
	if (copy_numbers.size() != 0) {
		for (int i = 0; i < n; ++i) {
			std::vector<int> indicies;
			std::vector<double> volume_ratio;
			grid->get_overlap(calc_grid.get_low_point(i),calc_grid.get_high_point(i),indicies,volume_ratio);
			const int noverlap = indicies.size();
			for (int j = 0; j < noverlap; ++j) {
				concentration[i] += copy_numbers[indicies[j]]*volume_ratio[j];
			}
		}
	}

	for (int i = 0; i < n; ++i) {
		const double inv_vol = 1.0/calc_grid.get_cell_volume(i);
		concentration[i] *= inv_vol;
	}
}

std::string Species::get_status_string() {
	std::ostringstream ss;
	ss << "Molecular Status:" << std::endl;

	return ss.str();
}

}




