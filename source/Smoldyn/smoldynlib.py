# Written by Shawn Garbett.  Added to the Smoldyn files Feb. 11, 2013 by Steve Andrews.

from ctypes import *
from ctypes.util import find_library

# Load Smoldyn
path = find_library("smoldyn_shared")
smoldyn = cdll.LoadLibrary(path)

# Define Get Version
smolGetVersion = smoldyn.smolGetVersion
smolGetVersion.restype = c_double

# Error Handling

# Simulation Structure
smolNewSim = smoldyn.smolNewSim
smolNewSim.restype = c_void_p
smolNewSim.argtypes = [c_int, c_void_p, c_void_p]

smolUpdateSim = smoldyn.smolUpdateSim
smolUpdateSim.restype = c_int
smolUpdateSim.argtypes = [c_void_p]

smolRunTimeStep = smoldyn.smolRunTimeStep
smolRunTimeStep.restype = c_int
smolRunTimeStep.argtypes = [c_void_p]

smolRunSim = smoldyn.smolRunSim
smolRunSim.restype = c_int
smolRunSim.argtypes = [c_void_p]

smolRunSimUntil = smoldyn.smolRunSimUntil
smolRunSimUntil.restype = c_int
smolRunSimUntil.argtypes = [c_void_p, c_double]

smolFreeSim = smoldyn.smolFreeSim
smolFreeSim.restype = c_int
smolFreeSim.argtypes = [c_void_p]

smolDisplaySim = smoldyn.smolDisplaySim
smolDisplaySim.restype = c_int
smolDisplaySim.argtypes = [c_void_p]

# Simulation Settings
smolSetSimTimes = smoldyn.smolSetSimTimes
smolSetSimTimes.restype = c_int
smolSetSimTimes.argtypes = [c_void_p, c_double, c_double, c_double]

smolSetTimeStart = smoldyn.smolSetTimeStart
smolSetTimeStart.restype = c_int
smolSetTimeStart.argtypes = [c_void_p, c_double]

smolSetTimeStop = smoldyn.smolSetTimeStop
smolSetTimeStop.restype = c_int
smolSetTimeStop.argtypes = [c_void_p, c_double]

smolSetTimeNow = smoldyn.smolSetTimeNow
smolSetTimeNow.restype = c_int
smolSetTimeNow.argtypes = [c_void_p, c_double]

smolSetTimeStep = smoldyn.smolSetTimeStep
smolSetTimeStep.restype = c_int
smolSetTimeStep.argtypes = [c_void_p, c_double]

smolSetRandomSeed = smoldyn.smolSetRandomSeed
smolSetRandomSeed.restype = c_int
smolSetRandomSeed.argtypes = [c_void_p, c_long]

smolSetPartitions = smoldyn.smolSetPartitions
smolSetPartitions.restype = c_int
smolSetPartitions.argtypes = [c_void_p, c_char_p, c_double]

# Runtime commands (Output values to file)
smolSetOutputPath = smoldyn.smolSetOutputPath
smolSetOutputPath.restype = c_int
smolSetOutputPath.argtypes = [c_void_p, c_char_p]

smolAddOutputFile = smoldyn.smolAddOutputFile
smolAddOutputFile.restype = c_int
smolAddOutputFile.argtypes = [c_void_p, c_char_p, c_int, c_int]

smolAddCommand = smoldyn.smolAddCommand
smolAddCommand.restype = c_int
smolAddCommand.argtypes = [c_void_p, c_char, c_double, c_double, c_double, d_double, c_char_p]

smolAddCommandFromString = smoldyn.smolAddCommandFromString
smolAddCommandFromString.restype = c_int
smolAddCommandFromString.argtypes = [c_void_p, c_char_p]

# Surfaces
smolSetBoundary = smoldyn.smolSetBoundary
smolSetBoundary.restype = c_int
smolSetBoundary.argtypes = [c_void_p, c_int, c_int, c_char]

smolAddSurface = smoldyn.smolAddSurface
smolAddSurface.restype = c_int
smolAddSurface.argtypes = [c_void_p, c_char]

smolGetSurfaceIndex = smoldyn.smolGetSurfaceIndex
smolGetSurfaceIndex.restype = c_int
smolGetSurfaceIndex.argtypes = [c_void_p, c_char_p]

smolGetSurfaceIndexNT = smoldyn.smolGetSurfaceIndexNT
smolGetSurfaceIndexNT.restype = c_int
smolGetSurfaceIndexNT.argtypes = [c_void_p, c_char_p]

smolGetSurfaceName = smoldyn.smolGetSurfaceName
smolGetSurfaceName.restype = c_char_p
smolGetSurfaceName.argtypes = [c_void_p, c_int, c_char_p]

smolSetSurfaceAction = smoldyn.smolSetSurfaceAction
smolSetSurfaceAction.restype = c_int
smolSetSurfaceAction.argtypes = [c_void_p, c_char_p, c_int, c_char_p, c_int, c_int, c_char_p]

smolSetSurfaceRate = smoldyn.smolSetSurfaceRate
smolSetSurfaceRate.restype = c_int
smolSetSurfaceRate.argtypes = [c_void_p, c_char_p, c_char_p, c_int, c_int, c_int, c_double, c_char_p, c_int]

smolAddPanel = smoldyn.smolAddPanel
smolAddPanel.restype = c_int
smolAddPanel.argtypes = [c_void_p, c_char_p, c_int, c_char_p, c_char_p, c_void_p]

#extern "C" int            smolGetPanelIndex(simptr sim,const char *surface,enum PanelShape *panelshapeptr,const char *panel);
#smolDisplaySim = smoldyn.smolDisplaySim
#smolDisplaySim.restype = c_int
#smolDisplaySim.argtypes = [c_void_p, c_char_p, ???]
#
#extern "C" int            smolGetPanelIndexNT(simptr sim,const char *surface,enum PanelShape *panelshapeptr,const char *panel);
#smolDisplaySim = smoldyn.smolDisplaySim
#smolDisplaySim.restype = c_int
#smolDisplaySim.argtypes = [c_void_p]

smolGetPanelName = smoldyn.smolGetPanelName
smolGetPanelName.restype = c_char_p
smolGetPanelName.argtypes = [c_void_p, c_char_p, c_int, c_int, c_char_p]

smolSetPanelJump = smoldyn.smolSetPanelJump
smolSetPanelJump.restype = c_int
smolSetPanelJump.argtypes = [c_void_p, c_char_p, c_char_p, c_int, c_char_p, c_int, c_int]

smolAddSurfaceUnboundedEmitter = smoldyn.smolAddSurfaceUnboundedEmitter
smolAddSurfaceUnboundedEmitter.restype = c_int
smolAddSurfaceUnboundedEmitter.argtypes = [c_void_p, c_char_p, c_int, c_char_p, c_double, c_void_p]

smolSetSurfaceSimParams = smoldyn.smolSetSurfaceSimParams
smolSetSurfaceSimParams.restype = c_int
smolSetSurfaceSimParams.argtypes = [c_void_p, c_char_p, c_double]

smolAddPanelNeighbor = smoldyn.smolAddPanelNeighbor
smolAddPanelNeighbor.restype = c_int
smolAddPanelNeighbor.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p, c_char_p, c_int]

smolSetSurfaceStyle = smoldyn.smolSetSurfaceStyle
smolSetSurfaceStyle.restype = c_int
smolSetSurfaceStyle.argtypes = [c_void_p, c_char_p, c_int, c_int, c_double, c_void_p, c_int, c_int, c_double]


# Compartments
smolAddCompartment = smoldyn.smolAddCompartment
smolAddCompartment.restype = c_int
smolAddCompartment.argtypes = [c_void_p, c_char_p]

smolGetCompartmentIndex = smoldyn.smolGetCompartmentIndex
smolGetCompartmentIndex.restype = c_int
smolGetCompartmentIndex.argtypes = [c_void_p, c_char_p]

smolGetCompartmentIndexNT = smoldyn.smolGetCompartmentIndexNT
smolGetCompartmentIndexNT.restype = c_int
smolGetCompartmentIndexNT.argtypes = [c_void_p, c_char_p]

smolGetCompartmentName = smoldyn.smolGetCompartmentName
smolGetCompartmentName.restype = c_char_p
smolGetCompartmentName.argtypes = [c_void_p, c_int, c_char_p]

smolAddCompartmentSurface = smoldyn.smolAddCompartmentSurface
smolAddCompartmentSurface.restype = c_int
smolAddCompartmentSurface.argtypes = [c_void_p, c_char_p, c_char_p]

smolAddCompartmentPoint = smoldyn.smolAddCompartmentPoint
smolAddCompartmentPoint.restype = c_int
smolAddCompartmentPoint.argtypes = [c_void_p, c_char_p, c_void_p]

smolAddCompartmentLogic = smoldyn.smolAddCompartmentLogic
smolAddCompartmentLogic.restype = c_int
smolAddCompartmentLogic.argtypes = [c_void_p, c_char_p, c_int, c_char_p]

# Reactions 

smolAddReaction = smoldyn.smolAddReaction
smolAddReaction.restype = c_int
smolAddReaction.argtypes = [c_void_p, c_char_p, c_char_p, c_int, c_char_p, c_int, c_int, c_void_p, c_void_p, c_double]

smolGetReactionIndex = smoldyn.smolGetReactionIndex
smolGetReactionIndex.restype = c_int
smolGetReactionIndex.argtypes = [c_void_p, c_void_p, c_char_p]

smolGetReactionIndexNT = smoldyn.smolGetReactionIndexNT
smolGetReactionIndexNT.restype = c_int
smolGetReactionIndexNT.argtypes = [c_void_p, c_void_p, c_char_p]

smolGetReactionName = smoldyn.smolGetReactionName
smolGetReactionName.restype = c_char_p
smolGetReactionName.argtypes = [c_void_p, c_int, c_int, c_char_p]

smolSetReactionRate = smoldyn.smolSetReactionRate
smolSetReactionRate.restype = c_int
smolSetReactionRate.argtypes = [c_void_p, c_char_p, c_double, c_int]

smolSetReactionRegion = smoldyn.smolSetReactionRegion
smolSetReactionRegion.restype = c_int
smolSetReactionRegion.argtypes = [c_void_p, c_char_p, c_char_p, c_char_p]

smolSetReactionProducts = smoldyn.smolSetReactionProducts
smolSetReactionProducts.restype = c_int
smolSetReactionProducts.argtypes = [c_void_p, c_char_p, c_int, c_double, c_char_p, c_void_p]


# Bit of test code
smolGetVersion()
lbound = (c_double * 2)(0.0, 0.0)
ubound = (c_double * 2)(100.0, 100.0)
s = smolNewSim(2, pointer(lbound), pointer(ubound))
smolDisplaySim(s)
smolFreeSim(s)
