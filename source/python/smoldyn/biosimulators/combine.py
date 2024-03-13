''' Methods for executing COMBINE archives and SED-ML files involving simulations of models encoded in Smoldyn

    Information about encoding Smoldyn simulations into SED-ML files and COMBINE archives is available at
    `https://github.com/ssandrews/Smoldyn/blob/master/Using-Smoldyn-with-SED-ML-COMBINE-BioSimulators.md <https://github.com/ssandrews/Smoldyn/blob/master/Using-Smoldyn-with-SED-ML-COMBINE-BioSimulators.md>`_.
'''

__author__ = 'Jonathan Karr'
__email__ = 'karr@mssm.edu'

__all__ = [
    'exec_sedml_docs_in_combine_archive',
    'exec_sed_task',
]

from .data_model import (SmoldynCommand, SmoldynOutputFile, SimulationChange, SimulationChangeExecution, AlgorithmParameterType,
                         KISAO_ALGORITHMS_MAP, KISAO_ALGORITHM_PARAMETERS_MAP)
from biosimulators_utils.combine.exec import exec_sedml_docs_in_archive
from biosimulators_utils.config import get_config, Config  # noqa: F401
from biosimulators_utils.log.data_model import CombineArchiveLog, TaskLog, StandardOutputErrorCapturerLevel  # noqa: F401
from biosimulators_utils.viz.data_model import VizFormat  # noqa: F401
from biosimulators_utils.report.data_model import ReportFormat, VariableResults, SedDocumentResults  # noqa: F401
from biosimulators_utils.sedml import validation
from biosimulators_utils.sedml.data_model import (Task, ModelLanguage, ModelAttributeChange,  # noqa: F401
                                                  UniformTimeCourseSimulation, AlgorithmParameterChange, Variable,
                                                  Symbol)
from biosimulators_utils.sedml.exec import exec_sed_doc as base_exec_sed_doc
from biosimulators_utils.utils.core import validate_str_value, parse_value, raise_errors_warnings
from biosimulators_simularium.exec import execute as exec_biosimularium
from smoldyn import smoldyn
import functools
import os
import numpy
import pandas
import re
import tempfile
import zipfile
import types  # noqa: F401

__all__ = ['exec_sedml_docs_in_combine_archive', 'exec_sed_task', 'exec_sed_doc', 'preprocess_sed_task']


def exec_sedml_docs_in_combine_archive(archive_filename, out_dir, config=None):
    ''' Execute the SED tasks defined in a COMBINE/OMEX archive and save the outputs

    Args:
        archive_filename (:obj:`str`): path to COMBINE/OMEX archive
        out_dir (:obj:`str`): path to store the outputs of the archive

            * CSV: directory in which to save outputs to files
              ``{ out_dir }/{ relative-path-to-SED-ML-file-within-archive }/{ report.id }.csv``
            * HDF5: directory in which to save a single HDF5 file (``{ out_dir }/reports.h5``),
              with reports at keys ``{ relative-path-to-SED-ML-file-within-archive }/{ report.id }`` within the HDF5 file

        config (:obj:`Config`, optional): BioSimulators common configuration

    Returns:
        :obj:`tuple`:

            * :obj:`SedDocumentResults`: results
            * :obj:`CombineArchiveLog`: log
    '''

    print('GENERATING A SIMULARIUM FILE ------------- ')
    # extract contents from archive
    temp_archive_root = tempfile.mkdtemp()
    with zipfile.ZipFile(archive_filename, 'r') as ref:
        ref.extractall(temp_archive_root)

    # process simularium file generation
    exec_biosimularium(
        working_dir=temp_archive_root,
        output_dir=out_dir,
        use_json=True
    )

    print('RUNNING A SEDML SIMULATION ------------------ ')
    # process sed result
    results, log = exec_sedml_docs_in_archive(exec_sed_doc, archive_filename, out_dir,
                                              apply_xml_model_changes=False,
                                              config=config)

    return results, log


def exec_sed_doc(doc, working_dir, base_out_path, rel_out_path=None,
                 apply_xml_model_changes=True,
                 log=None, indent=0, pretty_print_modified_xml_models=False,
                 log_level=StandardOutputErrorCapturerLevel.c, config=None):
    """ Execute the tasks specified in a SED document and generate the specified outputs

    Args:
        doc (:obj:`SedDocument` or :obj:`str`): SED document or a path to SED-ML file which defines a SED document
        working_dir (:obj:`str`): working directory of the SED document (path relative to which models are located)

        base_out_path (:obj:`str`): path to store the outputs

            * CSV: directory in which to save outputs to files
              ``{base_out_path}/{rel_out_path}/{report.id}.csv``
            * HDF5: directory in which to save a single HDF5 file (``{base_out_path}/reports.h5``),
              with reports at keys ``{rel_out_path}/{report.id}`` within the HDF5 file

        rel_out_path (:obj:`str`, optional): path relative to :obj:`base_out_path` to store the outputs
        apply_xml_model_changes (:obj:`bool`, optional): if :obj:`True`, apply any model changes specified in the SED-ML file before
            calling :obj:`task_executer`.
        log (:obj:`SedDocumentLog`, optional): log of the document
        indent (:obj:`int`, optional): degree to indent status messages
        pretty_print_modified_xml_models (:obj:`bool`, optional): if :obj:`True`, pretty print modified XML models
        log_level (:obj:`StandardOutputErrorCapturerLevel`, optional): level at which to log output
        config (:obj:`Config`, optional): BioSimulators common configuration
        simulator_config (:obj:`SimulatorConfig`, optional): tellurium configuration

    Returns:
        :obj:`tuple`:

            * :obj:`ReportResults`: results of each report
            * :obj:`SedDocumentLog`: log of the document
    """
    return base_exec_sed_doc(exec_sed_task, doc, working_dir, base_out_path,
                             rel_out_path=rel_out_path,
                             apply_xml_model_changes=apply_xml_model_changes,
                             log=log,
                             indent=indent,
                             pretty_print_modified_xml_models=pretty_print_modified_xml_models,
                             log_level=log_level,
                             config=config)


def exec_sed_task(task, variables, preprocessed_task=None, log=None, config=None):
    ''' Execute a task and save its results

    Args:
       task (:obj:`Task`): task
       variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
       preprocessed_task (:obj:`dict`, optional): preprocessed information about the task, including possible
            model changes and variables. This can be used to avoid repeatedly executing the same initialization
            for repeated calls to this method.
       log (:obj:`TaskLog`, optional): log for the task
       config (:obj:`Config`, optional): BioSimulators common configuration

    Returns:
        :obj:`tuple`:

            :obj:`VariableResults`: results of variables
            :obj:`TaskLog`: log
    '''
    if not config:
        config = get_config()

    if config.LOG and not log:
        log = TaskLog()

    sed_model_changes = task.model.changes
    sed_simulation = task.simulation

    if preprocessed_task is None:
        preprocessed_task = preprocess_sed_task(task, variables, config=config)
        sed_model_changes = list(filter(lambda change: change.target in preprocessed_task['sed_smoldyn_simulation_change_map'],
                                        sed_model_changes))

    # read Smoldyn configuration
    smoldyn_simulation = preprocessed_task['simulation']

    # apply model changes to the Smoldyn configuration
    sed_smoldyn_simulation_change_map = preprocessed_task['sed_smoldyn_simulation_change_map']
    for change in sed_model_changes:
        smoldyn_change = sed_smoldyn_simulation_change_map.get(change.target, None)
        if smoldyn_change is None or smoldyn_change.execution != SimulationChangeExecution.simulation:
            raise NotImplementedError('Target `{}` can only be changed during simulation preprocessing.'.format(change.target))
        apply_change_to_smoldyn_simulation(
            smoldyn_simulation, change, smoldyn_change)

    # get the Smoldyn representation of the SED uniform time course simulation
    smoldyn_simulation_run_timecourse_args = get_smoldyn_run_timecourse_args(sed_simulation)

    # execute the simulation
    smoldyn_run_args = dict(
        **smoldyn_simulation_run_timecourse_args,
        **preprocessed_task['simulation_run_alg_param_args'],
    )
    smoldyn_simulation.run(**smoldyn_run_args, overwrite=True, display=False, quit_at_end=False)

    # get the result of each SED variable
    variable_output_cmd_map = preprocessed_task['variable_output_cmd_map']
    smoldyn_output_files = preprocessed_task['output_files']
    variable_results = get_variable_results(sed_simulation.number_of_steps, variables, variable_output_cmd_map, smoldyn_output_files)

    # cleanup output files
    for smoldyn_output_file in smoldyn_output_files.values():
        os.remove(smoldyn_output_file.filename)

    # log simulation
    if config.LOG:
        log.algorithm = sed_simulation.algorithm.kisao_id
        log.simulator_details = {
            'class': 'smoldyn.Simulation',
            'instanceAttributes': preprocessed_task['simulation_attrs'],
            'method': 'run',
            'methodArguments': smoldyn_run_args,
        }

    # return the values of the variables and log
    return variable_results, log


def preprocess_sed_task(task, variables, config=None):
    """ Preprocess a SED task, including its possible model changes and variables. This is useful for avoiding
    repeatedly initializing tasks on repeated calls of :obj:`exec_sed_task`.

    Args:
        task (:obj:`Task`): task
        variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
        config (:obj:`Config`, optional): BioSimulators common configuration

    Returns:
        :obj:`dict`: preprocessed information about the task
    """
    config = config or get_config()

    sed_model = task.model
    sed_simulation = task.simulation

    if config.VALIDATE_SEDML:
        raise_errors_warnings(validation.validate_task(task),
                              error_summary='Task `{}` is invalid.'.format(task.id))
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
        raise_errors_warnings(*validation.validate_data_generator_variables(variables),
                              error_summary='Data generator variables for task `{}` are invalid.'.format(task.id))

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

    # preprocess model changes
    sed_smoldyn_preprocessing_change_map = {}
    sed_smoldyn_simulation_change_map = {}
    for change in sed_model.changes:
        smoldyn_change = validate_model_change(change)

        if smoldyn_change.execution == SimulationChangeExecution.preprocessing:
            sed_smoldyn_preprocessing_change_map[change] = smoldyn_change
        else:
            sed_smoldyn_simulation_change_map[change.target] = smoldyn_change

    # apply preprocessing-time changes
    for change, smoldyn_change in sed_smoldyn_preprocessing_change_map.items():
        apply_change_to_smoldyn_simulation_configuration(
            simulation_configuration, change, smoldyn_change)

    # write the modified Smoldyn configuration to a temporary file
    fid, smoldyn_configuration_filename = tempfile.mkstemp(suffix='.txt')
    os.close(fid)
    write_smoldyn_simulation_configuration(simulation_configuration, smoldyn_configuration_filename)

    # initialize a simulation from the Smoldyn file
    smoldyn_simulation = init_smoldyn_simulation_from_configuration_file(smoldyn_configuration_filename)

    # clean up temporary file
    os.remove(smoldyn_configuration_filename)

    # apply the SED algorithm parameters to the Smoldyn simulation and to the arguments to its ``run`` method
    smoldyn_simulation_attrs = {}
    smoldyn_simulation_run_alg_param_args = {}
    for sed_algorithm_parameter_change in sed_simulation.algorithm.changes:
        val = get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(sed_algorithm_parameter_change)
        if val['type'] == AlgorithmParameterType.run_argument:
            smoldyn_simulation_run_alg_param_args[val['name']] = val['value']
        else:
            smoldyn_simulation_attrs[val['name']] = val['value']

    # apply the SED algorithm parameters to the Smoldyn simulation and to the arguments to its ``run`` method
    for attr_name, value in smoldyn_simulation_attrs.items():
        setter = getattr(smoldyn_simulation, attr_name)
        setter(value)

    # validate SED variables
    variable_output_cmd_map = validate_variables(variables)

    # Setup Smoldyn output files for the SED variables
    smoldyn_configuration_dirname = os.path.dirname(smoldyn_configuration_filename)
    smoldyn_output_files = add_smoldyn_output_files_for_sed_variables(
        smoldyn_configuration_dirname, variables, variable_output_cmd_map, smoldyn_simulation)

    # return preprocessed information
    return {
        'simulation': smoldyn_simulation,
        'simulation_attrs': smoldyn_simulation_attrs,
        'simulation_run_alg_param_args': smoldyn_simulation_run_alg_param_args,
        'sed_smoldyn_simulation_change_map': sed_smoldyn_simulation_change_map,
        'variable_output_cmd_map': variable_output_cmd_map,
        'output_files': smoldyn_output_files,
    }


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


def validate_model_change(sed_model_change):
    ''' Validate a SED model attribute change to a configuration for a Smoldyn simulation

    ====================================================================  ===================
    target                                                                newValue
    ====================================================================  ===================
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
    ``killmol {species}({state})``                                        0
    ``killmolprob {species}({state}) {prob}``                             0
    ``killmolincmpt {species}({state}) {compartment}``                    0
    ``killmolinsphere {species}({state}) {surface}``                      0
    ``killmoloutsidesystem {species}({state})``                           0
    ``fixmolcount {species}({state})``                                    integer
    ``fixmolcountincmpt {species}({staet}) {compartment}``                integer
    ``fixmolcountonsurf {species}({state}) {surface}``                    integer
    ====================================================================  ===================

    Args:
        sed_model_change (:obj:`ModelAttributeChange`): SED model change

    Returns:
        :obj:`SimulationChange`: Smoldyn representation of the model change

    Raises:
        :obj:`NotImplementedError`: unsupported model change
    '''
    # TODO: support additional types of model changes

    target_type, _, target = sed_model_change.target.strip().partition(' ')
    target_type = target_type.strip()
    target = re.sub(r' +', ' ', target).strip()

    if target_type in [
        'killmol', 'killmolprob', 'killmolinsphere', 'killmolincmpt', 'killmoloutsidesystem',
    ]:
        # Examples:
        #   killmol red
        def new_line_func(new_value):
            return target_type + ' ' + target
        execution = SimulationChangeExecution.simulation

    elif target_type in [
        'fixmolcount', 'fixmolcountonsurf', 'fixmolcountincmpt',
    ]:
        # Examples:
        #   fixmolcount red
        species_name, species_target_sep, target = target.partition(' ')

        def new_line_func(new_value):
            return target_type + ' ' + species_name + ' ' + new_value + species_target_sep + target

        execution = SimulationChangeExecution.simulation

    elif target_type in [
        'define', 'difc', 'difc_rule', 'difm', 'difm_rule',
        'drift', 'drift_rule', 'surface_drift', 'surface_drift_rule',
    ]:
        # Examples:
        #   define K_FWD 0.001
        #   difc S 3
        #   difm red 1 0 0 0 0 0 0 0 2
        def new_line_func(new_value):
            return target_type + ' ' + target + ' ' + new_value

        execution = SimulationChangeExecution.preprocessing

    # elif target_type in [
    #     'mol', 'compartment_mol', 'surface_mol',
    # ]:
    #     # Examples:
    #     #   mol 5 red u
    #     #   compartment_mol 500 S inside
    #     #   surface_mol 100 E(front) membrane all all
    #     def new_line_func(new_value):
    #         return target_type + ' ' + new_value + ' ' + target
    #
    #     execution = SimulationChangeExecution.preprocessing

    # elif target_type in [
    #     'dim',
    # ]:
    #     # Examples:
    #     #   dim 1
    #     def new_line_func(new_value):
    #         return target_type + ' ' + new_value
    #
    #     execution = SimulationChangeExecution.preprocessing

    # elif target_type in [
    #     'boundaries', 'low_wall', 'high_wall',
    # ]:
    #     # Examples:
    #     #   low_wall x -10
    #     #   high_wall y 10
    #     def new_line_func(new_value):
    #         return target_type + ' ' + target + ' ' + new_value
    #
    #     execution = SimulationChangeExecution.preprocessing

    else:
        raise NotImplementedError('Target `{}` is not supported.'.format(sed_model_change.target))

    return SimulationChange(new_line_func, execution)


def apply_change_to_smoldyn_simulation(smoldyn_simulation, sed_change, smoldyn_change):
    ''' Apply a SED model attribute change to a Smoldyn simulation

    Args:
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation
        sed_change (:obj:`ModelAttributeChange`): SED model change
        smoldyn_change (:obj:`SimulationChange`): Smoldyn representation of the model change
    '''
    new_value = str(sed_change.new_value).strip()
    new_line = smoldyn_change.command(new_value)
    smoldyn_simulation.addCommand(new_line, 'b')


def apply_change_to_smoldyn_simulation_configuration(smoldyn_simulation_configuration, sed_change, smoldyn_change):
    ''' Apply a SED model attribute change to a configuration for a Smoldyn simulation

    Args:
        smoldyn_simulation_configuration (:obj:`list` of :obj:`str`): configuration for the Smoldyn simulation
        sed_change (:obj:`ModelAttributeChange`): SED model change
        smoldyn_change (:obj:`SimulationChange`): Smoldyn representation of the model change
    '''
    new_value = str(sed_change.new_value).strip()
    new_line = smoldyn_change.command(new_value)
    smoldyn_simulation_configuration.insert(0, new_line)


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


def add_smoldyn_output_file(configuration_dirname, smoldyn_simulation):
    ''' Add an output file to a Smoldyn simulation

    Args:
        configuration_dirname (:obj:`str`): path to the parent directory of the Smoldyn configuration file for the simulation
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation

    Returns:
        :obj:`SmoldynOutputFile`: output file
    '''
    fid, filename = tempfile.mkstemp(dir=configuration_dirname, suffix='.ssv')
    os.close(fid)
    name = os.path.relpath(filename, configuration_dirname)
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


def validate_variables(variables):
    ''' Validate SED variables

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
        variables (:obj:`list` of :obj:`Variable`): variables that should be recorded

    Returns:
        :obj:`dict`: dictionary that maps variable targets and symbols to Smoldyn output commands
    '''
    # TODO: support additional kinds of outputs

    variable_output_cmd_map = {}

    invalid_symbols = []
    invalid_targets = []

    for variable in variables:
        if variable.symbol:
            if variable.symbol == Symbol.time.value:
                output_command_args = 'molcount'
                include_header = True
                shape = None
                results_slicer = functools.partial(results_key_slicer, key='time')

            else:
                invalid_symbols.append('{}: {}'.format(variable.id, variable.symbol))
                output_command_args = None
                include_header = None

        else:
            output_command, _, output_args = re.sub(r' +', ' ', variable.target).partition(' ')

            if output_command in ['molcount', 'molcountinbox', 'molcountincmpt', 'molcountincmpt2', 'molcountonsurf']:
                species_name, _, output_args = output_args.partition(' ')
                output_command_args = output_command + ' ' + output_args
                include_header = True
                shape = None
                results_slicer = functools.partial(results_key_slicer, key=species_name)

            elif output_command in ['molcountspecies']:
                output_command_args = output_command + ' ' + output_args
                include_header = False
                shape = None
                results_slicer = results_array_slicer

            elif output_command in ['molcountspace', 'molcountspaceradial',
                                    'molcountspacepolarangle', 'radialdistribution', 'radialdistribution2']:
                output_command_args = output_command + ' ' + output_args + ' 0'
                include_header = False
                shape = None
                results_slicer = results_matrix_slicer

            elif output_command in ['molcountspace2d']:
                output_command_args = output_command + ' ' + output_args + ' 0'
                include_header = False
                output_args_list = output_args.split(' ')
                if len(output_args_list) == 8:
                    shape = (int(output_args_list[-4]), int(output_args_list[-1]))
                else:
                    shape = (int(output_args_list[-6]), int(output_args_list[-3]))
                results_slicer = None

            else:
                invalid_targets.append('{}: {}'.format(variable.id, variable.target))
                output_command_args = None

        if output_command_args is not None:
            output_command_args = output_command_args.strip()
            variable_output_cmd_map[(variable.target, variable.symbol)] = (output_command_args, include_header, shape, results_slicer)

    if invalid_symbols:
        msg = '{} symbols cannot be recorded:\n  {}\n\nThe following symbols can be recorded:\n  {}'.format(
            len(invalid_symbols),
            '\n  '.join(sorted(invalid_symbols)),
            '\n  '.join(sorted([Symbol.time.value])),
        )
        raise ValueError(msg)

    if invalid_targets:
        valid_target_output_commands = [
            'molcount',

            'molcount', 'molcountinbox', 'molcountincmpt', 'molcountincmpt2', 'molcountonsurf',

            'molcountspace', 'molcountspaceradial',
            'molcountspacepolarangle', 'radialdistribution', 'radialdistribution2',

            'molcountspace2d',
        ]

        msg = '{} targets cannot be recorded:\n  {}\n\nTargets are supported for the following output commands:\n  {}'.format(
            len(invalid_targets),
            '\n  '.join(sorted(invalid_targets)),
            '\n  '.join(sorted(set(valid_target_output_commands))),
        )
        raise NotImplementedError(msg)

    return variable_output_cmd_map


def results_key_slicer(results, key):
    """ Get the results for a key from a set of results

    Args:
        results (:obj:`pandas.DataFrame`): set of results

    Returns:
        :obj:`pandas.DataFrame`: results for a key
    """
    return results.get(key, None)


def results_array_slicer(results):
    """ Extract an array of results from a matrix of time and results

    Args:
        results (:obj:`pandas.DataFrame`): matrix of time and results

    Returns:
        :obj:`pandas.DataFrame`: results
    """
    return results.iloc[:, 1]


def results_matrix_slicer(results):
    """ Extract a matrix array of results from a matrix of time and results

    Args:
        results (:obj:`pandas.DataFrame`): matrix of time and results

    Returns:
        :obj:`pandas.DataFrame`: results
    """
    return results.iloc[:, 1:]


def add_smoldyn_output_files_for_sed_variables(configuration_dirname, variables, variable_output_cmd_map, smoldyn_simulation):
    ''' Add Smoldyn output files for capturing each SED variable

    Args:
        configuration_dirname (:obj:`str`): path to the parent directory of the Smoldyn configuration file for the simulation
        variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
        variable_output_cmd_map (:obj:`dict`): dictionary that maps variable targets and symbols to Smoldyn output commands
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation

    Returns:
        :obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`: Smoldyn output files
    '''
    smoldyn_output_files = {}

    output_cmds = set()
    for variable in variables:
        output_cmds.add(variable_output_cmd_map[(variable.target, variable.symbol)])

    for command, include_header, _, _ in output_cmds:
        add_smoldyn_output_file_for_output(configuration_dirname, smoldyn_simulation,
                                           command, include_header,
                                           smoldyn_output_files)
    # return output files
    return smoldyn_output_files


def add_smoldyn_output_file_for_output(configuration_dirname, smoldyn_simulation,
                                       smoldyn_output_command, include_header, smoldyn_output_files):
    ''' Add a Smoldyn output file for molecule counts

    Args:
        configuration_dirname (:obj:`str`): path to the parent directory of the Smoldyn configuration file for the simulation
        smoldyn_simulation (:obj:`smoldyn.Simulation`): Smoldyn simulation
        smoldyn_output_command (:obj:`str`): Smoldyn output command (e.g., ``molcount``)
        include_header (:obj:`bool`): whether to include a header
        smoldyn_output_files (:obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`): Smoldyn output files
    '''
    smoldyn_output_files[smoldyn_output_command] = add_smoldyn_output_file(configuration_dirname, smoldyn_simulation)

    commands = []
    if include_header:
        commands.append(SmoldynCommand('molcountheader', 'B'))
    commands.append(SmoldynCommand(smoldyn_output_command, 'E'))

    add_commands_to_smoldyn_output_file(
        smoldyn_simulation,
        smoldyn_output_files[smoldyn_output_command],
        commands,
    )


def get_variable_results(number_of_steps, variables, variable_output_cmd_map, smoldyn_output_files):
    ''' Get the result of each SED variable

    Args:
        number_of_steps (:obj:`int`): number of steps
        variables (:obj:`list` of :obj:`Variable`): variables that should be recorded
        variable_output_cmd_map (:obj:`dict`): dictionary that maps variable targets and symbols to Smoldyn output commands
        smoldyn_output_files (:obj:`dict` of :obj:`str` => :obj:`SmoldynOutputFile`): Smoldyn output files

    Returns:
        :obj:`VariableResults`: result of each SED variable

    Raises:
        :obj:`ValueError`: unsupported results
    '''
    smoldyn_results = {}

    missing_variables = []

    variable_results = VariableResults()
    for variable in variables:
        output_command_args, _, shape, results_slicer = variable_output_cmd_map[(variable.target, variable.symbol)]
        variable_result = get_smoldyn_output(output_command_args, True, shape, smoldyn_output_files, smoldyn_results)
        if results_slicer:
            variable_result = results_slicer(variable_result)

        if variable_result is None:
            missing_variables.append('{}: {}: {}'.format(variable.id, 'target', variable.target))
        else:
            if variable_result.ndim == 1:
                variable_results[variable.id] = variable_result.to_numpy()[-(number_of_steps + 1):, ]
            elif variable_result.ndim == 2:
                variable_results[variable.id] = variable_result.to_numpy()[-(number_of_steps + 1):, :]
            else:
                variable_results[variable.id] = variable_result[-(number_of_steps + 1):, :, :]

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
