''' Methods for executing COMBINE archives and SED-ML files involving simulations of models encoded in Smoldyn

    Information about encoding Smoldyn simulations into SED-ML files and COMBINE archives is available at
    `https://github.com/ssandrews/Smoldyn/blob/master/Using-Smoldyn-with-SED-ML-COMBINE-BioSimulators.md <https://github.com/ssandrews/Smoldyn/blob/master/Using-Smoldyn-with-SED-ML-COMBINE-BioSimulators.md>`_.
'''

__author__ = 'Jonathan Karr'
__email__ = 'karr@mssm.edu'

__all__ = [
    'KISAO_ALGORITHMS_MAP',
    'KISAO_ALGORITHM_PARAMETERS_MAP',
    'exec_sedml_docs_in_combine_archive',
    'exec_sed_task',
]

from biosimulators_utils.combine.exec import exec_sedml_docs_in_archive
from biosimulators_utils.data_model import ValueType
from biosimulators_utils.log.data_model import CombineArchiveLog, TaskLog  # noqa: F401
from biosimulators_utils.plot.data_model import PlotFormat  # noqa: F401
from biosimulators_utils.report.data_model import ReportFormat, VariableResults  # noqa: F401
from biosimulators_utils.sedml import validation
from biosimulators_utils.sedml.data_model import (Task, ModelLanguage, ModelAttributeChange,  # noqa: F401
                                                  UniformTimeCourseSimulation, AlgorithmParameterChange, Variable,
                                                  Symbol)
from biosimulators_utils.sedml.exec import exec_sed_doc
from biosimulators_utils.utils.core import validate_str_value, parse_value, raise_errors_warnings
from smoldyn import smoldyn
import enum
import functools
import os
import numpy
import pandas
import re
import tempfile


class AlgorithmParameterType(str, enum.Enum):
    ''' Type of algorithm parameter '''
    run_argument = 'run_argument'
    instance_attribute = 'instance_attribute'


class SmoldynOutputFile(object):
    ''' A Smoldyn output file

    Attributes:
        name (:obj:`str`): name
        filename (:obj:`str`): path to the file
    '''

    def __init__(self, name, filename):
        '''
        Args:
            name (:obj:`str`): name
            filename (:obj:`str`): path to the file
        '''
        self.name = name
        self.filename = filename


class SmoldynCommand(object):
    ''' A Smoldyn command

    Attributes:
        command (:obj:`str`): command (e.g., ``molcount``)
        type (:obj:`str`): command type (e.g., ``E``)
    '''

    def __init__(self, command, type):
        '''
        Args:
            command (:obj:`str`): command (e.g., ``molcount``)
            type (:obj:`str`): command type (e.g., ``E``)
        '''
        self.command = command
        self.type = type


KISAO_ALGORITHMS_MAP = {
    'KISAO_0000057': {
        'name': 'Brownian diffusion Smoluchowski method',
    }
}

KISAO_ALGORITHM_PARAMETERS_MAP = {
    'KISAO_0000254': {
        'name': 'accuracy',
        'type': AlgorithmParameterType.run_argument,
        'data_type': ValueType.float,
        'default': 10.,
    },
    'KISAO_0000488': {
        'name': 'setRandomSeed',
        'type': AlgorithmParameterType.instance_attribute,
        'data_type': ValueType.integer,
        'default': None,
    },
}


def exec_sedml_docs_in_combine_archive(archive_filename, out_dir,
                                       report_formats=None, plot_formats=None,
                                       bundle_outputs=None, keep_individual_outputs=None):
    ''' Execute the SED tasks defined in a COMBINE/OMEX archive and save the outputs

    Args:
        archive_filename (:obj:`str`): path to COMBINE/OMEX archive
        out_dir (:obj:`str`): path to store the outputs of the archive

            * CSV: directory in which to save outputs to files
              ``{ out_dir }/{ relative-path-to-SED-ML-file-within-archive }/{ report.id }.csv``
            * HDF5: directory in which to save a single HDF5 file (``{ out_dir }/reports.h5``),
              with reports at keys ``{ relative-path-to-SED-ML-file-within-archive }/{ report.id }`` within the HDF5 file

        report_formats (:obj:`list` of :obj:`ReportFormat`, optional): report format (e.g., csv or h5)
        plot_formats (:obj:`list` of :obj:`PlotFormat`, optional): report format (e.g., pdf)
        bundle_outputs (:obj:`bool`, optional): if :obj:`True`, bundle outputs into archives for reports and plots
        keep_individual_outputs (:obj:`bool`, optional): if :obj:`True`, keep individual output files

    Returns:
        :obj:`CombineArchiveLog`: log
    '''
    sed_doc_executer = functools.partial(exec_sed_doc, exec_sed_task)
    return exec_sedml_docs_in_archive(sed_doc_executer, archive_filename, out_dir,
                                      apply_xml_model_changes=False,
                                      report_formats=report_formats,
                                      plot_formats=plot_formats,
                                      bundle_outputs=bundle_outputs,
                                      keep_individual_outputs=keep_individual_outputs)


def exec_sed_task(sed_task, sed_variables, log=None):
    ''' Execute a task and save its results

    Args:
       sed_task (:obj:`Task`): task
       sed_variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
       log (:obj:`TaskLog`, optional): log for the task

    Returns:
        :obj:`tuple`:

            :obj:`VariableResults`: results of variables
            :obj:`TaskLog`: log
    '''
    log = log or TaskLog()

    sed_model = sed_task.model
    sed_simulation = sed_task.simulation

    raise_errors_warnings(validation.validate_task(sed_task),
                          error_summary='Task `{}` is invalid.'.format(sed_task.id))
    raise_errors_warnings(validation.validate_model_language(sed_model.language, ModelLanguage.Smoldyn),
                          error_summary='Language for model `{}` is not supported.'.format(sed_model.id))
    raise_errors_warnings(validation.validate_model_change_types(sed_model.changes, (ModelAttributeChange, )),
                          error_summary='Changes for model `{}` are not supported.'.format(sed_model.id))
    raise_errors_warnings(*validation.validate_model_changes(sed_model),
                          error_summary='Changes for model `{}` are invalid.'.format(sed_model.id))
    raise_errors_warnings(validation.validate_simulation_type(sed_simulation, (UniformTimeCourseSimulation, )),
                          error_summary='{} `{}` is not supported.'.format(sed_simulation.__class__.__name__, sed_simulation.id))
    raise_errors_warnings(*validation.validate_simulation(sed_simulation),
                          error_summary='Simulation `{}` is invalid.'.format(sed_simulation.id))
    raise_errors_warnings(*validation.validate_data_generator_variables(sed_variables),
                          error_summary='Data generator variables for task `{}` are invalid.'.format(sed_task.id))

    if sed_simulation.algorithm.kisao_id not in KISAO_ALGORITHMS_MAP:
        msg = 'Algorithm `{}` is not supported. The following algorithms are supported:{}'.format(
            sed_simulation.algorithm.kisao_id,
            ''.join('\n  {}: {}'.format(kisao_id, alg_props['name']) for kisao_id, alg_props in KISAO_ALGORITHMS_MAP.items())
        )
        raise NotImplementedError(msg)

    # read Smoldyn configuration
    simulation_configuration = read_smoldyn_simulation_configuration(sed_model.source)
    normalize_smoldyn_simulation_configuration(simulation_configuration)

    # turn off Smoldyn's graphics
    disable_smoldyn_graphics_in_simulation_configuration(simulation_configuration)

    # apply model changes to the Smoldyn configuration
    for change in sed_model.changes:
        apply_sed_model_change_to_smoldyn_simulation_configuration(change, simulation_configuration)

    # write the modified Smoldyn configuration to a temporary file
    fid, smoldyn_configuration_filename = tempfile.mkstemp(suffix='.txt')
    os.close(fid)
    write_smoldyn_simulation_configuration(simulation_configuration, smoldyn_configuration_filename)

    # initialize a simulation from the Smoldyn file
    smoldyn_simulation = init_smoldyn_simulation_from_configuration_file(smoldyn_configuration_filename)

    # get the Smoldyn representation of the SED uniform time course simulation
    smoldyn_run_timecourse_args = get_smoldyn_run_timecourse_args(sed_simulation)

    # apply the SED algorithm parameters to the Smoldyn simulation and to the arguments to its ``run`` method
    smoldyn_simulation_attr_setter_args = {}
    smoldyn_run_alg_param_args = {}
    for sed_algorithm_parameter_change in sed_simulation.algorithm.changes:
        val = get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(sed_algorithm_parameter_change)
        if val['type'] == AlgorithmParameterType.run_argument:
            smoldyn_run_alg_param_args[val['name']] = val['value']
        else:
            smoldyn_simulation_attr_setter_args[val['name']] = val['value']
            setter = getattr(smoldyn_simulation, val['name'])
            setter(val['value'])

    # Setup Smoldyn output files for the SED variables
    smoldyn_output_files = add_smoldyn_output_files_for_sed_variables(smoldyn_configuration_filename, sed_variables, smoldyn_simulation)

    # execute the simulation
    smoldyn_run_args = dict(
        **smoldyn_run_timecourse_args,
        **smoldyn_run_alg_param_args,
    )
    smoldyn_simulation.run(**smoldyn_run_args, overwrite=True, display=False, quit_at_end=False)

    # get the result of each SED variable
    variable_results = get_variable_results(sed_simulation.number_of_steps, sed_variables, smoldyn_output_files)

    # cleanup temporary files
    os.remove(smoldyn_configuration_filename)

    for smoldyn_output_file in smoldyn_output_files.values():
        os.remove(smoldyn_output_file.filename)

    # log simulation
    log.algorithm = sed_simulation.algorithm.kisao_id
    log.simulator_details = {
        'class': 'smoldyn.Simulation',
        'setInstanceAttributes': smoldyn_simulation_attr_setter_args,
        'method': 'run',
        'methodArguments': smoldyn_run_args,
    }

    # return the values of the variables and log
    return variable_results, log


def init_smoldyn_simulation_from_configuration_file(filename):
    ''' Initialize a simulation for a Smoldyn model from a file

    Args:
        filename (:obj:`str`): path to model file

    Returns:
        :obj:`smoldyn.Simulation`: simulation
    '''
    if not os.path.isfile(filename):
        raise FileNotFoundError('Model source `{}` is not a file.'.format(filename))

    smoldyn_simulation = smoldyn.Simulation.fromFile(filename)
    if not smoldyn_simulation.getSimPtr():
        error_code, error_msg = smoldyn.getError()
        msg = 'Model source `{}` is not a valid Smoldyn file.\n\n  {}: {}'.format(
            filename, error_code.name[0].upper() + error_code.name[1:], error_msg.replace('\n', '\n  '))
        raise ValueError(msg)

    return smoldyn_simulation


def read_smoldyn_simulation_configuration(filename):
    ''' Read a configuration for a Smoldyn simulation

    Args:
        filename (:obj:`str`): path to model file

    Returns:
        :obj:`list` of :obj:`str`: simulation configuration
    '''
    with open(filename, 'r') as file:
        return [line.strip('\n') for line in file]


def write_smoldyn_simulation_configuration(configuration, filename):
    ''' Write a configuration for Smoldyn simulation to a file

    Args:
        configuration
        filename (:obj:`str`): path to save configuration
    '''
    with open(filename, 'w') as file:
        for line in configuration:
            file.write(line)
            file.write('\n')


def normalize_smoldyn_simulation_configuration(configuration):
    ''' Normalize a configuration for a Smoldyn simulation

    Args:
        configuration (:obj:`list` of :obj:`str`): configuration for a Smoldyn simulation
    '''
    # normalize spacing and comments
    for i_line, line in enumerate(configuration):
        if '#' in line:
            cmd, comment = re.split('#+', line, maxsplit=1)
            cmd = re.sub(' +', ' ', cmd).strip()
            comment = comment.strip()

            if cmd:
                if comment:
                    line = cmd + ' # ' + comment
                else:
                    line = cmd
            else:
                if comment:
                    line = '# ' + comment
                else:
                    line = ''

        else:
            line = re.sub(' +', ' ', line).strip()

        configuration[i_line] = line

    # remove end_file and following lines
    for i_line, line in enumerate(configuration):
        if re.match(r'^end_file( |$)', line):
            for i_line_remove in range(len(configuration) - i_line):
                configuration.pop()
            break

    # remove empty starting lines
    for line in list(configuration):
        if not line:
            configuration.pop(0)
        else:
            break

    # remove empty ending lines
    for line in reversed(configuration):
        if not line:
            configuration.pop()
        else:
            break


def disable_smoldyn_graphics_in_simulation_configuration(configuration):
    ''' Turn off graphics in the configuration of a Smoldyn simulation

    Args:
        configuration (:obj:`list` of :obj:`str`): simulation configuration
    '''
    for i_line, line in enumerate(configuration):
        if line.startswith('graphics '):
            configuration[i_line] = re.sub(r'^graphics +[a-z_]+', 'graphics none', line)


def apply_sed_model_change_to_smoldyn_simulation_configuration(sed_model_change, simulation_configuration):
    ''' Apply a SED model attribute change to a configuration for a Smoldyn simulation

    ====================================================================  ===================
    target                                                                newValue type
    ====================================================================  ===================
    ``dim``                                                               integer
    ``low_wall {dim}``                                                    float string
    ``high_wall {dim}``                                                   float string
    ``boundaries {dim}``                                                  float float string?
    ``define {name}``                                                     float
    ``difc {species}``                                                    float
    ``difc {species}({state})``                                           float
    ``difc_rule {species}({state})``                                      float
    ``difm {species}``                                                    float[]
    ``difm {species}({state})``                                           float[]
    ``difm_rule {species}({state})``                                      float[]
    ``drift {species}``                                                   float[]
    ``drift {species}({state})``                                          float[]
    ``drift_rule {species}({state})``                                     float[]
    ``surface_drift {species}({state}) {surface} {panel-shape}``          float[]
    ``surface_drift_rule {species}({state}) {surface} {panel-shape}``     float[]
    ``mol {species} [{pos} ...]``                                         integer
    ``compartment_mol {species} {compartment-name}``                      integer
    ``surface_mol {species}({state}) {panel-shape} {panel} [{pos} ...]``  integer
    ====================================================================  ===================

    Args:
        sed_model_change (:obj:`ModelAttributeChange`): SED model change
        simulation_configuration (:obj:`list` of :obj:`str`): configuration for a Smoldyn simulation

    Raises:
        :obj:`ValueError`: invalid value for a model change
        :obj:`NotImplementedError`: unsupported model change
    '''
    # TODO: support additional types of model changes

    target_type, _, target = sed_model_change.target.strip().partition(' ')
    target_type = target_type.strip()
    target = re.sub(r' +', ' ', target).strip()
    new_value = sed_model_change.new_value.strip()

    valid = False

    if target_type in [
        'dim'
    ]:
        # Examples:
        #   dim 1
        for i_line, line in enumerate(simulation_configuration):
            pattern = r'^{} '.format(target_type)
            if re.match(pattern, line):
                valid = True
                simulation_configuration[i_line] = re.sub(r'^{} +[^#]+'.format(target_type),
                                                          '{} {} '.format(target_type, new_value),
                                                          line, count=1).strip()
                break

    elif target_type in [
        'boundaries', 'low_wall', 'high_wall',
        'define',
        'difc', 'difc_rule', 'difm', 'difm_rule', 'drift', 'drift_rule', 'surface_drift', 'surface_drift_rule'
    ]:
        # Examples:
        #   define K_FWD 0.001
        #   difc S 3
        #   difm red 1 0 0 0 0 0 0 0 2
        for i_line, line in enumerate(simulation_configuration):
            pattern = r'^{} +{} '.format(target_type, re.escape(target).replace(' ', ' +'))
            if re.match(pattern, line):
                valid = True
                simulation_configuration[i_line] = re.sub(r'^{} +{} +[^#]+'.format(target_type, re.escape(target).replace(' ', ' +')),
                                                          '{} {} {} '.format(target_type, target, new_value),
                                                          line, count=1).strip()
                break

    elif target_type in ['mol', 'compartment_mol', 'surface_mol']:
        # Examples:
        #   mol 5 red u
        #   compartment_mol 500 S inside
        #   surface_mol 100 E(front) membrane all all
        for i_line, line in enumerate(simulation_configuration):
            pattern = r'^{} +\d+ +{}'.format(target_type, re.escape(target).replace(' ', ' +'))
            if re.match(pattern, line):
                valid = True
                simulation_configuration[i_line] = re.sub(r'^{} +\d+ +{}'.format(target_type, re.escape(target).replace(' ', ' +')),
                                                          '{} {} {} '.format(target_type, new_value, target),
                                                          line, count=1).strip()
                break
    else:
        raise NotImplementedError('Target `{}` is not supported.'.format(sed_model_change.target))

    if not valid:
        raise ValueError('Model does not contain target `{}`.'.format(sed_model_change.target))


def get_smoldyn_run_timecourse_args(sed_simulation):
    ''' Get the Smoldyn representation of a SED uniform time course simulation

    Args:
        sed_simulation (:obj:`UniformTimeCourseSimulation`): SED uniform time course simulation

    Returns:
        :obj:`dict`: dictionary with keys ``start``, ``stop``, and ``dt`` that captures the Smoldyn
            representation of the time course

    Raises:
        :obj:`NotImplementedError`: unsupported timecourse
    '''
    number_of_steps = (
        (
            sed_simulation.output_end_time - sed_simulation.initial_time
        ) / (
            sed_simulation.output_end_time - sed_simulation.output_start_time
        ) * (
            sed_simulation.number_of_steps
        )
    )
    if (number_of_steps - int(number_of_steps)) > 1e-8:
        msg = (
            'Simulations must specify an integer number of steps, not {}.'
            '\n  Initial time: {}'
            '\n  Output start time: {}'
            '\n  Output end time: {}'
            '\n  Number of steps (output start to end time): {}'
        ).format(
            number_of_steps,
            sed_simulation.initial_time,
            sed_simulation.output_start_time,
            sed_simulation.output_end_time,
            sed_simulation.number_of_steps,
        )
        raise NotImplementedError(msg)

    dt = (sed_simulation.output_end_time - sed_simulation.output_start_time) / sed_simulation.number_of_steps

    return {
        'start': sed_simulation.initial_time,
        'stop': sed_simulation.output_end_time,
        'dt': dt,
    }


def get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(sed_algorithm_parameter_change):
    ''' Get the Smoldyn representation of a SED uniform time course simulation

    Args:
        sed_algorithm_parameter_change (:obj:`AlgorithmParameterChange`): SED change to a parameter of an algorithm

    Returns:
        :obj:`dict`: dictionary with keys ``type``, ``name``, and ``value`` that captures the Smoldyn representation
            of the algorithm parameter

    Raises:
        :obj:`ValueError`: unsupported algorithm parameter value
        :obj:`NotImplementedError`: unsupported algorithm parameter
    '''
    parameter_props = KISAO_ALGORITHM_PARAMETERS_MAP.get(sed_algorithm_parameter_change.kisao_id, None)
    if parameter_props:
        if not validate_str_value(sed_algorithm_parameter_change.new_value, parameter_props['data_type']):
            msg = '{} ({}) must be a {}, not `{}`.'.format(
                parameter_props['name'], sed_algorithm_parameter_change.kisao_id,
                parameter_props['data_type'].name, sed_algorithm_parameter_change.new_value,
            )
            raise ValueError(msg)
        new_value = parse_value(sed_algorithm_parameter_change.new_value, parameter_props['data_type'])

        return {
            'type': parameter_props['type'],
            'name': parameter_props['name'],
            'value': new_value,
        }

    else:
        msg = 'Algorithm parameter `{}` is not supported. The following parameters are supported:{}'.format(
            sed_algorithm_parameter_change.kisao_id,
            ''.join('\n  {}: {}'.format(kisao_id, parameter_props['name'])
                    for kisao_id, parameter_props in KISAO_ALGORITHM_PARAMETERS_MAP.items())
        )
        raise NotImplementedError(msg)


def add_smoldyn_output_file(configuration_filename, smoldyn_simulation):
    ''' Add an output file to a Smoldyn simulation

    Args:
        configuration_filename (:obj:`str`): path to the Smoldyn configuration file for the simulation
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation

    Returns:
        :obj:`SmoldynOutputFile`: output file
    '''
    out_dirname = os.path.dirname(configuration_filename)
    fid, filename = tempfile.mkstemp(dir=out_dirname, suffix='.ssv')
    os.close(fid)
    name = os.path.relpath(filename, out_dirname)
    smoldyn_simulation.setOutputFile(name, append=False)
    smoldyn_simulation.setOutputPath('./')
    return SmoldynOutputFile(name=name, filename=filename)


def add_commands_to_smoldyn_output_file(simulation, output_file, commands):
    ''' Add commands to a Smoldyn output file

    Args:
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation
        smoldyn_output_file (:obj:`SmoldynOutputFile`): Smoldyn output file
        commands (:obj:`list` of :obj`SmoldynCommand`): Smoldyn commands
    '''
    for command in commands:
        simulation.addCommand(command.command + ' ' + output_file.name, command.type)


def add_smoldyn_output_files_for_sed_variables(configuration_filename, sed_variables, smoldyn_simulation):
    ''' Add Smoldyn output files for capturing each SED variable

    =============================================================================================================================================  ===========================================================================================================================================  ===========================================
    Smoldyn output file                                                                                                                            SED variable target                                                                                                                          Shape
    =============================================================================================================================================  ===========================================================================================================================================  ===========================================
    ``molcount``                                                                                                                                   ``molcount {species}``                                                                                                                       (numberOfSteps + 1,)
    ``molcountspecies {species}({state})``                                                                                                         ``molcountspecies {species}({state})``                                                                                                       (numberOfSteps + 1,)
    ``molcountspecieslist {species}({state})+``                                                                                                    ``molcountspecies {species}({state})``                                                                                                       (numberOfSteps + 1,)
    ``molcountinbox {low-x} {hi-x}``                                                                                                               ``molcountinbox {species} {low-x} {hi-x}``                                                                                                   (numberOfSteps + 1,)
    ``molcountinbox {low-x} {hi-x} {low-y} {hi-y}``                                                                                                ``molcountinbox {species} {low-x} {hi-x} {low-y} {hi-y}``                                                                                    (numberOfSteps + 1,)
    ``molcountinbox {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z}``                                                                                 ``molcountinbox {species} {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z}``                                                                     (numberOfSteps + 1,)
    ``molcountincmpt {compartment}``                                                                                                               ``molcountincmpt {species} {compartment}``                                                                                                   (numberOfSteps + 1,)
    ``molcountincmpts {compartment}+``                                                                                                             ``molcountincmpt {species} {compartment}``                                                                                                   (numberOfSteps + 1,)
    ``molcountincmpt2 {compartment} {state}``                                                                                                      ``molcountincmpt2 {species} {compartment} {state}``                                                                                          (numberOfSteps + 1,)
    ``molcountonsurf {surface}``                                                                                                                   ``molcountonsurf {species} {surface}``                                                                                                       (numberOfSteps + 1,)
    ``molcountspace {species}({state}) {axis} {low} {hi} {bins} 0``                                                                                ``molcountspace {species}({state}) {axis} {low} {hi} {bins}``                                                                                (numberOfSteps + 1, bins)
    ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi} 0``                                                                     ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi}``                                                                     (numberOfSteps + 1, bins)
    ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi} {low} {hi} 0``                                                          ``molcountspace {species}({state}) {axis} {low} {hi} {bins} {low} {hi} {low} {hi}``                                                          (numberOfSteps + 1, bins)
    ``molcountspace2d {species}({state}) z {low-x} {hi-x} {bins-x} {low-y} {hi-y} {bins-y} 0``                                                     ``molcountspace2d {species}({state}) z {low-x} {hi-x} {bins-x} {low-y} {hi-y} {bins-y}``                                                     (numberOfSteps + 1, bins-x, bins-y)
    ``molcountspace2d {species}({state}) {axis} {low-1} {hi-1} {bins-1} {low-2} {hi-2} {bins-2} {low-3} {hi-3} 0``                                 ``molcountspace2d {species}({state}) {axis} {low-1} {hi-1} {bins-1} {low-2} {hi-2} {bins-3} {low-3} {hi-3}``                                 (numberOfSteps + 1, bins-1, bins-2)
    ``molcountspaceradial {species}({state}) {center-x} {radius} {bins} 0``                                                                        ``molcountspaceradial {species}({state}) {center-x} {radius} {bins}``                                                                        (numberOfSteps + 1, bins)
    ``molcountspaceradial {species}({state}) {center-x} {center-y} {radius} {bins} 0``                                                             ``molcountspaceradial {species}({state}) {center-x} {center-y} {radius} {bins}``                                                             (numberOfSteps + 1, bins)
    ``molcountspaceradial {species}({state}) {center-x} {center-y} {center-z} {radius} {bins} 0``                                                  ``molcountspaceradial {species}({state}) {center-x} {center-y} {center-z} {radius} {bins}``                                                  (numberOfSteps + 1, bins)
    ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {pole-x} {pole-y} {radius-min} {radius-max} {bins} 0``                      ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {pole-x} {pole-y} {radius-min} {radius-max} {bins}``                      (numberOfSteps + 1, bins)
    ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {center-z} {pole-x} {pole-y} {pole-z} {radius-min} {radius-max} {bins} 0``  ``molcountspacepolarangle {species}({state}) {center-x} {center-y} {center-z} {pole-x} {pole-y} {pole-z} {radius-min} {radius-max} {bins}``  (numberOfSteps + 1, bins)
    ``radialdistribution {species-1}({state-1}) {species-2}({state-2}) {radius} {bins} 0``                                                         ``radialdistribution {species-1}({state-1}) {species-2}({state-2}) {radius} {bins}``                                                         (numberOfSteps + 1, bins)
    ``radialdistribution2 {species-1}({state-1}) {species-2}({state-2}) {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z} {radius} {bins} 0``           ``radialdistribution2 {species-1}({state-1}) {species-2}({state-2}) {low-x} {hi-x} {low-y} {hi-y} {low-z} {hi-z} {radius} {bins}``           (numberOfSteps + 1, bins)
    =============================================================================================================================================   ==========================================================================================================================================  ===========================================

    Args:
        configuration_filename (:obj:`str`): path to the Smoldyn configuration file for the simulation
        sed_variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation

    Returns:
        :obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`: Smoldyn output files
    '''
    # TODO: support additional kinds of outputs

    # initialize dictionary of output files
    smoldyn_output_files = {}

    missing_symbols = []

    for sed_variable in sed_variables:
        if sed_variable.symbol:
            if sed_variable.symbol == Symbol.time.value:
                add_smoldyn_output_file_for_output(configuration_filename, smoldyn_simulation,
                                                   'molcount', True, smoldyn_output_files)
            else:
                missing_symbols.append('{}: {}'.format(sed_variable.id, sed_variable.symbol))

        else:
            output_command, _, output_args = re.sub(r' +', ' ', sed_variable.target).partition(' ')
            if output_command in ['molcount', 'molcountinbox', 'molcountincmpt', 'molcountincmpt2', 'molcountonsurf']:
                output_args = output_args.partition(' ')[2]
                add_smoldyn_output_file_for_output(configuration_filename, smoldyn_simulation,
                                                   output_command + ' ' + output_args, True, smoldyn_output_files)
            elif output_command in ['molcountspecies']:
                add_smoldyn_output_file_for_output(configuration_filename, smoldyn_simulation,
                                                   output_command + ' ' + output_args, False, smoldyn_output_files)

            elif output_command in ['molcountspace', 'molcountspace2d', 'molcountspaceradial',
                                    'molcountspacepolarangle', 'radialdistribution', 'radialdistribution2']:
                add_smoldyn_output_file_for_output(configuration_filename, smoldyn_simulation,
                                                   output_command + ' ' + output_args + ' 0', False, smoldyn_output_files)

            else:
                raise NotImplementedError('Output command `{}` is not supported.'.format(output_command))

    if missing_symbols:
        msg = '{} symbols cannot be recorded:\n  {}\n\nThe following symbols can be recorded:\n  symbol: {}'.format(
            len(missing_symbols),
            '\n  '.join(missing_symbols),
            Symbol.time.value,
        )
        raise ValueError(msg)

    # return output files
    return smoldyn_output_files


def add_smoldyn_output_file_for_output(configuration_filename, smoldyn_simulation,
                                       smoldyn_output_command, include_header, smoldyn_output_files):
    ''' Add a Smoldyn output file for molecule counts

    Args:
        configuration_filename (:obj:`str`): path to the Smoldyn configuration file for the simulation
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation
        smoldyn_output_command (:obj:`str`): Smoldyn output command (e.g., ``molcount``)
        include_header (:obj:`bool`): whether to include a header
        smoldyn_output_files (:obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`): Smoldyn output files
    '''
    smoldyn_output_command = smoldyn_output_command.strip()
    if smoldyn_output_command not in smoldyn_output_files:
        smoldyn_output_files[smoldyn_output_command] = add_smoldyn_output_file(configuration_filename, smoldyn_simulation)

        commands = [SmoldynCommand(smoldyn_output_command, 'E')]
        if include_header:
            commands.insert(0, SmoldynCommand('molcountheader', 'B'))

        add_commands_to_smoldyn_output_file(
            smoldyn_simulation,
            smoldyn_output_files[smoldyn_output_command],
            commands,
        )


def get_variable_results(number_of_steps, sed_variables, smoldyn_output_files):
    ''' Get the result of each SED variable

    Args:
        number_of_steps (:obj:`int`): number of steps
        sed_variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
        smoldyn_output_files (:obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`): Smoldyn output files

    Returns:
        :obj:`VariableResults`: result of each SED variable

    Raises:
        :obj:`ValueError`: unsupported results
    '''
    # TODO: support additional kinds of outputs
    smoldyn_results = {}

    missing_variables = []

    variable_results = VariableResults()
    for sed_variable in sed_variables:
        if sed_variable.symbol:
            if sed_variable.symbol == Symbol.time.value:
                variable_result = get_smoldyn_output('molcount', True, None, smoldyn_output_files, smoldyn_results)['time']
            else:
                variable_result = None
                missing_variables.append('{}: {}: {}'.format(sed_variable.id, 'symbol', sed_variable.symbol))
        else:
            output_command, _, output_args = re.sub(r' +', ' ', sed_variable.target).partition(' ')
            if output_command in ['molcount', 'molcountinbox', 'molcountincmpt', 'molcountincmpt2', 'molcountonsurf']:
                species_name, _, output_args = output_args.partition(' ')
                variable_result = get_smoldyn_output(output_command + ' ' + output_args, True, None,
                                                     smoldyn_output_files, smoldyn_results).get(species_name, None)
                if variable_result is None:
                    missing_variables.append('{}: {}: {}'.format(sed_variable.id, 'target', sed_variable.target))

            elif output_command in ['molcountspecies']:
                variable_result = get_smoldyn_output(output_command + ' ' + output_args, True, None,
                                                     smoldyn_output_files, smoldyn_results).iloc[:, 1]

            elif output_command in ['molcountspace', 'molcountspaceradial',
                                    'molcountspacepolarangle', 'radialdistribution', 'radialdistribution2']:
                variable_result = get_smoldyn_output(output_command + ' ' + output_args + ' 0', True, None,
                                                     smoldyn_output_files, smoldyn_results).iloc[:, 1:]

            elif output_command in ['molcountspace2d']:
                output_args_list = output_args.split(' ')
                if len(output_args_list) == 8:
                    shape = (int(output_args_list[-4]), int(output_args_list[-1]))
                else:
                    shape = (int(output_args_list[-6]), int(output_args_list[-3]))

                variable_result = get_smoldyn_output(output_command + ' ' + output_args + ' 0', True, shape,
                                                     smoldyn_output_files, smoldyn_results)

            else:
                raise NotImplementedError('Output command `{}` is not supported.'.format(output_command))

        if variable_result is not None:
            if variable_result.ndim == 1:
                variable_results[sed_variable.id] = variable_result.to_numpy()[-(number_of_steps + 1):, ]
            elif variable_result.ndim == 2:
                variable_results[sed_variable.id] = variable_result.to_numpy()[-(number_of_steps + 1):, :]
            else:
                variable_results[sed_variable.id] = variable_result[-(number_of_steps + 1):, :, :]

    if missing_variables:
        msg = '{} variables could not be recorded:\n  {}'.format(
            len(missing_variables),
            '\n  '.join(missing_variables),
        )
        raise ValueError(msg)

    return variable_results


def get_smoldyn_output(smoldyn_output_command, has_header, three_d_shape, smoldyn_output_files, smoldyn_results):
    ''' Get the simulated count of each molecule

    Args:
        smoldyn_output_command (:obj:`str`): Smoldyn output command (e.g., ``molcount``)
        has_header (:obj:`bool`): whether to include a header
        three_d_shape (:obj:`tuple` of :obj:`int`): dimensions of the output
        smoldyn_output_files (:obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`): Smoldyn output files
        smoldyn_results (:obj:`dict`)

    Returns:
        :obj:`pandas.DataFrame` or :obj:`numpy.ndarray`: results
    '''
    smoldyn_output_command = smoldyn_output_command.strip()
    if smoldyn_output_command not in smoldyn_results:
        smoldyn_output_file = smoldyn_output_files[smoldyn_output_command]
        if three_d_shape:
            with open(smoldyn_output_file.filename, 'r') as file:
                data_list = []

                i_line = 0
                for line in file:
                    if i_line % (three_d_shape[1] + 1) == 0:
                        time_point_data = []
                    else:
                        profile = [int(el) for el in line.split(' ')]
                        time_point_data.append(profile)

                        if i_line % (three_d_shape[1] + 1) == three_d_shape[1]:
                            data_list.append(time_point_data)

                    i_line += 1

            smoldyn_results[smoldyn_output_command] = numpy.array(data_list).transpose((0, 2, 1))

        else:
            smoldyn_results[smoldyn_output_command] = pandas.read_csv(smoldyn_output_file.filename, sep=' ')

    return smoldyn_results[smoldyn_output_command]
