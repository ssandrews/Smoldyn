''' Data model for representing configurations for Smoldyn simulations for use with SED-ML
'''

__author__ = 'Jonathan Karr'
__email__ = 'karr@mssm.edu'

__all__ = [
    'Simulation',
    'SimulationInstruction',
]


class Simulation(object):
    """ Configuration of a simulation

    Attributes:
        species (:obj:`list` of :obj:`str`): names of species
        compartments (:obj:`list` of :obj:`str`): names of compartments
        surfaces (:obj:`list` of :obj:`str`): names of surfaces
        instructions (:obj:`list` of :obj:`SimulationInstruction`): instructions
    """

    def __init__(self, species=None, compartments=None, surfaces=None, instructions=None):
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
    """ Configuration of a simulation

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
        return (self.__class__ == other.__class__
                and self.macro == other.macro
                and self.arguments == other.arguments
                and self.id == other.id
                and self.description == other.description
                and self.comment == other.comment
                )
