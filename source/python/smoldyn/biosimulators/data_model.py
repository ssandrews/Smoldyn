"""Data model for representing configurations for Smoldyn simulations for use with SED-ML"""

from biosimulators_utils.data_model import ValueType
import enum
import types  # noqa: F401

__author__ = "Jonathan Karr"
__email__ = "karr@mssm.edu"

__all__ = [
    "Simulation",
    "SimulationInstruction",
    "SmoldynCommand",
    "SmoldynOutputFile",
    "SimulationChange",
    "SimulationChangeExecution",
    "AlgorithmParameterType",
    "KISAO_ALGORITHMS_MAP",
    "KISAO_ALGORITHM_PARAMETERS_MAP",
]


class Simulation(object):
    """Configuration of a simulation

    Attributes:
        species (:obj:`list` of :obj:`str`): names of species
        compartments (:obj:`list` of :obj:`str`): names of compartments
        surfaces (:obj:`list` of :obj:`str`): names of surfaces
        instructions (:obj:`list` of :obj:`SimulationInstruction`): instructions
    """

    def __init__(
        self, species=None, compartments=None, surfaces=None, instructions=None
    ):
        """
        Args:
            species (:obj:`list` of :obj:`str`, optional): names of species
            compartments (:obj:`list` of :obj:`str`, optional): names of compartments
            surfaces (:obj:`list` of :obj:`str`, optional): names of surfaces
            instructions (:obj:`list` of :obj:`SimulationInstruction`, optional): instructions
        """
        self.species = species or []
        self.compartments = compartments or []
        self.surfaces = surfaces or []
        self.instructions = instructions or []


class SimulationInstruction(object):
    """Configuration of a simulation

    Attributes:
        macro (:obj:`str`): Smoldyn macro
        arguments (:obj:`str`): arguments of the Smoldyn macro
        id (:obj:`str`): unique id of the instruction
        description (:obj:`str`): human-readable description of the instruction
        comment (:obj:`str`): comment about the instruction
    """

    def __init__(self, macro, arguments, id=None, description=None, comment=None):
        """
        Args:
            macro (:obj:`str`): Smoldyn macro
            arguments (:obj:`str`): arguments of the Smoldyn macro
            id (:obj:`str`, optional): unique id of the instruction
            description (:obj:`str`, optional): human-readable description of the instruction
            comment (:obj:`str`, optional): comment about the instruction
        """
        self.macro = macro
        self.arguments = arguments
        self.id = id
        self.description = description
        self.comment = comment

    def is_equal(self, other):
        return (
            self.__class__ == other.__class__
            and self.macro == other.macro
            and self.arguments == other.arguments
            and self.id == other.id
            and self.description == other.description
            and self.comment == other.comment
        )


class SmoldynCommand(object):
    """A Smoldyn command

    Attributes:
        command (:obj:`str`): command (e.g., ``molcount``)
        type (:obj:`str`): command type (e.g., ``E``)
    """

    def __init__(self, command, type):
        """
        Args:
            command (:obj:`str`): command (e.g., ``molcount``)
            type (:obj:`str`): command type (e.g., ``E``)
        """
        self.command = command
        self.type = type


class SmoldynOutputFile(object):
    """A Smoldyn output file

    Attributes:
        name (:obj:`str`): name
        filename (:obj:`str`): path to the file
    """

    def __init__(self, name, filename):
        """
        Args:
            name (:obj:`str`): name
            filename (:obj:`str`): path to the file
        """
        self.name = name
        self.filename = filename


class SimulationChange(object):
    """A change to a Smoldyn simulation

    Attributes:
        command (:obj:`types.FunctionType`): function for generating a Smoldyn configuration command for a new value
        execution (:obj:`SimulationChangeExecution`): operation when change should be executed
    """

    def __init__(self, command, execution):
        """
        Args:
            command (:obj:`types.FunctionType`): function for generating a Smoldyn configuration command for a new value
        execution (:obj:`SimulationChangeExecution`): operation when change should be executed
        """
        self.command = command
        self.execution = execution


class SimulationChangeExecution(str, enum.Enum):
    """Operation when a simulation change should be executed"""

    preprocessing = "preprocessing"
    simulation = "simulation"


class AlgorithmParameterType(str, enum.Enum):
    """Type of algorithm parameter"""

    run_argument = "run_argument"
    instance_attribute = "instance_attribute"


KISAO_ALGORITHMS_MAP = {
    "KISAO_0000057": {
        "name": "Brownian diffusion Smoluchowski method",
    }
}

KISAO_ALGORITHM_PARAMETERS_MAP = {
    "KISAO_0000254": {
        "name": "accuracy",
        "type": AlgorithmParameterType.run_argument,
        "data_type": ValueType.float,
        "default": 10.0,
    },
    "KISAO_0000488": {
        "name": "setRandomSeed",
        "type": AlgorithmParameterType.instance_attribute,
        "data_type": ValueType.integer,
        "default": None,
    },
}
