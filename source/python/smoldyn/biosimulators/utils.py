"""Methods for parsing configurations of Smoldyn simulations for use with SED-ML"""

__author__ = "Jonathan Karr"
__email__ = "karr@mssm.edu"

__all__ = [
    "read_simulation",
]

from .data_model import Simulation, SimulationInstruction
import re


def read_simulation(filename):
    """Read the configuration for a Smoldyn simulation

    Args:
        filename (:obj:`str`): path to a configuration for a Smoldyn simulation

    Returns:
        :obj:`Simulation`: data structure which represents the configuration of the Smoldyn simulation
    """
    sim = Simulation()
    param_group_counts = {}
    with open(filename, "r") as file:
        for line in file:
            # separate comments
            line, _, comment = line.partition("#")
            line = line.strip()
            comment = comment.strip()

            # remove consecutive spaces
            line = re.sub(" +", " ", line)

            _read_simulation_line(line, param_group_counts, sim)

            if re.match(r"^end_file( |$)", line):
                break

    return sim


def _read_simulation_line(line, macro_counts, sim):
    """Parse a line of a configuration of a Smoldyn simulation

    Args:
        line (:obj:`str`): line of a configuration of a Smoldyn simulation
        macro_counts (:obj:`dict`): dictionary used to count instances of the same macro
        sim (:obj:`Simulation`): data structure which represents the configuration of the Smoldyn simulation
    """
    if line.startswith("species "):
        sim.species.extend(line.split(" ")[1:])

    elif line.startswith("start_compartment "):
        sim.compartments.append(line.partition(" ")[2])

    elif line.startswith("start_surface "):
        sim.surfaces.append(line.partition(" ")[2])

    for pattern in CONFIG_DECLARATION_PATTERNS:
        match = re.match(pattern["regex"], line)
        if match:
            if pattern.get("macro", None):
                group = pattern["macro"]["group"](match)
                if group not in macro_counts:
                    macro_counts[group] = -1
                macro_counts[group] += 1
                i_group = macro_counts[group]

                sim.instructions.append(
                    SimulationInstruction(
                        id=pattern["macro"]["id"](match, i_group),
                        description=pattern["macro"]["description"](match, i_group),
                        macro=pattern["macro"]["macro"](match),
                        arguments=pattern["macro"]["arguments"](match),
                    )
                )


CONFIG_DECLARATION_PATTERNS = [
    {
        "regex": r"^(dim) (.*?)$",
        "macro": {
            "group": lambda match: "number_dimensions",
            "id": lambda match, i_group: "number_dimensions",
            "description": lambda match, i_group: "Number of dimensions",
            "macro": lambda match: match.group(1),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^low_wall ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "low_wall_{}".format(match.group(1)),
            "id": lambda match, i_group: "low_{}_wall_{}".format(
                match.group(1), i_group + 1
            ),
            "description": lambda match, i_group: "Low {} wall {}".format(
                match.group(1), i_group + 1
            ),
            "macro": lambda match: "low_wall {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^high_wall ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "high_wall_{}".format(match.group(1)),
            "id": lambda match, i_group: "high_{}_wall_{}".format(
                match.group(1), i_group + 1
            ),
            "description": lambda match, i_group: "High {} wall {}".format(
                match.group(1), i_group + 1
            ),
            "macro": lambda match: "high_wall {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^boundaries ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "boundaries_{}".format(match.group(1)),
            "id": lambda match, i_group: "{}_boundary".format(match.group(1)),
            "description": lambda match, i_group: "{} boundary".format(
                match.group(1).upper()
            ),
            "macro": lambda match: "boundaries {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^define ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "define_{}".format(match.group(1)),
            "id": lambda match, i_group: "value_parameter_{}".format(match.group(1)),
            "description": lambda match, i_group: 'Value of parameter "{}"'.format(
                match.group(1)
            ),
            "macro": lambda match: "define {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^difc ([^ \(\)]+) (.*?)$",
        "macro": {
            "group": lambda match: "difc_{}".format(match.group(1)),
            "id": lambda match, i_group: "diffusion_coefficient_species_{}".format(
                match.group(1)
            ),
            "description": lambda match,
            i_group: 'Diffusion coefficient of species "{}"'.format(match.group(1)),
            "macro": lambda match: "difc {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^difc ([^ \(\)]+)\(([^ \(\)]+)\) (.*?)$",
        "macro": {
            "group": lambda match: "difc_{}_{}".format(match.group(1), match.group(2)),
            "id": lambda match,
            i_group: "diffusion_coefficient_species_{}_state_{}".format(
                match.group(1), match.group(2)
            ),
            "description": lambda match,
            i_group: 'Diffusion coefficient of species "{}" in state "{}"'.format(
                match.group(1), match.group(2)
            ),
            "macro": lambda match: "difc {}({})".format(match.group(1), match.group(2)),
            "arguments": lambda match: match.group(3),
        },
    },
    {
        "regex": r"^difc_rule ([^ \(\)]+) (.*?)$",
        "macro": {
            "group": lambda match: "difc_rule_{}".format(match.group(1)),
            "id": lambda match, i_group: "diffusion_coefficient_rule_species_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1))
            ),
            "description": lambda match,
            i_group: 'Diffusion coefficient rule for species "{}"'.format(
                match.group(1)
            ),
            "macro": lambda match: "difc_rule {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^difc_rule ([^ \(\)]+)\(([^ \(\)]+)\) (.*?)$",
        "macro": {
            "group": lambda match: "difc_rule_{}_{}".format(
                match.group(1), match.group(2)
            ),
            "id": lambda match,
            i_group: "diffusion_coefficient_rule_species_{}_state_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1)), match.group(2)
            ),
            "description": lambda match,
            i_group: 'Diffusion coefficient rule for species "{}" in state "{}"'.format(
                match.group(1), match.group(2)
            ),
            "macro": lambda match: "difc_rule {}({})".format(
                match.group(1), match.group(2)
            ),
            "arguments": lambda match: match.group(3),
        },
    },
    {
        "regex": r"^difm ([^ \(\)]+) (.*?)$",
        "macro": {
            "group": lambda match: "difm_{}".format(match.group(1)),
            "id": lambda match,
            i_group: "membrane_diffusion_coefficient_species_{}".format(match.group(1)),
            "description": lambda match,
            i_group: 'Membrane diffusion coefficient of species "{}"'.format(
                match.group(1)
            ),
            "macro": lambda match: "difm {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^difm ([^ \(\)]+)\(([^ \(\)]+)\) (.*?)$",
        "macro": {
            "group": lambda match: "difm_{}_{}".format(match.group(1), match.group(2)),
            "id": lambda match,
            i_group: "membrane_diffusion_coefficient_species_{}_state_{}".format(
                match.group(1), match.group(2)
            ),
            "description": lambda match,
            i_group: 'Membrane diffusion coefficient of species "{}" in state "{}"'.format(
                match.group(1), match.group(2)
            ),
            "macro": lambda match: "difm {}({})".format(match.group(1), match.group(2)),
            "arguments": lambda match: match.group(3),
        },
    },
    {
        "regex": r"^difm_rule ([^ \(\)]+) (.*?)$",
        "macro": {
            "group": lambda match: "difm_rule_{}".format(match.group(1)),
            "id": lambda match,
            i_group: "membrane_diffusion_coefficient_rule_species_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1))
            ),
            "description": lambda match,
            i_group: 'Membrane diffusion coefficient rule for species "{}"'.format(
                match.group(1)
            ),
            "macro": lambda match: "difm_rule {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^difm_rule ([^ \(\)]+)\(([^ \(\)]+)\) (.*?)$",
        "macro": {
            "group": lambda match: "difm_rule_{}_{}".format(
                match.group(1), match.group(2)
            ),
            "id": lambda match,
            i_group: "membrane_diffusion_coefficient_rule_species_{}_state_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1)), match.group(2)
            ),
            "description": lambda match,
            i_group: 'Membrane diffusion coefficient rule for species "{}" in state "{}"'.format(
                match.group(1), match.group(2)
            ),
            "macro": lambda match: "difm_rule {}({})".format(
                match.group(1), match.group(2)
            ),
            "arguments": lambda match: match.group(3),
        },
    },
    {
        "regex": r"^drift ([^ \(\)]+) (.*?)$",
        "macro": {
            "group": lambda match: "drift_{}".format(match.group(1)),
            "id": lambda match, i_group: "drift_species_{}".format(match.group(1)),
            "description": lambda match, i_group: 'Drift of species "{}"'.format(
                match.group(1)
            ),
            "macro": lambda match: "drift {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^drift ([^ \(\)]+)\(([^ \(\)]+)\) (.*?)$",
        "macro": {
            "group": lambda match: "drift_{}_{}".format(match.group(1), match.group(2)),
            "id": lambda match, i_group: "drift_species_{}_state_{}".format(
                match.group(1), match.group(2)
            ),
            "description": lambda match,
            i_group: 'Drift of species "{}" in state "{}"'.format(
                match.group(1), match.group(2)
            ),
            "macro": lambda match: "drift {}({})".format(
                match.group(1), match.group(2)
            ),
            "arguments": lambda match: match.group(3),
        },
    },
    {
        "regex": r"^drift_rule ([^ \(\)]+) (.*?)$",
        "macro": {
            "group": lambda match: "drift_rule_{}".format(match.group(1)),
            "id": lambda match, i_group: "drift_rule_species_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1))
            ),
            "description": lambda match, i_group: 'Drift rule for species "{}"'.format(
                match.group(1)
            ),
            "macro": lambda match: "drift_rule {}".format(match.group(1)),
            "arguments": lambda match: match.group(2),
        },
    },
    {
        "regex": r"^drift_rule ([^ \(\)]+)\(([^ \(\)]+)\) (.*?)$",
        "macro": {
            "group": lambda match: "drift_rule_{}_{}".format(
                match.group(1), match.group(2)
            ),
            "id": lambda match, i_group: "drift_rule_species_{}_state_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1)), match.group(2)
            ),
            "description": lambda match,
            i_group: 'Drift rule for species "{}" in state "{}"'.format(
                match.group(1), match.group(2)
            ),
            "macro": lambda match: "drift_rule {}({})".format(
                match.group(1), match.group(2)
            ),
            "arguments": lambda match: match.group(3),
        },
    },
    {
        "regex": r"^surface_drift ([^ \(\)]+) ([^ ]+) ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "surface_drift_{}_{}_{}".format(
                match.group(1), match.group(2), match.group(3)
            ),
            "id": lambda match,
            i_group: "surface_drift_species_{}_surface_{}_shape_{}".format(
                match.group(1), match.group(2), match.group(3)
            ),
            "description": lambda match,
            i_group: 'Surface drift of species "{}" on surface "{}" with panel shape "{}"'.format(
                match.group(1), match.group(2), match.group(3)
            ),
            "macro": lambda match: "surface_drift {} {} {}".format(
                match.group(1), match.group(2), match.group(3)
            ),
            "arguments": lambda match: match.group(4),
        },
    },
    {
        "regex": r"^surface_drift ([^ \(\)]+)\(([^ \(\)]+)\) ([^ ]+) ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "surface_drift_{}_{}_{}_{}".format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "id": lambda match,
            i_group: "surface_drift_species_{}_state_{}_surface_{}_shape_{}".format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "description": lambda match,
            i_group: 'Surface drift of species "{}" in state "{}" on surface "{}" with panel shape "{}"'.format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "macro": lambda match: "surface_drift {}({}) {} {}".format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "arguments": lambda match: match.group(5),
        },
    },
    {
        "regex": r"^surface_drift_rule ([^ \(\)]+) ([^ ]+) ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "surface_drift_rule_{}_{}_{}".format(
                match.group(1), match.group(2), match.group(3)
            ),
            "id": lambda match,
            i_group: "surface_drift_rule_species_{}_surface_{}_panel_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1)),
                match.group(2),
                match.group(3),
            ),
            "description": lambda match,
            i_group: 'Surface drift rule for species "{}" on surface "{}" of panel shape "{}"'.format(
                match.group(1), match.group(2), match.group(3)
            ),
            "macro": lambda match: "surface_drift_rule {} {} {}".format(
                match.group(1), match.group(2), match.group(3)
            ),
            "arguments": lambda match: match.group(4),
        },
    },
    {
        "regex": r"^surface_drift_rule ([^ \(\)]+)\(([^ \(\)]+)\) ([^ ]+) ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "surface_drift_rule_{}_{}_{}_{}".format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "id": lambda match,
            i_group: "surface_drift_rule_species_{}_state_{}_surface_{}_panel_{}".format(
                re.sub("[^a-zA-Z0-9_]", "_", match.group(1)),
                match.group(2),
                match.group(3),
                match.group(4),
            ),
            "description": lambda match,
            i_group: 'Surface drift rule for species "{}" in state "{}" on surface "{}" of panel shape "{}"'.format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "macro": lambda match: "surface_drift_rule {}({}) {} {}".format(
                match.group(1), match.group(2), match.group(3), match.group(4)
            ),
            "arguments": lambda match: match.group(5),
        },
    },
    {
        "regex": r"^mol ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "mol_{}".format(match.group(2)),
            "id": lambda match, i_group: "initial_count_species_{}".format(
                re.sub(r"[^a-zA-Z0-9_]", "_", match.group(2))
            ),
            "description": lambda match,
            i_group: 'Initial count of species "{}"'.format(match.group(2)),
            "macro": lambda match: "mol {}".format(match.group(2)),
            "arguments": lambda match: match.group(1),
        },
    },
    {
        "regex": r"^compartment_mol ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "compartment_mol_{}".format(match.group(2)),
            "id": lambda match, i_group: "initial_count_species_{}".format(
                re.sub(r"[^a-zA-Z0-9_]", "_", match.group(2))
            ),
            "description": lambda match,
            i_group: 'Initial count of species "{}"'.format(match.group(2)),
            "macro": lambda match: "compartment_mol {}".format(match.group(2)),
            "arguments": lambda match: match.group(1),
        },
    },
    {
        "regex": r"^surface_mol ([^ ]+) (.*?)$",
        "macro": {
            "group": lambda match: "surface_mol_{}".format(match.group(2)),
            "id": lambda match, i_group: "initial_count_species_{}".format(
                re.sub(r"[^a-zA-Z0-9_]", "_", match.group(2))
            ),
            "description": lambda match,
            i_group: 'Initial count of species "{}"'.format(match.group(2)),
            "macro": lambda match: "surface_mol {}".format(match.group(2)),
            "arguments": lambda match: match.group(1),
        },
    },
]
