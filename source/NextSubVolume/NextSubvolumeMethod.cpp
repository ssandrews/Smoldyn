/*
 * NextSubvolumeMethod.cpp
 *
 *  Created on: 15 Oct 2012
 *      Author: robinsonm
 */

#include "NextSubvolumeMethod.h"
#include "Log.h"
#include "Constants.h"

#include <numeric>
#include <limits>
//extern "C"
//{
#include "random2.h"
//}



namespace Kairos {
bool ReactionsWithSameRateAndLHS::add_if_same_lhs(const double rate_to_add, const ReactionSide& lhs_to_add, const ReactionSide& rhs_to_add) {
	//std::sort(lhs_to_add);
	if ((lhs_to_add == lhs)&&(rate_to_add == rate)) {
		all_rhs.push_back(rhs_to_add);
		//			if (lhs[0].compartment_index==0) {
		//				std::cout<<"found duplicate lhs, adding new rhs with ci = "<<rhs_to_add[0].compartment_index<<". new all_rhs.size() = "<<all_rhs.size()<<std::endl;
		//			}
		return true;
	} else {
		return false;
	}
}

ReactionSide& ReactionsWithSameRateAndLHS::pick_random_rhs(const double rand) {
	const int n = all_rhs.size();
	if (n==1) return all_rhs[0];
	return all_rhs[int(floor(rand*n))];
}



void ReactionList::list_reactions() {
	//for (auto& r : reactions) {
	for (std::vector<ReactionsWithSameRateAndLHS>::iterator r=reactions.begin();r!=reactions.end();r++) {
		std::cout <<"With rate = " << r->rate << ":" << std::endl;
		//for (auto& rhs : r.all_rhs) {
		for (std::vector<ReactionSide>::iterator rhs=r->all_rhs.begin();rhs!=r->all_rhs.end();rhs++) {
			//for (auto& c : r.lhs) {
			for (std::vector<ReactionComponent>::iterator c=r->lhs.begin();c!=r->lhs.end();c++) {
				//std::cout << "(" << c.multiplier << "*" << c.species->id << "<" << c.compartment_index << ">) ";
				std::cout << "(" << c->multiplier << "*" << c->species->id << "<" << c->compartment_index << ">) ";
			}
			std::cout << "-> ";
			//for (auto& c : rhs) {
			for (std::vector<ReactionComponent>::iterator c=rhs->begin();c!=rhs->end();c++) {
				//std::cout << "(" << c.multiplier << "*" << c.species->id << "<" << c.compartment_index << ">) ";
				std::cout << "(" << c->multiplier << "*" << c->species->id << "<" << c->compartment_index << ">) ";
			}
			std::cout << std::endl;
		}
	}
}

void ReactionList::add_reaction(const double rate, const ReactionEquation& eq) {

	//		if (eq.rhs.size() > 0) {
	//			if (eq.lhs.size() > 0) {
	//				std::cout << "adding reaction with lhs = (" << eq.lhs[0].compartment_index << "," << eq.lhs[0].multiplier << ") rhs = (" << eq.rhs[0].compartment_index << "," << eq.rhs[0].multiplier << ")" << std::endl;
	//			} else {
	//				std::cout << "adding reaction with lhs = (null) rhs = (" << eq.rhs[0].compartment_index << "," << eq.rhs[0].multiplier << ")" << std::endl;
	//			}
	//		} else {
	//			std::cout << "adding reaction with lhs = (" << eq.lhs[0].compartment_index << "," << eq.lhs[0].multiplier << ") rhs = (null)" << std::endl;
	//		}
	bool added = false;
	//std::cout <<"added unsorted reaction. size of lhs = "<<lhs.size()<<std::endl;
	ReactionSide sorted_lhs(eq.lhs);
	std::sort(sorted_lhs.begin(),sorted_lhs.end());
	//std::cout <<"added sorted reaction. size of lhs = "<<sorted_lhs.size()<<std::endl;
	//for (auto& r : reactions) {
	for (std::vector<ReactionsWithSameRateAndLHS>::iterator r=reactions.begin();r!=reactions.end();r++) {
		//added |= r.add_if_same_lhs(rate, sorted_lhs, eq.rhs);
		added |= r->add_if_same_lhs(rate, sorted_lhs, eq.rhs);
		if (added) break;
	}
	if (!added) {
		reactions.push_back(ReactionsWithSameRateAndLHS(rate, sorted_lhs, eq.rhs));
		propensities.push_back(0);
        //std::cout << "added reaction with eq = "<<eq<<" compartment "<<eq.rhs[0].compartment_index<<std::endl;
	}
	my_size++;
}

void ReactionList::clear() {
	reactions.clear();
	propensities.clear();
	my_size = 0;
}

double ReactionList::delete_reaction(const ReactionEquation& eq) {
	const int n = reactions.size();
	double ret_rate = 0;
	bool found = false;
	int i,j;
	for (i = 0; i < n; ++i) {
		if (eq.lhs == reactions[i].lhs) {
			const int n_r = reactions[i].all_rhs.size();
			for (j = 0; j < n_r; ++j) {
				found |= (eq.rhs == reactions[i].all_rhs[j]);
				if (found) break;
			}
			if (found) break;
		}
	}
//	if (!found) {
//		std::cerr << "ERROR: could not find reaction to erase!!!" << std::endl;
//		return;
//	}
	if (found) {
		ret_rate = reactions[i].rate;
		reactions[i].all_rhs.erase(reactions[i].all_rhs.begin() + j);
		if (reactions[i].all_rhs.size() == 0) {
			reactions.erase(reactions.begin() + i);
			propensities.erase(propensities.begin()+i);
		}
		my_size--;
	}
	return ret_rate;
}

ReactionEquation ReactionList::pick_random_reaction(const double rand) {
	double last_sum_propensities = 0;
	double sum_propensities = 0;
	const int n = reactions.size();
	const double rand_times_total_propensity = rand*total_propensity;
	for (int i = 0; i < n; i++) {
		sum_propensities += propensities[i];
		if (rand_times_total_propensity < sum_propensities) {
			ASSERT(propensities[i] > 0, "chosen reaction with propensity less than or equal to zero");
			const double scaled_rand = (rand_times_total_propensity-last_sum_propensities)/(sum_propensities-last_sum_propensities);
			return ReactionEquation(reactions[i].lhs,reactions[i].pick_random_rhs(scaled_rand));
		}
		last_sum_propensities = sum_propensities;
	}

	throw std::runtime_error("ERROR: should have picked a reaction. rand is either not 0->1 or total_propensity != sum of propensities!!!!!!");
	// exit(-1);
	//std::cout << "returning reaction with lhs.size() = " << reactions[n].size()<<std::endl;
	//		const double scaled_rand = (rand_times_total_propensity - sum_propensities)/(total_propensity-sum_propensities);
	//		return ReactionEquation(reactions[n].lhs,reactions[n].pick_random_rhs(rand));
}

double ReactionList::recalculate_propensities() {
	total_propensity = 0;
	inv_total_propensity = 0;
	const int n = reactions.size();
	for (int i = 0; i < n; i++) {
		ReactionsWithSameRateAndLHS& rs = reactions[i];
		double& propensity = propensities[i];
		propensity = 1.0;
		int beta = 0;
		//for (auto& rc : rs.lhs) {
		for (std::vector<ReactionComponent>::iterator rc=rs.lhs.begin();rc!=rs.lhs.end();rc++) {
			int copy_number = rc->species->copy_numbers[rc->compartment_index];
			beta += rc->multiplier;
			ASSERT(copy_number >= 0, "copy number is less than zero!!");
			if (copy_number < rc->multiplier) {
				propensity = 0.0;
				break;
			}
			for (int k = 1; k < rc->multiplier; ++k) {
				copy_number *= copy_number-k;
			}
			propensity *= copy_number;
		}
		propensity *= rs.size()*rs.rate;
		ASSERT(propensity >= 0, "calculated propensity is less than zero!!");
		total_propensity += propensity;
		//			if (reactions[i].lhs[0].compartment_index==0) {
		//				std::cout << "reaction with neighbour "<<reactions[i].all_rhs[0][0].compartment_index<<" and num particles = " <<
		//						reactions[i].lhs[0].species->copy_numbers[reactions[i].lhs[0].compartment_index] << " has propensity = "<<propensities[i]<<std::endl;
		//			}
	}
	if (total_propensity != 0) inv_total_propensity = 1.0/total_propensity;
	return inv_total_propensity;
}

static const double LONGEST_TIME = 100000;


NextSubvolumeMethod::NextSubvolumeMethod(StructuredGrid& subvolumes):
		subvolumes(subvolumes),
		time(0) {
	const int n = subvolumes.size();
	//std::cout << "created "<<n<<" subvolumes"<<std::endl;
	heap.clear();
	for (int i = 0; i < n; ++i) {
		subvolume_heap_handles.push_back(heap.push(HeapNode(time + LONGEST_TIME,i,time)));
		subvolume_reactions.push_back(ReactionList());
		saved_subvolume_reactions.push_back(ReactionList());
	}
}

void NextSubvolumeMethod::reset() {
	reset_all_priorities();
}

void  NextSubvolumeMethod::add_reaction(const double rate, ReactionEquation eq) {
	const int n = subvolumes.size();
	for (int i = 0; i < n; ++i) {
		add_reaction_to_compartment(rate,eq,i);
	}
}

void  NextSubvolumeMethod::add_reaction_to_compartment(const double rate, ReactionEquation eq, const int i) {
	eq.lhs.set_compartment_index(i);
	eq.rhs.set_compartment_index(i);
    //std::cout << "adding from compartment "<<eq.lhs[0].compartment_index<<" to compartment "<<eq.rhs[0].compartment_index<<std::endl;
	const int beta = eq.lhs.get_num_reactants();
	if (beta == 0) {
		subvolume_reactions[i].add_reaction(rate*subvolumes.get_cell_volume(i),eq);

	} else if (beta == 1) {
		subvolume_reactions[i].add_reaction(rate,eq);

	} else {
		subvolume_reactions[i].add_reaction(rate*pow(subvolumes.get_cell_volume(i),1-eq.lhs.get_num_reactants()),eq);
	}
	reset_priority(i);
}

Species* NextSubvolumeMethod::get_species(const int id) {
	for (unsigned int i = 0; i < diffusing_species.size(); ++i) {
		if (diffusing_species[i]->id==id) {
			return diffusing_species[i];
		}
	}
	return NULL;
}

void NextSubvolumeMethod::add_diffusion(Species &s, const double rate) {
	if (get_species(s.id) == NULL) {
		diffusing_species.push_back(&s);
	} else {
		return;
	}

	const int n = subvolumes.size();
	for (int i = 0; i < n; ++i) {
		const std::vector<int>& neighbrs = subvolumes.get_neighbour_indicies(i);
		const int nn = neighbrs.size();
		for (int j = 0; j < nn; ++j) {
			ReactionSide lhs;
			lhs.push_back(ReactionComponent(1.0,s,i));
			ReactionSide rhs;
			rhs.push_back(ReactionComponent(1.0,s,neighbrs[j]));
			//				if (i==0) {
			//					std::cout <<"adding reaction with rate = "<<rate<<" and lhs[0].compartment_index = "<<lhs[0].compartment_index<<" and rhs[0].compartment_index"<<rhs[0].compartment_index<<" to the 0th compartment"<<std::endl;
			//				}
			subvolume_reactions[i].add_reaction(rate,ReactionEquation(lhs,rhs));
		}
		reset_priority(i);
	}

}

void NextSubvolumeMethod::add_diffusion(Species &s) {
	if (get_species(s.id) == NULL) {
		diffusing_species.push_back(&s);
	} else {
		return;
	}

	const int n = subvolumes.size();
	for (int i = 0; i < n; ++i) {
		const std::vector<int>& neighbrs = subvolumes.get_neighbour_indicies(i);
		const int nn = neighbrs.size();
		for (int j = 0; j < nn; ++j) {
			const double h = subvolumes.get_distance_between(i,neighbrs[j]);
			const double rate = s.D/pow(h,2);
			ReactionSide lhs;
			lhs.push_back(ReactionComponent(1.0,s,i));
			ReactionSide rhs;
			rhs.push_back(ReactionComponent(1.0,s,neighbrs[j]));
			//				if (i==0) {
			//					std::cout <<"adding reaction with rate = "<<rate<<" and lhs[0].compartment_index = "<<lhs[0].compartment_index<<" and rhs[0].compartment_index"<<rhs[0].compartment_index<<" to the 0th compartment"<<std::endl;
			//				}
			subvolume_reactions[i].add_reaction(rate,ReactionEquation(lhs,rhs));
		}
		reset_priority(i);
	}
}

void NextSubvolumeMethod::add_diffusion_between(Species &s, const double rate, std::vector<int>& from, std::vector<int>& to) {
	ASSERT(from.size() == to.size(), "From and To vectors must be the same length");
	const int n = from.size();
	for (int i = 0; i < n; ++i) {
		ReactionSide lhs;
		lhs.push_back(ReactionComponent(1.0,s,from[i]));
		ReactionSide rhs;
		rhs.push_back(ReactionComponent(1.0,s,to[i]));
		subvolume_reactions[from[i]].add_reaction(rate,ReactionEquation(lhs,rhs));
		reset_priority(i);
	}

}

void NextSubvolumeMethod::reset_all_priorities() {
	const int n = subvolumes.size();
	for (int i = 0; i < n; ++i) {
		reset_priority(i);
	}
}

void NextSubvolumeMethod::reset_priority(const int i) {
	const double inv_total_propensity = subvolume_reactions[i].recalculate_propensities();
	HeapNode& h = *(subvolume_heap_handles[i]);
	if (inv_total_propensity != 0) {
		h.time_at_next_reaction = time - inv_total_propensity*log(randOCD());
	} else {
		h.time_at_next_reaction = time + LONGEST_TIME;
	}
//	if (i==929) {
//		std::cout <<"recalcing priority for 929: time to next react = "<<h.time_at_next_reaction<<" total propensity = "<<1.0/inv_total_propensity<<std::endl;
//	}
	h.time_at_last_random_sample = time;
	heap.update(subvolume_heap_handles[i]);
}

void NextSubvolumeMethod::recalc_priority(const int i) {
	const double old_propensity = subvolume_reactions[i].get_propensity();
	const double inv_total_propensity = subvolume_reactions[i].recalculate_propensities();
	HeapNode& h = *(subvolume_heap_handles[i]);
	if (inv_total_propensity != 0) {
		//const double time_to_next_reaction = -inv_total_propensity*log(uni());
		if (old_propensity == 0) {
			h.time_at_next_reaction = time - inv_total_propensity*log(randOCD());
		} else {
			h.time_at_next_reaction = time - inv_total_propensity*log(randOCD());

			//h.time_at_next_reaction = time + old_propensity*inv_total_propensity*(h.time_at_next_reaction - h.time_at_last_random_sample);
		}
	} else {
		h.time_at_next_reaction = time + LONGEST_TIME;
	}
//	if (i==929) {
//		std::cout <<"recalcing priority for 929: time to next react = "<<h.time_at_next_reaction<<" total propensity = "<<1.0/inv_total_propensity<<std::endl;
//	}
	h.time_at_last_random_sample = time;
	heap.update(subvolume_heap_handles[i]);
}

void NextSubvolumeMethod::operator ()(const double dt) {
	const double final_time = time + dt;
	while (get_next_event_time() < final_time) {
		const int sv_i = heap.top().subvolume_index;
		time = heap.top().time_at_next_reaction;
		//std::cout << "dealing with subvolume with time = " << time << " and index = " << sv_i << std::endl;
		const double rand = randCOD();
		ReactionEquation r = subvolume_reactions[sv_i].pick_random_reaction(rand);
		//std::cout << "reacting: " << r<<" "<<r.lhs[0].compartment_index<<" " <<r.rhs[0].compartment_index<<std::endl;
		//std::cout << "rand: " << rand<<std::endl;

		react(r);
	}
	time = final_time;
}

void NextSubvolumeMethod::list_reactions() {
	const int n = subvolumes.size();
	for (int i = 0; i < n; ++i) {
		std::cout << "Compartment " << i << " has the following reactions:" <<std::endl;
		subvolume_reactions[i].list_reactions();
	}
}

void NextSubvolumeMethod::set_interface_reactions(
		std::vector<int>& from_indicies,
		std::vector<int>& to_indicies,
        int id,
		const double dt,
		const bool corrected) {

	//std::cout << "setting interface from x["<<from_indicies.size()<<"] to y["<<to_indicies.size()<<"] with dt = "<<dt<<std::endl;
//	for (unsigned int i = 0; i < from_indicies.size(); ++i) {
//		std::cout << "\tfrom "<<from_indicies[i] << " to "<<to_indicies[i]<<std::endl;
//	}
	const unsigned int n = from_indicies.size();
	ASSERT(n==to_indicies.size(),"from and to indicies vectors have different size");
	/*
	 * update diffusion reaction rates for neighbouring cells
	 */

    Species *s_ptr = get_species(id);
	if (s_ptr != NULL) {
		Species& s = *s_ptr;
		for (unsigned int ii = 0; ii < n; ++ii) {
			const int i = from_indicies[ii];
			const int j = to_indicies[ii];
			ReactionSide lhs;
			lhs.push_back(ReactionComponent(1.0,s,i));
			ReactionSide rhs;
			rhs.push_back(ReactionComponent(1.0,s,j));
			rhs[0].tmp = std::sqrt(2.0*s.D*dt);
			double rate = subvolume_reactions[i].delete_reaction(ReactionEquation(lhs,rhs));
			if (rate != 0) {
				//rate *= 2.0*subvolumes.get_distance_between(i,j)/sqrt(PI*s.D*dt);
				const double h = subvolumes.get_distance_between(i,j);
				if (corrected) {
					rate *= 2.0*h/sqrt(PI*s.D*dt);				// two regime method
				} else {
					rate *= h/sqrt(PI*s.D*dt);
				}
				//std::cout << "new interface rate = rate * 2*"<<subvolumes.get_distance_between(i,j)<<" div sqrt(pi*d*dt)"<<std::endl;
				//rate *= 0.5;
				rhs[0].compartment_index = -j;
				subvolume_reactions[i].add_reaction(rate,ReactionEquation(lhs,rhs));
				reset_priority(i);
			}
//			std::cout << "reactions for i,j = "<<i<<","<<j<<std::endl;
//			subvolume_reactions[i].list_reactions();
		}
	}
}

void NextSubvolumeMethod::unset_interface_reactions(
		std::vector<int>& from_indicies,
		std::vector<int>& to_indicies) {
	/*
	 * update diffusion reaction rates for neighbouring cells
	 */
	const unsigned int n = from_indicies.size();
	ASSERT(n==to_indicies.size(),"from and to indicies vectors have different size");

	const unsigned int ns = diffusing_species.size();
	for (unsigned int is = 0; is < ns; ++is) {
		Species& s = *diffusing_species[is];
		for (unsigned int ii = 0; ii < n; ++ii) {
			const int i = from_indicies[ii];
			const int j = to_indicies[ii];
			ReactionSide lhs;
			lhs.push_back(ReactionComponent(1.0,s,i));
			ReactionSide rhs;
			rhs.push_back(ReactionComponent(1.0,s,-j));
			double rate = subvolume_reactions[i].delete_reaction(ReactionEquation(lhs,rhs));
			if (rate != 0) {
				rate = s.D*subvolumes.get_laplace_coefficient(i,j);
				if (rate != 0) {
					rhs[0].compartment_index = j;
					subvolume_reactions[i].add_reaction(rate,ReactionEquation(lhs,rhs));
				}
				reset_priority(i);
			}
		}
	}
}


void NextSubvolumeMethod::react(ReactionEquation& eq) {
	//for (auto& rc : eq.lhs) {
	for (std::vector<ReactionComponent>::iterator rc=eq.lhs.begin();rc!=eq.lhs.end();rc++) {
		//rc.species->copy_numbers[rc.compartment_index] -= rc.multiplier;
		rc->species->copy_numbers[rc->compartment_index] -= rc->multiplier;
//		if ((rc.compartment_index==0)||(rc.compartment_index==560)) {
//			print = true;
//			std::cout<<" compartment "<<rc.compartment_index<<" changed to have "<<rc.species->copy_numbers[rc.compartment_index]<<" molecules"<<std::endl;
//		}
	}
	//for (auto& rc : eq.rhs) {
	for (std::vector<ReactionComponent>::iterator rc=eq.rhs.begin();rc!=eq.rhs.end();rc++) {
        //std::cout << "compartment index = "<<rc->compartment_index<<std::endl;
		if (rc->compartment_index < 0) {
            // test if this reaction is within the compartment and generates a particle
            if (
                (eq.lhs[0].compartment_index == -rc->compartment_index) || 
                ((eq.lhs[0].compartment_index==0) && (eq.rhs[0].compartment_index == -std::numeric_limits<int>::max()))  
               ) {
                for (int i=0; i<rc->multiplier; i++) {
                    Vect3d newr = get_grid().get_random_point(-rc->compartment_index);
                    rc->species->particles.push_back(newr);
				    rc->species->particlesx.push_back(newr);
                }
            // else must be an interface reaction
            } else {

//			const double step_length = rc.tmp;
//			const double dt = pow(step_length,2)/(2.0*rc.species->D);
//			const double kappa = 0.86*step_length/dt;
//			const double h = subvolumes.get_distance_between(eq.lhs[0].compartment_index,-rc.compartment_index);
//			const double P1 = kappa*h/rc.species->D;
//			std::cout << "absorbing with P1 = "<<P1<<std::endl;
//			if (uni() < P1) {
				Rectangle r = subvolumes.get_face_between(eq.lhs[0].compartment_index,-rc->compartment_index);
				Vect3d newr,newn;
				r.get_random_point_and_normal_triangle(newr, newn);
				const double P = randCCD();
				const double P2 = pow(P,2);
				const double step_length = rc->tmp;
				const double dist_from_intersect = step_length*(0.729614*P - 0.70252*P2)/(1.0 - 1.47494*P + 0.484371*P2);
				newr += newn*dist_from_intersect;
				rc->species->particles.push_back(newr);
				rc->species->particlesx.push_back(subvolumes.get_cell_centre(eq.lhs[0].compartment_index));
//			} else {
//				rc.species->copy_numbers[rc.compartment_index] += rc.multiplier;
//			}
            }
		} else {
			rc->species->copy_numbers[rc->compartment_index] += rc->multiplier;
		}
//		if ((rc.compartment_index==0)||(rc.compartment_index==560)) {
//			std::cout<<" compartment "<<rc.compartment_index<<" changed to have "<<rc.species->copy_numbers[rc.compartment_index]<<" molecules"<<std::endl;
//		}
	}
	if (eq.lhs.size() == 0) {
		// must be zeroth order rection
		ASSERT(eq.rhs.size() > 0,"empty equation, cannot react");
		reset_priority(eq.rhs[0].compartment_index);
	} else {
		reset_priority(eq.lhs[0].compartment_index);
		if ((eq.rhs.size() == 1) && (eq.rhs[0].compartment_index >= 0) && (eq.lhs[0].compartment_index != eq.rhs[0].compartment_index)) {
			//diffusion reaction
			recalc_priority(eq.rhs[0].compartment_index);
		}
	}


}


std::ostream& operator<< (std::ostream& out, NextSubvolumeMethod &b) {
	out << "\tNext Subvolume Method:"<<std::endl;
	out << "\t\tStructured Grid:"<<std::endl;
	out << "\t\t\tlow = "<<b.get_grid().get_low() << " high = "<<b.get_grid().get_high()<<std::endl;
	out << "\t\t\tcompartment sizes = "<<b.get_grid().get_cell_size() << std::endl;
	out << "\t\tDiffusing Species:"<<std::endl;
	for (unsigned int i = 0; i < b.get_diffusing_species().size(); ++i) {
		Species *s = b.get_diffusing_species()[i];
		out <<"\t\t\tSpecies "<<s->id<<" (D = "<<s->D<<") has "<<
					std::accumulate(s->copy_numbers.begin(),s->copy_numbers.end(),0)<<
					" particles in compartments and "<<s->particles.size()<<" off-lattice particles"<<std::endl;
	}
	return out;
}


}
