/* 
 * nsvc.cpp
 *
 * Copyright 2012 Martin Robinson
 *
 * This file is part of Smoldyn.
 *
 * Smoldyn is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Smoldyn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Smoldyn.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: Apr 16, 2013
 *      Author: mrobins
 */

#define NSVC_CPP
#include "nsvc.h"
#include "smoldynfuncs.h"
#include <sstream>
#include <set>
#include <numeric>

void nsv_init() {
	Kairos::init(0,NULL);
}

NextSubvolumeMethod* nsv_new(double* min, double* max, double* dx, int n) {
	using namespace Kairos;
	Vect3d min_vect(0.0,0.0,0.0);
	Vect3d max_vect(1.0,1.0,1.0);
	Vect3d dx_vect(1.0,1.0,1.0);
	for (int i = 0; i < n; ++i) {
		min_vect[i] = min[i];
		max_vect[i] = max[i];
		dx_vect[i] = dx[i];
	}
	StructuredGrid *grid = new StructuredGrid(min_vect,max_vect,dx_vect);
	NextSubvolumeMethod* nsv = new NextSubvolumeMethod(*grid);

	return nsv;
}

void nsv_delete(NextSubvolumeMethod* nsv) {
	if (!nsv) return;
	delete &nsv->get_grid();
	delete nsv;
}

void nsv_print(NextSubvolumeMethod* nsv, char** bufferptr) {
	int length;
	std::ostringstream ss;
	char *buffer;

	ss << std::endl << *nsv << std::endl;
	length=ss.str().length();
	buffer=(char*) calloc(length,sizeof(char));
	if(!buffer) return;
	ss.str().copy(buffer,length,0);
	buffer[length-1] = '\0';
	*bufferptr=buffer;
	return;
}


void nsv_add_interface(NextSubvolumeMethod* nsv,int id,double dt, double *start,double *end,double *norm,int dim) {
	using namespace Kairos;
	Vect3d min(0.0,0.0,0.0);
	Vect3d max(1.0,1.0,1.0);
	for (int i = 0; i < dim; ++i) {
		min[i] = start[i];
		max[i] = end[i];
	}
	if ((norm[0] == 1) || (norm[0] == -1)) {
		xrect interface(min,max,norm[0]);
		nsv->set_interface(interface,id,dt,false);

	} else if ((norm[1] == 1) || (norm[1] == -1)) {
		yrect interface(min,max,norm[1]);
		nsv->set_interface(interface,id,dt,false);

	} else if ((norm[2] == 1) || (norm[2] == -1)) {
		zrect interface(min,max,norm[2]);
		nsv->set_interface(interface,id,dt,false);

	}
}


void nsv_add_species(NextSubvolumeMethod* nsv,int id,double D,char *btype,int dim) {
	using namespace Kairos;

	//TODO: assumes that species has not been added before
	Species *ns = new Species(id,D,&nsv->get_grid());
	nsv->add_diffusion(*ns);

	if (dim > 0) {
		xplane xlow(nsv->get_grid().get_low()[0],1);
		xplane xhigh(nsv->get_grid().get_high()[0],-1);
		if (btype[0]=='p') {

			const double hx = nsv->get_grid().get_cell_size()[0];
			nsv->add_diffusion_between(*ns,ns->D/pow(hx,2),xlow,xhigh);
			nsv->add_diffusion_between(*ns,ns->D/pow(hx,2),xhigh,xlow);
		}
		if (dim > 1) {
			yplane ylow(nsv->get_grid().get_low()[1],1);
			yplane yhigh(nsv->get_grid().get_high()[1],-1);
			if (btype[1]=='p') {

				const double hy = nsv->get_grid().get_cell_size()[1];
				nsv->add_diffusion_between(*ns,ns->D/pow(hy,2),ylow,yhigh);
				nsv->add_diffusion_between(*ns,ns->D/pow(hy,2),yhigh,ylow);
			}
			if (dim > 2) {
				zplane zlow(nsv->get_grid().get_low()[2],1);
				zplane zhigh(nsv->get_grid().get_high()[2],-1);
				if (btype[2]=='p') {

					const double hz = nsv->get_grid().get_cell_size()[2];
					nsv->add_diffusion_between(*ns,ns->D/pow(hz,2),zlow,zhigh);
					nsv->add_diffusion_between(*ns,ns->D/pow(hz,2),zhigh,zlow);
				}
			}
		}
	}
}

class SmoldynSurface {
public:
	SmoldynSurface(surfaceptr s, enum PanelFace face):s(s),face(face) {}
	bool lineXsurface(const Kairos::Vect3d& p1, const Kairos::Vect3d& p2) const {
		if (face==PFnone) return false;
		double crsspt[3];
		double p1t[3],p2t[3];
		enum PanelFace face1,face2;
		p1t[0] = p1[0];
		p1t[1] = p1[1];
		p1t[2] = p1[2];
		p2t[0] = p2[0];
		p2t[1] = p2[1];
		p2t[2] = p2[2];
		for (int i = 0; i < PSMAX; ++i) {
			for (int j = 0; j < s->npanel[i]; ++j) {
				if (lineXpanel(p1t,p2t,s->panels[i][j],s->srfss->sim->dim,crsspt,&face1,&face2,NULL,NULL,NULL,0)==1) {
					if ((face1!=face2) && ((face==PFboth)||(face1==face))) return true;
				}
			}
		}
		return false;
	}


private:
	surfaceptr s;
	enum PanelFace face;
};


class SmoldynCompartment {
public:
	SmoldynCompartment(compartptr c):c(c) {}
	bool is_in(const Kairos::Vect3d& test_point) const {
		double pt[3];
		pt[0] = test_point[0];
		pt[1] = test_point[1];
		pt[2] = test_point[2];
		return posincompart(c->cmptss->sim,pt,c,0)==1;
	}
private:
	compartptr c;
};

extern void nsv_add_surface(NextSubvolumeMethod* nsv,surfacestruct* surface) {
	using namespace Kairos;
	const int ns = nsv->get_diffusing_species().size();
	//std::cout << "adding surface "<<surface->sname<<std::endl;
	for (int i = 0; i < ns; ++i) {
		enum PanelFace face;
		int id = nsv->get_diffusing_species()[i]->id;
		if ((surface->action[id][0][PFfront]==SAreflect) &&
				(surface->action[id][0][PFback]==SAreflect)) face = PFboth;
		else if (surface->action[id][0][PFfront]==SAreflect) face = PFfront;
		else if (surface->action[id][0][PFback]==SAreflect) face = PFback;
		else face = PFnone;
		if (face != PFnone) {
			//std::cout << "finished reflective surface with face="<<face<<std::endl;
			SmoldynSurface s(surface,face);
			nsv->scale_diffusion_across(*(nsv->get_diffusing_species()[i]),s,0);
		}

		if ((surface->action[id][0][PFfront]==SAabsorb) &&
				(surface->action[id][0][PFback]==SAabsorb)) face = PFboth;
		else if (surface->action[id][0][PFfront]==SAabsorb) face = PFfront;
		else if (surface->action[id][0][PFback]==SAabsorb) face = PFback;
		else face = PFnone;
		if (face != PFnone) {
			//std::cout << "finished absorption surface with face="<<face<<std::endl;
			SmoldynSurface s(surface,face);
			nsv->absorption_across(*(nsv->get_diffusing_species()[i]),s,1);
		}
	}
	//std::cout << "finished adding surface "<<surface->sname<<std::endl;

}


void nsv_add_reaction(NextSubvolumeMethod* nsv,rxnstruct *reaction) {
	const double rate = reaction->rate;
	const int nreactants = reaction->rxnss->order;
	const int *reactant_ids = reaction->rctident;
	const int nproducts = reaction->nprod;
	const int *product_ids = reaction->prdident;


	using namespace Kairos;


	ReactionSide lhs;
	for (int i = 0; i < nreactants; ++i) {
		Species* s = nsv->get_species(reactant_ids[i]);
		bool found = false;
		for (std::vector<ReactionComponent>::iterator rc=lhs.begin();rc!=lhs.end();rc++) {
			if (rc->species==s) {
				rc->multiplier++;
				found = true;
			}
		}
		if (!found) {
			lhs = lhs + *(nsv->get_species(reactant_ids[i]));
		}
	}
	ReactionSide rhs;
	for (int i = 0; i < nproducts; ++i) {
		Species* s = nsv->get_species(product_ids[i]);

        bool to_particle = false;
        if (reaction->prdrep != NULL) to_particle = (reaction->prdrep[i] == SRparticle);

		bool found = false;
		for (std::vector<ReactionComponent>::iterator rc=rhs.begin();rc!=rhs.end();rc++) {
            bool rc_to_particle = (rc->compartment_index == -1);
			if ((rc->species==s) && (to_particle == rc_to_particle)) {
				rc->multiplier++;
				found = true;
			}
		}
		if (!found) {
			rhs = rhs + *(nsv->get_species(product_ids[i]));
            if (to_particle) {
//                std::cout << "adding a t0_paricle reaction "<<rhs<<std::endl;
                (rhs.end()-1)->compartment_index = -1;
            } else {
//                std::cout << "adding a normal reaction"<<std::endl;
                (rhs.end()-1)->compartment_index = 1;
            }
		}
	}

	if (reaction->srf) {
		enum PanelFace face = PFboth;
		SmoldynSurface surface(reaction->srf,face);
		nsv->add_reaction_on(rate, lhs >> rhs, surface);
	} else if (reaction->cmpt) {
		SmoldynCompartment compartment(reaction->cmpt);
		nsv->add_reaction_in(rate, lhs >> rhs, compartment);
	} else {
		nsv->add_reaction(rate, lhs >> rhs);
	}
}


//void nsv_add_reaction(NextSubvolumeMethod* nsv,double rate,
//						int nreactants,int *reactant_ids,
//						int nproducts,int *product_ids) {
//
//	//TODO: assumes that species has already been added via nsv_add_species
//	using namespace Kairos;
//
//
//	ReactionSide lhs;
//	for (int i = 0; i < nreactants; ++i) {
//		lhs = lhs + *(nsv->get_species(reactant_ids[i]));
//	}
//	ReactionSide rhs;
//	for (int i = 0; i < nproducts; ++i) {
//		rhs = rhs + *(nsv->get_species(product_ids[i]));
//	}
//	nsv->add_reaction(rate, lhs >> rhs);
//}

void nsv_integrate(NextSubvolumeMethod* nsv,double dt, portstruct *port, latticestruct *lattice) {
	using namespace Kairos;
	//std::cout << "running lattice dt"<<std::endl;
	nsv->integrate(dt);
		const int ns = nsv->get_diffusing_species().size();
	int n = 0;
	for (int i = 0; i < ns; ++i) {
		n += nsv->get_diffusing_species()[i]->particles.size();
	}

	int *species = new int[n];
	double **positions = new double*[n];
	double **positionsx = new double*[n];

	//const double lattice_lengthscale = 0.0001*std::min(lattice->dx[0],std::min(lattice->dx[1],lattice->dx[2]));
    if (!port) return;

	simptr sim = port->portss->sim;
	n += sim->mols->nl[port->llport];
	double *crsspt = new double[sim->dim];
	enum PanelFace face1,face2;

	/*
	 * look in port for new particles
	 */
	std::set<int> dirty_indicies;
	int nout = 0;
	n = sim->mols->nl[port->llport];
	for (int i = 0; i < n; ++i) {
		moleculeptr m = sim->mols->live[port->llport][i];

//		//process other surface interactions
//		int er = checksurfaces1mol(sim,m);
//		if (er != 0) simLog(NULL,11,"ERROR: failure in nsv_integrate (while processing surface interactions)\n");
//
//		//check if already gone back through port
//
//		bool in = true;
//		for (int j = 0; j < port->srf->npanel[PSrect]; ++j) {
//			if (lineXpanel(m->posx,m->pos,port->srf->panels[PSrect][j],sim->dim,crsspt,&face1,&face2,NULL,NULL,NULL,0)) {
//				if ((face1!=face2) && (face2==PFfront)) {
//					in = false;
//				}
//			}
//		}
//
//		if (!in) {
//			//std::cout <<" particle going back through port!!!"<<std::endl;
//			species[nout] = m->ident;
//			positions[nout] = m->pos;
//			positionsx[nout] = m->via;
//			nout++;
//			continue;
//		}


		// if outside lattice domain raise error
		Vect3d newr(0.5,0.5,0.5);
		for (int d = 0; d < sim->dim; ++d) {
			newr[d] = m->via[d];
			double low = nsv->get_grid().get_low()[d];
			double high = nsv->get_grid().get_high()[d];
			if (newr[d] < low) {
				std::cout << "d = "<<d<<" via = "<<m->via[d]<<" pos = "<<m->pos[d]<<" posx = "<<m->posx[d]<<" newr = "<<newr[d]<<std::endl;
				simLog(NULL,11,"ERROR: particle unexpectedly outside lattice domain\n");
			} else if (newr[d] > high) {
				std::cout << "d = "<<d<<" via = "<<m->via[d]<<" pos = "<<m->pos[d]<<" posx = "<<m->posx[d]<<" newr = "<<newr[d]<<std::endl;
				simLog(NULL,11,"ERROR: particle unexpectedly outside lattice domain\n");
			}
		}

		const int ci = nsv->get_grid().get_cell_index(newr);
		Vect3d cc = nsv->get_grid().get_cell_centre(ci);

		bool throw_back = false;

		for (int i = 0; i < lattice->nsurfaces; ++i) {
			if ((lattice->surfacelist[i]->action[m->ident][0][PFfront] != SAreflect) && (lattice->surfacelist[i]->action[m->ident][0][PFback] != SAreflect)) {
				continue;
			}
			for (int j = 0; j < PSMAX; ++j) {
				for (int k = 0; k < lattice->surfacelist[i]->npanel[j]; ++k) {
					if (lineXpanel(m->posx,cc.data(),lattice->surfacelist[i]->panels[j][k],port->portss->sim->dim,crsspt,&face1,&face2,NULL,NULL,NULL,0)==1) {
						//if particle crossed surface when adding to lattice, throw back through port
						if ((face1!=face2) && (lattice->surfacelist[i]->action[m->ident][0][face1]==SAreflect)) throw_back = true;

					}
				}
			}
		}


		if (throw_back) {
			//std::cout << "throwing back particle at posx = ("<<m->posx[0]<<','<<m->posx[1]<<','<<m->posx[2]<<") and pos = ("<<m->pos[0]<<','<<m->pos[1]<<','<<m->pos[2]<<")"<<std::endl;
			species[nout] = m->ident;
			positions[nout] = m->posx;
			positionsx[nout] = m->posx;
			nout++;
		} else {
			nsv->get_species(m->ident)->copy_numbers[ci]++;
			dirty_indicies.insert(ci);
		}
	}
	delete[] crsspt;


	//delete particles in port
	int er = portgetmols(sim,port,-1,MSall,1);
	if (er != n) simLog(NULL,11,"ERROR: failure in nsv_integrate (while deleting particles from port)\n");


	/*
	 * put new particles in port
	 */
	for (int i = 0; i < ns; ++i) {
		Species *s = nsv->get_diffusing_species()[i];
		const int np = s->particles.size();
		double *crsspt = new double[sim->dim];
		enum PanelFace face1,face2;
		for (int j = 0; j < np; ++j) {
			//check if already gone back through port
			bool in = true;
            if (port->srf) {
                for (int k = 0; k < port->srf->npanel[PSrect]; ++k) {
                    if (lineXpanel(s->particlesx[j].data(),s->particles[j].data(),port->srf->panels[PSrect][k],sim->dim,crsspt,&face1,&face2,NULL,NULL,NULL,0)) {
                        if ((face1!=face2) && (port->srf->action[s->id][0][face1]==SAport)) {
                            in = false;
                        }
                    }
                }
            }

			if (in) {
				//std::cout <<" particle going back through port!!!"<<std::endl;
				species[nout] = s->id;
				positions[nout] = s->particles[j].data();
//				if (positions[nout][0] <= -10) std::cout << "x <= -10: "<<positions[nout]<<std::endl;
//				if (positions[nout][0] >= 10) std::cout << "x >= -10: "<<positions[nout]<<std::endl;
//				if (positions[nout][1] <= -10) std::cout << "x <= -10: "<<positions[nout]<<std::endl;
//				if (positions[nout][1] >= 10) std::cout << "x >= -10: "<<positions[nout]<<std::endl;
				positionsx[nout] = s->particlesx[j].data();
				nout++;
			} else {
				//add to lattice
				//std::cout << "adding particle of species "<<s->id<<" back into lattice at position "<<s->particles[j]<<std::endl;
				const int ci = nsv->get_grid().get_cell_index(s->particles[j]);
				//Vect3d cc = nsv->get_grid().get_cell_centre(ci);
				nsv->get_species(s->id)->copy_numbers[ci]++;
				dirty_indicies.insert(ci);
			}

		}
		s->particles.clear();
		s->particlesx.clear();
	}

	for (std::set<int>::iterator i=dirty_indicies.begin();i!=dirty_indicies.end();i++) {
		nsv->recalc_priority(*i);
	}

	//put particles back in port if needed
	if (nout > 0) {
		er = portputmols(sim,port,nout,species[0],species,positions,positionsx);
		if (er != 0) simLog(NULL,11,"ERROR: failure in nsv_integrate (while putting particles in the port)\n");
	}

	delete[] species;
	delete[] positions;
}

vtkUnstructuredGrid* nsv_get_grid(NextSubvolumeMethod* nsv) {
	using namespace Kairos;
#if defined(HAVE_VTK)
	return get_vtk_grid(nsv->get_grid(),nsv->get_diffusing_species());
#endif
	return NULL;
}

void nsv_molcountspace(NextSubvolumeMethod* nsv,int id, double *low, double *high, int dim, int nbins, int axis, int *ret_array) {
	using namespace Kairos;

	Vect3d vlow(0,0,0);
	Vect3d vhigh(1,1,1);
	Vect3d grid_size(1,1,1);

	for (int i = 0; i < dim; ++i) {
		vlow[i] = low[i];
		vhigh[i] = high[i];
		grid_size[i] = high[i] - low[i];
	}
	if (nbins > 1) {
		grid_size[axis] = (high[axis] - low[axis])/nbins;
	}

	StructuredGrid grid(vlow,vhigh,grid_size);

	std::vector<double> concentration;

	nsv->get_species(id)->get_concentration(grid,concentration);

	for (int i = 0; i < nbins; ++i) {
		ret_array[i] = concentration[i]*grid.get_cell_volume(i);
	}
}

/*
 * returns the concentration of species id at point
 */
double nsv_concentration_point(NextSubvolumeMethod* nsv,int id, double *point, int dim) {
	using namespace Kairos;
	Vect3d vpoint(0,0,0);
	for (int i = 0; i < dim; ++i) {
		vpoint[i] = point[i];
	}
	const Species* s = nsv->get_species(id);
	const int index = nsv->get_grid().get_cell_index(vpoint);
	return s->copy_numbers[index]/nsv->get_grid().get_cell_volume(index);
}

void nsv_molcount(NextSubvolumeMethod* nsv, int *ret_array) {
	using namespace Kairos;

	std::vector<Species*> species = nsv->get_diffusing_species();
	for (unsigned int i = 0; i < species.size(); ++i) {
		const int n = std::accumulate(species[i]->copy_numbers.begin(),species[i]->copy_numbers.end(),0) +
						species[i]->particles.size();
		ret_array[species[i]->id] = n;
	}
}

/*
 * kills one molecule of species id at point
 */
void nsv_kill_molecule(NextSubvolumeMethod* nsv,int id, double *point, int dim) {
	using namespace Kairos;
	Vect3d vpoint(0,0,0);
	for (int i = 0; i < dim; ++i) {
		vpoint[i] = point[i];
	}
	Species* s = nsv->get_species(id);
	const int index = nsv->get_grid().get_cell_index(vpoint);
    s->copy_numbers[index]--;
    if (s->copy_numbers[index] < 0) {
        simLog(NULL,11,"ERROR: lattice species became less than zero (in nsv_kill_molecule)\n");
    }
	nsv->recalc_priority(index);
}


void nsv_add_mol(NextSubvolumeMethod* nsv,int id, double* pos, int dim) {
	using namespace Kairos;
	Vect3d newr(0.5,0.5,0.5);
	// if outside lattice domain raise error
	for (int d = 0; d < dim; ++d) {
		double low = nsv->get_grid().get_low()[d];
		double high = nsv->get_grid().get_high()[d];
		if (pos[d] < low) {
			simLog(NULL,11,"ERROR: particle unexpectedly outside lattice domain\n");
		} else if (pos[d] > high) {
			simLog(NULL,11,"ERROR: particle unexpectedly outside lattice domain\n");
		} else {
			newr[d] = pos[d];
		}
	}
	//nsv->get_species(id)->particles.push_back(newr);

	const int ci = nsv->get_grid().get_cell_index(newr);
	nsv->get_species(id)->copy_numbers[ci]++;
	nsv->recalc_priority(ci);
}

/*
 * returns vector of copy numbers for each lattice site and positions of lattice sites
 *
 * return value = number of lattice sites returned for species "id"
 * copy_numbers = array of ints representing copy number of species "id" at each lattice site
 * 					equals NULL if species does not exist
 * positions = array of doubles representing positions of each site. In lattice-site major order.
 * 		eg. x-coord of lattice site n is positions[3*n+0]
 */
int nsv_get_species_copy_numbers(NextSubvolumeMethod* nsv, int id,const int **copy_numbers, const double** positions) {
	using namespace Kairos;
	int num_lattice_sites;

	Species* s = nsv->get_species(id);
	if (s==NULL) {
		*copy_numbers = NULL;
		num_lattice_sites = 0;
	} else {
		*copy_numbers = &(s->copy_numbers[0]);
		num_lattice_sites = s->copy_numbers.size();
		//*positions = s->grid->get_raw_positions().data();
		*positions = &(s->grid->get_raw_positions()[0]);
	}
	return num_lattice_sites;
}

