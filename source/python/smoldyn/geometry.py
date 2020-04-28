# -*- coding: utf-8 -*-

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"


from . import _smoldyn


'''
    enum ErrorCode smolSetBoundaryType(simptr sim,int dimension,int highside,char type);
    Sets the molecule interaction properties for a system boundary that bounds the dimension axis. Enter dimension as -1 to indicate all dimensions. Set highside to 0 for the lower boundary, to 1 for the upper boundary, and to -1 for both boundaries. The boundary type is entered in type as ‘r’ for reflecting, ‘p’ for periodic, ‘a’ for absorbing, or ‘t’ for transmitting. Note that Smoldyn only observes these properties if no surfaces are declared; otherwise all boundaries are transmitting regardless of what’s entered here.

    int smolAddSurface(simptr sim,char *surface);
    Adds a surface called surface to the system.

    int smolGetSurfaceIndex(simptr sim,char *surface);
    Returns the surface index that corresponds to the surface named surface. The index is non-negative. On failure, this returns an error code cast as an integer.

    char* smolGetSurfaceName(simptr sim,int surfaceindex,char *surface);
    Returns the surface name for surface number surfaceindex both directly and in the surface string. On failure, this returns NULL.

    enum ErrorCode smolSetSurfaceAction(simptr sim,char *surface,enum PanelFace face,char *species,enum MolecState state,enum SrfAction action);
    Sets the action that should happen when a molecule of species species (may be “all”) and state state (may be MSall) diffuses into face face (may be PFboth) of surface surface. The action is set to action.

    enum ErrorCode smolSetSurfaceRate(simptr sim,char *surface,char *species,enum MolecState state,enum MolecState state1,enum MolecState state2,double rate,char *newspecies,int isinternal);
    Sets the surface interaction rate(s) for surface surface (may be “all”) and species species (may be “all”) and state state. The transition being considered is from state1 to state2 (this function uses the tri-state format for describing surface interactions, shown below). The interaction rate is set to rate, which is interpreted as a probability value for internal use if isinternal is 1 and as a physical interaction coefficient if isinternal is 0. If the molecule ends up interacting with the surface, it changes to new species newspecies. Enter newspecies as either NULL or an empty string to indicate that molecules should not change species upon interactions. The molecule states are most easily understood with the following table. If the action listed in the table is in italics, then the corresponding combination of states is not a permitted input.
    				
    interaction class 		action 		
    	state 	state1 	state2 	
    	soln 	soln 	soln 	reflect
    	“ 	“ 	bsoln 	transmit
    collision from 	“ 	“ 	bound 	adsorb
    solution state 	“ 	bsoln 	soln 	transmit
    	“ 	“ 	bsoln 	reflect
    	“ 	“ 	bound 	adsorb
    	“ 	bound 	soln 	desorb
    action from 	“ 	“ 	bsoln 	desorb
    bound state 	“ 	“ 	bound 	no change
    	“ 	“ 	bound’ 	flip
    	bound 	soln 	soln 	reflect
    	“ 	“ 	bsoln 	transmit
    	“ 	“ 	bound 	hop
    collision from 	“ 	“ 	bound’ 	hop
    bound state 	“ 	bsoln 	soln 	transmit
    	“ 	“ 	bsoln 	reflect
    	“ 	“ 	bound 	hop
    	“ 	“ 	bound’ 	hop
    	“ 	bound 	soln 	desorb
    action from 	“ 	“ 	bsoln 	desorb
    bound state 	“ 	“ 	bound 	no change
    	“ 	“ 	bound’ 	flip
    impossible 	“ 	bound’ 	any 	nonsense

    int
    smolAddPanel(simptr sim,char *surface,enum PanelShape panelshape,char *panel,char *axisstring,double *params);
    Adds or modifies a panel of shape panelshape of surface surface. axisstring lists any text parameters for the panel, which in practice is only a single word that gives the orientation of a rectangle panel (e.g. “+0” or “-y”). params lists the numerical parameters for the panel location, size, and drawing characteristics. These are exactly the same parameters that are listed for the “panel” statement in Smoldyn configuration files, with the sole exception that the first rectangle “parameter” is actually a string that is entered in axisstring. panelname is an optional parameter for naming the panel; if it is included and is not an empty string, the panel is named panelname. If this panel name was already used by a panel of the same shape, then this function overwrites that panel’s data with the new data. If the name was already used by a panel with a different shape, then this creates an error, and if the name was not used before, then a new panel is created. To use default panel naming, send in panelname as either NULL or as an empty string. In the latter case, panelname is returned with the newly assigned default name.

    int
    smolGetPanelIndex(simptr sim,char *surface,enum PanelShape *panelshapeptr,char *panel);
    Returns the panel index for the panel called panel on surface surface. If panelshapeptr is not NULL, this also returns the panel shape in panelshapeptr. On failure, this returns the error code cast as an integer.

    char*
    smolGetPanelName(simptr sim,char *surface,enum PanelShape panelshape,int panelindex,char *panel);
    Returns the name of the panel that is in surface surface, has shape panelshape, and has index panelindex, both directly and in the string panel. On failure, this returns NULL.

    enum ErrorCode
    smolSetPanelJump(simptr sim,const char *surface,const char *panel1,enum PanelFace face1,const char *panel2,enum PanelFace face2,int isbidirectional);
    Sets a jumping link between face face1 of panel panel1 and face face2 of panel panel2 of surface surface. The link goes from panel1 to panel2 if bidirectional is entered as 0 and goes in both directions if bidirectional is entered as 1. None of the surface, panel, or face entries is allowed to be “all”. This does not set the actions of any species to “jump”, which has to be done using the smolSetSurfaceAction function.

    enum ErrorCode
    smolAddSurfaceUnboundedEmitter(simptr sim,const char *surface,enum PanelFace face,const char *species,double emitamount,double *emitposition);
    Adds information about a point molecular source so that face face of surface surface can have its absorption properties calculated so that the molecular concentrations will become the same as they would be if the surface weren’t there at all. The point molecular source emits molecules of species species, with a rate of emitamount and is at location emitposition. The emission rate does not need to be in absolute units, but only has to be correct relative to other unbounded emitters. None of the inputs to this function are allowed to be “all”.

    enum ErrorCode
    smolSetSurfaceSimParams(simptr sim,const char *parameter,double value);
    Sets the surface simulation parameter named with parameter to value value. The possible parameters are “epsilon”, “margin”, and “neighbordist”. In all cases, the defaults are nearly always good, although this function allows them to be modified if desired. Epsilon is the maximum distance away from a surface that Smoldyn is allowed to place a surface-bound molecule. Margin is the distance inside from the edge of a surface panel that Smoldyn will place surface-bound molecules that hop onto this panel. Neighbor distance is the maximum distance over which surface-bound molecules are allowed to hop to transition from one panel to a neighboring panel.

    enum ErrorCode
    smolAddPanelNeighbor(simptr sim,const char *surface1,const char *panel1,const char *surface2,const char *panel2,int reciprocal);
    Adds panel panel2 of surface surface2 as a neighbor of panel panel1 or surface surface1, meaning that surface-bound molecules will be allowed to diffuse from panel1 to panel2. These are not allowed to be the same panel. Also, “all” values are not permitted. Otherwise, essentially any possible entries are legitimate. If surface-bound molecules should also be allowed to diffuse from panel2 to panel1, enter reciprocal as 1; if not, enter reciprocal as 0.

    enum ErrorCode
    smolSetSurfaceStyle(simptr sim,const char *surface,enum PanelFace face,enum DrawMode mode,double thickness,double *color,int stipplefactor,int stipplepattern,double shininess);
    Sets the graphics output style for face face of surface surface. mode is the drawing mode; enter it as DMnone to not set this parameter and otherwise enter it as DMno to not draw the surface, DMvert for vertices, DMedge for edges, or DMface for faces. The thickness parameter gives the point size or line width for drawing vertices or edges, or can be entered as a negative number to not set this parameter. color is the 4-value color vector for the surface, or can be entered as NULL to not set this parameter. stipplefactor is the repeat distance for the entire edge stippling pattern, or can be entered as a negative number to not set it. stipplepattern is the edge stippling pattern, which needs to be between 0 and 0xFFFF, or can be entered as -1 to not set this parameter. And shininess is the surface shininess, for use with lighting in the “opengl_better” graphics display option, or can be entered as -1 to not set this parameter. The parameters thickness, stipplefactor, and stipplepattern only apply to edge style drawing modes and ignore any input in the face entry. The shininess parameter only applies to the face style drawing modes.

Compartments

    int
    smolAddCompartment(simptr sim,char *compartment);
    Adds a compartment called compartment to the system.

    int
    smolGetCompartmentIndex(simptr sim,char *compartment)
    Returns the index of the compartment named compartment. On failure, this returns an error code cast as an integer.

    char*
    smolGetCompartmentName(simptr sim,int compartmentindex,char *compartment)
    Returns the name of the compartment that has index compartmentindex both directly and in the string compartment. Returns NULL if an error arises.

    enum ErrorCode
    smolAddCompartmentSurface(simptr sim,char *compartment,char *surface);
    Adds surface surface as one of the bounding surfaces of compartment compartment.

    enum ErrorCode
    smolAddCompartmentPoint(simptr sim,char *compartment,double *point);
    Adds point as one of the interior-defining points of compartment compartment.

    enum ErrorCode smolAddCompartmentLogic(simptr sim,char *compartment,enum
    CmptLogic logic,char *compartment2); Modifies the current definition of
    compartment compartment using a logical rule specified in logic and the
    definition of compartment2.

'''


class Geometry(object):
    """Geometry in smoldyn.


    """

    def __init__(self):
        super().__init__()
    

class Surface(Geometry):
    """Surface

    """
    def __init__(self, arg):
        super().__init__()


class Compartment(Geometry):
    """Compartment
    """
    def __init__(self, arg):
        super().__init__()
        self.arg = arg
    
