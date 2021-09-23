__author__ = "Jonathan Karr"
__email__ = "karr@mssm.edu"

try:
    import biosimulators_utils
    from biosimulators_utils.combine.data_model import CombineArchive, CombineArchiveContent, CombineArchiveContentFormat
    from biosimulators_utils.combine.io import CombineArchiveWriter
    from biosimulators_utils.config import get_config
    from biosimulators_utils.report.data_model import ReportFormat
    from biosimulators_utils.report.io import ReportReader
    from biosimulators_utils.sedml.data_model import (
        SedDocument,
        Model, ModelLanguage, ModelAttributeChange,
        UniformTimeCourseSimulation, Algorithm, AlgorithmParameterChange,
        Task, DataGenerator, Report, DataSet, Variable, Symbol)
    from biosimulators_utils.sedml.io import SedmlSimulationWriter
    from smoldyn.biosimulators.data_model import Simulation, SimulationInstruction
    from smoldyn.biosimulators.utils import read_simulation, _read_simulation_line
except ModuleNotFoundError:
    biosimulators_utils = None
from unittest import mock
import datetime
import dateutil.tz
import flaky
import numpy
import numpy.testing
import os
import shutil
import smoldyn
try:
    import smoldyn.biosimulators.__main__
    import smoldyn.biosimulators.combine
except ModuleNotFoundError:
    biosimulators_utils = None
import tempfile
import unittest


@unittest.skipIf(biosimulators_utils is None, "BioSimulators-utils must be installed")
class BioSimulatorsUtilsTestCase(unittest.TestCase):
    EXAMPLES_DIRNAME = os.path.join(os.path.dirname(__file__), '..', 'examples')

    def test__read_simulation_line(self):
        sim = Simulation()

        _read_simulation_line('dim 3', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='number_dimensions',
            description='Number of dimensions',
            macro='dim',
            arguments='3',
        )))

        _read_simulation_line('low_wall x -100 p', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='low_x_wall_1',
            description='Low x wall 1',
            macro='low_wall x',
            arguments='-100 p',
        )))

        _read_simulation_line('high_wall x -100 p', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='high_x_wall_1',
            description='High x wall 1',
            macro='high_wall x',
            arguments='-100 p',
        )))

        _read_simulation_line('boundaries x 0 1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='x_boundary',
            description='X boundary',
            macro='boundaries x',
            arguments='0 1',
        )))

        _read_simulation_line('define SYSLENGTH 50', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='value_parameter_SYSLENGTH',
            description='Value of parameter "SYSLENGTH"',
            macro='define SYSLENGTH',
            arguments='50',
        )))

        _read_simulation_line('difc all 1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='diffusion_coefficient_species_all',
            description='Diffusion coefficient of species "all"',
            macro='difc all',
            arguments='1',
        )))

        _read_simulation_line('difc all(x) 1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='diffusion_coefficient_species_all_state_x',
            description='Diffusion coefficient of species "all" in state "x"',
            macro='difc all(x)',
            arguments='1',
        )))

        _read_simulation_line('difc_rule Prot* 22', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='diffusion_coefficient_rule_species_Prot_',
            description='Diffusion coefficient rule for species "Prot*"',
            macro='difc_rule Prot*',
            arguments='22',
        )))

        _read_simulation_line('difc_rule Prot*(x) 22', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='diffusion_coefficient_rule_species_Prot__state_x',
            description='Diffusion coefficient rule for species "Prot*" in state "x"',
            macro='difc_rule Prot*(x)',
            arguments='22',
        )))

        _read_simulation_line('difm red 1 0 0 0 0 0 0 0 2', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='membrane_diffusion_coefficient_species_red',
            description='Membrane diffusion coefficient of species "red"',
            macro='difm red',
            arguments='1 0 0 0 0 0 0 0 2',
        )))

        _read_simulation_line('difm red(x) 1 0 0 0 0 0 0 0 2', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='membrane_diffusion_coefficient_species_red_state_x',
            description='Membrane diffusion coefficient of species "red" in state "x"',
            macro='difm red(x)',
            arguments='1 0 0 0 0 0 0 0 2',
        )))

        _read_simulation_line('difm_rule red* 1 0 0 0 0 0 0 0 2', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='membrane_diffusion_coefficient_rule_species_red_',
            description='Membrane diffusion coefficient rule for species "red*"',
            macro='difm_rule red*',
            arguments='1 0 0 0 0 0 0 0 2',
        )))

        _read_simulation_line('difm_rule red*(x) 1 0 0 0 0 0 0 0 2', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='membrane_diffusion_coefficient_rule_species_red__state_x',
            description='Membrane diffusion coefficient rule for species "red*" in state "x"',
            macro='difm_rule red*(x)',
            arguments='1 0 0 0 0 0 0 0 2',
        )))

        _read_simulation_line('drift red 0 0 0', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='drift_species_red',
            description='Drift of species "red"',
            macro='drift red',
            arguments='0 0 0',
        )))

        _read_simulation_line('drift red(x) 0 0 0', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='drift_species_red_state_x',
            description='Drift of species "red" in state "x"',
            macro='drift red(x)',
            arguments='0 0 0',
        )))

        _read_simulation_line('drift_rule red* 0 0 0', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='drift_rule_species_red_',
            description='Drift rule for species "red*"',
            macro='drift_rule red*',
            arguments='0 0 0',
        )))

        _read_simulation_line('drift_rule red*(x) 0 0 0', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='drift_rule_species_red__state_x',
            description='Drift rule for species "red*" in state "x"',
            macro='drift_rule red*(x)',
            arguments='0 0 0',
        )))

        _read_simulation_line('surface_drift red surf1 all 0.1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='surface_drift_species_red_surface_surf1_shape_all',
            description='Surface drift of species "red" on surface "surf1" with panel shape "all"',
            macro='surface_drift red surf1 all',
            arguments='0.1',
        )))

        _read_simulation_line('surface_drift red(x) surf1 all 0.1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='surface_drift_species_red_state_x_surface_surf1_shape_all',
            description='Surface drift of species "red" in state "x" on surface "surf1" with panel shape "all"',
            macro='surface_drift red(x) surf1 all',
            arguments='0.1',
        )))

        _read_simulation_line('surface_drift_rule red* surf1 all 0.1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='surface_drift_rule_species_red__surface_surf1_panel_all',
            description='Surface drift rule for species "red*" on surface "surf1" of panel shape "all"',
            macro='surface_drift_rule red* surf1 all',
            arguments='0.1',
        )))

        _read_simulation_line('surface_drift_rule red*(x) surf1 all 0.1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='surface_drift_rule_species_red__state_x_surface_surf1_panel_all',
            description='Surface drift rule for species "red*" in state "x" on surface "surf1" of panel shape "all"',
            macro='surface_drift_rule red*(x) surf1 all',
            arguments='0.1',
        )))

        _read_simulation_line('mol 167 A 5 u u', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='initial_count_species_A_5_u_u',
            description='Initial count of species "A 5 u u"',
            macro='mol A 5 u u',
            arguments='167',
        )))

        _read_simulation_line('compartment_mol 500 red intersection', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='initial_count_species_red_intersection',
            description='Initial count of species "red intersection"',
            macro='compartment_mol red intersection',
            arguments='500',
        )))

        _read_simulation_line('surface_mol 100 red(up) all rect r1', {}, sim)
        self.assertTrue(sim.instructions[-1].is_equal(SimulationInstruction(
            id='initial_count_species_red_up__all_rect_r1',
            description='Initial count of species "red(up) all rect r1"',
            macro='surface_mol red(up) all rect r1',
            arguments='100',
        )))

    def test_read_simulation(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        sim = read_simulation(filename)
        self.assertEqual(sim.species, ['red', 'green'])
        self.assertEqual(sim.compartments, [])
        self.assertEqual(sim.surfaces, [])

        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S9_compartments', 'compart.txt')
        sim = read_simulation(filename)
        self.assertEqual(sim.species, ['red', 'green'])
        self.assertEqual(sim.compartments, ['middle'])
        self.assertEqual(sim.surfaces, ['walls', 'surf'])


@unittest.skipIf(biosimulators_utils is None, "BioSimulators-utils must be installed")
class BioSimulatorsCombineTestCase(unittest.TestCase):
    EXAMPLES_DIRNAME = os.path.join(os.path.dirname(__file__), '..', 'examples')

    def setUp(self):
        self.dirname = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.dirname)

    def test_SmoldynOutputFile(self):
        output_file = smoldyn.biosimulators.data_model.SmoldynOutputFile('out.txt', '/tmp/out.txt')
        self.assertEqual(output_file.name, 'out.txt')
        self.assertEqual(output_file.filename, '/tmp/out.txt')

    def test_SmoldynCommand(self):
        output_file = smoldyn.biosimulators.data_model.SmoldynCommand('molcount', 'E')
        self.assertEqual(output_file.command, 'molcount')
        self.assertEqual(output_file.type, 'E')

    def test_init_smoldyn_simulation_from_configuration_file(self):
        sim = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(
            os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt'))
        self.assertIsInstance(sim, smoldyn.Simulation)

        with self.assertRaises(FileNotFoundError):
            smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file('not a file')

        with self.assertRaisesRegex(ValueError, 'Error: '):
            smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(
                os.path.join(self.EXAMPLES_DIRNAME, 'CMakeLists.txt'))

    def test_read_write_smoldyn_simulation_configuration(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        self.assertEqual(config[2], 'graphics opengl')
        self.assertEqual(config[-6], 'mol 2 green 50')

        filename2 = os.path.join(self.dirname, 'config.txt')
        smoldyn.biosimulators.combine.write_smoldyn_simulation_configuration(config, filename2)

        config2 = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename2)
        self.assertEqual(config2, config)

    def test_normalize_smoldyn_simulation_configuration(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        self.assertEqual(len(config), 30)

        smoldyn.biosimulators.combine.normalize_smoldyn_simulation_configuration(config)

        self.assertEqual(len(config), 25)

        self.assertEqual(config[2], 'graphics opengl')
        self.assertEqual(config[-1], 'mol 2 green 50')

        config[0] = '#'
        smoldyn.biosimulators.combine.normalize_smoldyn_simulation_configuration(config)
        self.assertEqual(len(config), 23)

        config[0] = 'graphics  opengl###comment   #comment2'
        smoldyn.biosimulators.combine.normalize_smoldyn_simulation_configuration(config)
        self.assertEqual(config[0], 'graphics opengl # comment   #comment2')

        config[0] = 'graphics  opengl##  '
        smoldyn.biosimulators.combine.normalize_smoldyn_simulation_configuration(config)
        self.assertEqual(config[0], 'graphics opengl')

        config[0] = '#comment   comment2'
        smoldyn.biosimulators.combine.normalize_smoldyn_simulation_configuration(config)
        self.assertEqual(config[0], '# comment   comment2')

        config[0] = 'graphics  opengl  '
        smoldyn.biosimulators.combine.normalize_smoldyn_simulation_configuration(config)
        self.assertEqual(config[0], 'graphics opengl')

    def test_disable_smoldyn_graphics_in_simulation_configuration(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        self.assertEqual(config[2], 'graphics opengl')

        smoldyn.biosimulators.combine.disable_smoldyn_graphics_in_simulation_configuration(config)
        self.assertEqual(config[2], 'graphics none')

    def test_apply_change_to_smoldyn_simulation(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        smoldyn.biosimulators.combine.disable_smoldyn_graphics_in_simulation_configuration(config)
        filename2 = os.path.join(self.dirname, 'config.txt')
        smoldyn.biosimulators.combine.write_smoldyn_simulation_configuration(config, filename2)

        smoldyn_simulation = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        change = ModelAttributeChange(target='define K_1', new_value='0')
        preprocessed_change = smoldyn.biosimulators.combine.validate_model_change(change)
        smoldyn.biosimulators.combine.apply_change_to_smoldyn_simulation_configuration(config, change, preprocessed_change)

        smoldyn_simulation = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        change = ModelAttributeChange(target='killmol red', new_value='0')
        preprocessed_change = smoldyn.biosimulators.combine.validate_model_change(change)
        smoldyn.biosimulators.combine.apply_change_to_smoldyn_simulation(smoldyn_simulation, change, preprocessed_change)

        smoldyn_simulation = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        change = ModelAttributeChange(target='fixmolcount green', new_value='10')
        preprocessed_change = smoldyn.biosimulators.combine.validate_model_change(change)
        smoldyn.biosimulators.combine.apply_change_to_smoldyn_simulation(smoldyn_simulation, change, preprocessed_change)

        smoldyn_simulation = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        change = ModelAttributeChange(target='fixmolcountincmpt green  cytosol', new_value='10')
        preprocessed_change = smoldyn.biosimulators.combine.validate_model_change(change)
        smoldyn.biosimulators.combine.apply_change_to_smoldyn_simulation(smoldyn_simulation, change, preprocessed_change)

        change = ModelAttributeChange(target=' dim  ', new_value=' 5 ')
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.validate_model_change(change)

    def test_get_smoldyn_run_timecourse_args(self):
        sim = UniformTimeCourseSimulation(
            initial_time=10.,
            output_start_time=20.,
            output_end_time=30.,
            number_of_points=10,
        )

        self.assertEqual(smoldyn.biosimulators.combine.get_smoldyn_run_timecourse_args(sim), {
            'start': 10.,
            'stop': 30.,
            'dt': 1.,
        })

        sim.output_end_time += 0.01
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.get_smoldyn_run_timecourse_args(sim)

    def test_get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(self):
        change = AlgorithmParameterChange(kisao_id='KISAO_0000254', new_value='5')
        param = smoldyn.biosimulators.combine.get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(change)
        self.assertEqual(param, {
            'name': 'accuracy',
            'type': smoldyn.biosimulators.combine.AlgorithmParameterType.run_argument,
            'value': 5.,
        })

        change = AlgorithmParameterChange(kisao_id='KISAO_0000488', new_value='10')
        param = smoldyn.biosimulators.combine.get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(change)
        self.assertEqual(param, {
            'name': 'setRandomSeed',
            'type': smoldyn.biosimulators.combine.AlgorithmParameterType.instance_attribute,
            'value': 10,
        })

        change = AlgorithmParameterChange(kisao_id='KISAO_0000488', new_value='not a float')
        with self.assertRaises(ValueError):
            smoldyn.biosimulators.combine.get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(change)

        change = AlgorithmParameterChange(kisao_id='KISAO_0000001', new_value='not a float')
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.get_smoldyn_instance_attr_or_run_algorithm_parameter_arg(change)

    def test_add_smoldyn_output_file(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        smoldyn.biosimulators.combine.disable_smoldyn_graphics_in_simulation_configuration(config)

        filename2 = os.path.join(self.dirname, 'config.txt')
        smoldyn.biosimulators.combine.write_smoldyn_simulation_configuration(config, filename2)

        sim = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        sim.setGraphics('none')
        output_file = smoldyn.biosimulators.combine.add_smoldyn_output_file(self.dirname, sim)
        self.assertIsInstance(output_file, smoldyn.biosimulators.data_model.SmoldynOutputFile)

        smoldyn.biosimulators.combine.add_commands_to_smoldyn_output_file(sim, output_file, [
            smoldyn.biosimulators.data_model.SmoldynCommand(command='molcount', type='E'),
        ])

    def test_add_smoldyn_output_files_for_sed_variables(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        smoldyn.biosimulators.combine.disable_smoldyn_graphics_in_simulation_configuration(config)

        filename2 = os.path.join(self.dirname, 'config.txt')
        smoldyn.biosimulators.combine.write_smoldyn_simulation_configuration(config, filename2)

        sim = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        sim.setGraphics('none')

        var = Variable(id='time', symbol=Symbol.time.value)
        variable_output_cmd_map = smoldyn.biosimulators.combine.validate_variables([var])
        smoldyn_output_files = smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(
            self.dirname, [var], variable_output_cmd_map, sim)
        self.assertEqual(set(smoldyn_output_files.keys()), set(['molcount']))

        var.symbol = 'undefined'
        with self.assertRaises(ValueError):
            smoldyn.biosimulators.combine.validate_variables([var])

        var.symbol = None
        var.target = 'undefined '
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.validate_variables([var])

    def test_get_variable_results(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        smoldyn.biosimulators.combine.disable_smoldyn_graphics_in_simulation_configuration(config)

        filename2 = os.path.join(self.dirname, 'config.txt')
        smoldyn.biosimulators.combine.write_smoldyn_simulation_configuration(config, filename2)

        sim = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        sim.setGraphics('none')

        vars = [
            Variable(id='time', symbol=Symbol.time.value),
            Variable(id='red', target='molcount red'),
            Variable(id='green', target='molcountspecies green'),
            Variable(id='greenBox', target='molcountinbox green 0 10'),
            Variable(id='greenHist', target='molcountspace green x 0 100 20')
        ]
        variable_output_cmd_map = smoldyn.biosimulators.combine.validate_variables(vars)
        smoldyn_output_files = smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(
            self.dirname, vars, variable_output_cmd_map, sim)

        sim.run(start=0., stop=0.2, dt=0.01, overwrite=True, display=False, quit_at_end=False)

        results = smoldyn.biosimulators.combine.get_variable_results(10, vars, variable_output_cmd_map, smoldyn_output_files)
        self.assertEqual(set(results.keys()), set(['time', 'red', 'green', 'greenBox', 'greenHist']))
        numpy.testing.assert_allclose(results['time'], numpy.linspace(0.1, 0.2, 11))

        self.assertEqual(results['red'].shape, (11, ))
        self.assertFalse(numpy.any(numpy.isnan(results['red'])))

        self.assertEqual(results['green'].shape, (11, ))
        self.assertFalse(numpy.any(numpy.isnan(results['green'])))

        self.assertEqual(results['greenBox'].shape, (11, ))
        self.assertFalse(numpy.any(numpy.isnan(results['greenBox'])))

        self.assertEqual(results['greenHist'].shape, (11, 20))
        self.assertFalse(numpy.any(numpy.isnan(results['greenHist'])))

        vars = [
            Variable(id='time', symbol='undefined'),
        ]
        with self.assertRaises(ValueError):
            variable_output_cmd_map = smoldyn.biosimulators.combine.validate_variables(vars)
            smoldyn.biosimulators.combine.get_variable_results(10, vars, variable_output_cmd_map, smoldyn_output_files)

        vars = [
            Variable(id='red', target='molcount blue'),
        ]
        variable_output_cmd_map = smoldyn.biosimulators.combine.validate_variables(vars)
        with self.assertRaises(ValueError):
            smoldyn.biosimulators.combine.get_variable_results(10, vars, variable_output_cmd_map, smoldyn_output_files)

        vars = [
            Variable(id='red', target='undefined blue'),
        ]
        with self.assertRaises(NotImplementedError):
            variable_output_cmd_map = smoldyn.biosimulators.combine.validate_variables(vars)
            smoldyn.biosimulators.combine.get_variable_results(10, vars, variable_output_cmd_map, smoldyn_output_files)

    def test_get_variable_results_2d(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S4_molecules', 'isotropic', 'diffi.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        smoldyn.biosimulators.combine.disable_smoldyn_graphics_in_simulation_configuration(config)

        filename2 = os.path.join(self.dirname, 'config.txt')
        smoldyn.biosimulators.combine.write_smoldyn_simulation_configuration(config, filename2)

        sim = smoldyn.biosimulators.combine.init_smoldyn_simulation_from_configuration_file(filename2)
        sim.setGraphics('none')

        vars = [
            Variable(id='time', symbol=Symbol.time.value),
            Variable(id='greenHist', target='molcountspace2d green(all) z -10 10 30 -10 10 40 -10 10')
        ]
        variable_output_cmd_map = smoldyn.biosimulators.combine.validate_variables(vars)
        smoldyn_output_files = smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(
            self.dirname, vars, variable_output_cmd_map, sim)

        sim.run(start=0., stop=0.2, dt=0.01, overwrite=True, display=False, quit_at_end=False)

        results = smoldyn.biosimulators.combine.get_variable_results(10, vars, variable_output_cmd_map, smoldyn_output_files)
        self.assertEqual(set(results.keys()), set(['time', 'greenHist']))
        numpy.testing.assert_allclose(results['time'], numpy.linspace(0.1, 0.2, 11))

        self.assertEqual(results['greenHist'].shape, (11, 30, 40))
        self.assertFalse(numpy.any(numpy.isnan(results['greenHist'])))

    def test_exec_sed_task(self):
        task = Task(
            id='task',
            model=Model(
                id='model',
                source=os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt'),
                language=ModelLanguage.Smoldyn.value,
            ),
            simulation=UniformTimeCourseSimulation(
                initial_time=0.,
                output_start_time=0.1,
                output_end_time=0.2,
                number_of_points=10,
                algorithm=Algorithm(
                    kisao_id='KISAO_0000057',
                    changes=[
                        AlgorithmParameterChange(kisao_id='KISAO_0000488', new_value='10'),
                    ]
                )
            ),
        )

        variables = [
            Variable(id='time', symbol=Symbol.time.value, task=task),
            Variable(id='red', target='molcount red', task=task),
            Variable(id='green', target='molcount green', task=task),
        ]

        results, log = smoldyn.biosimulators.combine.exec_sed_task(task, variables)

        self.assertEqual(set(results.keys()), set(['time', 'red', 'green']))
        numpy.testing.assert_allclose(results['time'], numpy.linspace(0.1, 0.2, 11))
        for result in results.values():
            self.assertEqual(result.shape, (11, ))
            self.assertFalse(numpy.any(numpy.isnan(result)))

        self.assertEqual(log.algorithm, 'KISAO_0000057')
        self.assertEqual(log.simulator_details, {
            'class': 'smoldyn.Simulation',
            'instanceAttributes': {'setRandomSeed': 10},
            'method': 'run',
            'methodArguments': {
                'start': 0.,
                'stop': 0.2,
                'dt': 0.01,
            },
        })

        task.simulation.algorithm.changes.append(AlgorithmParameterChange('KISAO_0000254', '5'))
        results, log = smoldyn.biosimulators.combine.exec_sed_task(task, variables)
        self.assertEqual(log.simulator_details, {
            'class': 'smoldyn.Simulation',
            'instanceAttributes': {'setRandomSeed': 10},
            'method': 'run',
            'methodArguments': {
                'start': 0.,
                'stop': 0.2,
                'dt': 0.01,
                'accuracy': 5.,
            },
        })

        task.simulation.algorithm.kisao_id = 'KISAO_0000437'
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.exec_sed_task(task, variables)

    def test_exec_sed_task_positive_initial_time(self):
        task = Task(
            id='task',
            model=Model(
                id='model',
                source=os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt'),
                language=ModelLanguage.Smoldyn.value,
            ),
            simulation=UniformTimeCourseSimulation(
                initial_time=0.01,
                output_start_time=0.01,
                output_end_time=0.11,
                number_of_points=10,
                algorithm=Algorithm(
                    kisao_id='KISAO_0000057',
                )
            ),
        )

        variables = [
            Variable(id='time', symbol=Symbol.time.value, task=task),
            Variable(id='red', target='molcount red', task=task),
            Variable(id='green', target='molcount green', task=task),
        ]

        results, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables)

        self.assertEqual(set(results.keys()), set(['time', 'red', 'green']))
        numpy.testing.assert_allclose(results['time'], numpy.linspace(0.01, 0.11, 11))

    def test_exec_sed_task_negative_initial_time(self):
        task = Task(
            id='task',
            model=Model(
                id='model',
                source=os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt'),
                language=ModelLanguage.Smoldyn.value,
            ),
            simulation=UniformTimeCourseSimulation(
                initial_time=-0.01,
                output_start_time=-0.01,
                output_end_time=0.09,
                number_of_points=10,
                algorithm=Algorithm(
                    kisao_id='KISAO_0000057',
                )
            ),
        )

        variables = [
            Variable(id='time', symbol=Symbol.time.value, task=task),
            Variable(id='red', target='molcount red', task=task),
            Variable(id='green', target='molcount green', task=task),
        ]

        results, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables)

        self.assertEqual(set(results.keys()), set(['time', 'red', 'green']))
        numpy.testing.assert_allclose(results['time'], numpy.linspace(-0.01, 0.09, 11), rtol=5e-6)

    @flaky.flaky(max_runs=10, min_passes=1)
    def test_exec_sed_task_with_changes(self):
        task = Task(
            id='task',
            model=Model(
                id='model',
                source=os.path.join(os.path.dirname(__file__), 'fixtures', 'lotvolt.txt'),
                language=ModelLanguage.Smoldyn.value,
            ),
            simulation=UniformTimeCourseSimulation(
                initial_time=0.,
                output_start_time=0.,
                output_end_time=0.1,
                number_of_points=10,
                algorithm=Algorithm(
                    kisao_id='KISAO_0000057',
                    changes=[
                        AlgorithmParameterChange(kisao_id='KISAO_0000488', new_value='10'),
                    ]
                )
            ),
        )
        model = task.model
        sim = task.simulation

        variable_ids = ['rabbit', 'fox']

        variables = []
        for variable_id in variable_ids:
            variables.append(Variable(id=variable_id, target='molcount ' + variable_id, task=task))

        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        results, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        with self.assertRaises(AssertionError):
            for variable_id in variable_ids:
                numpy.testing.assert_allclose(
                    results[variable_id][0:int(sim.number_of_points / 2 + 1)],
                    results[variable_id][-int(sim.number_of_points / 2 + 1):])

        # check simulation is repeatable
        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results2[variable_id],
                results[variable_id])

        # check simulation is repeatable in two steps
        sim.output_end_time = sim.output_end_time / 2
        sim.number_of_points = int(sim.number_of_points / 2)

        model.changes = []
        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='fixmolcount ' + variable_id, new_value=None))
        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        model.changes = []
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results2[variable_id],
                results[variable_id][0:sim.number_of_points + 1])

        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='fixmolcount ' + variable_id, new_value=results2[variable_id][-1]))
        results3, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results3[variable_id],
                results[variable_id][-(sim.number_of_points + 1):])

        # check model change modifies simulation
        model.changes = []
        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='fixmolcount ' + variable_id, new_value=None))
        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        model.changes = []
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results2[variable_id],
                results[variable_id][0:sim.number_of_points + 1])

        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='fixmolcount ' + variable_id, new_value=results2[variable_id][-1] + 1))
        results3, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        with self.assertRaises(AssertionError):
            for variable_id in variable_ids:
                numpy.testing.assert_allclose(
                    results3[variable_id],
                    results[variable_id][-(sim.number_of_points + 1):])

        # check model change modifies simulation
        model.changes = []
        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='killmol ' + variable_id, new_value=None))
        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        model.changes = []
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results2[variable_id],
                results[variable_id][0:sim.number_of_points + 1])

        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='killmol ' + variable_id, new_value=0))
        results3, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        with self.assertRaises(AssertionError):
            for variable_id in variable_ids:
                numpy.testing.assert_allclose(
                    results3[variable_id],
                    results[variable_id][-(sim.number_of_points + 1):])

        # preprocessing-time change
        model.changes = []
        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='define K_1', new_value=10))
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results2[variable_id],
                results[variable_id][0:sim.number_of_points + 1])

        model.changes = []
        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='define K_1', new_value=10))
        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        with self.assertRaisesRegex(NotImplementedError, 'can only be changed during simulation preprocessing'):
            smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        model.changes = []
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        for variable_id in variable_ids:
            numpy.testing.assert_allclose(
                results2[variable_id],
                results[variable_id][0:sim.number_of_points + 1])

        model.changes = []
        for variable_id in variable_ids:
            model.changes.append(ModelAttributeChange(target='define K_1', new_value=0))
        preprocessed_task = smoldyn.biosimulators.combine.preprocess_sed_task(task, variables)
        model.changes = []
        results2, _ = smoldyn.biosimulators.combine.exec_sed_task(task, variables, preprocessed_task=preprocessed_task)
        with self.assertRaises(AssertionError):
            for variable_id in variable_ids:
                numpy.testing.assert_allclose(
                    results2[variable_id],
                    results[variable_id][0:sim.number_of_points + 1])

    def test_exec_sedml_docs_in_combine_archive(self):
        doc, archive_filename = self._build_combine_archive()

        out_dir = os.path.join(self.dirname, 'out')

        config = get_config()
        config.REPORT_FORMATS = [ReportFormat.h5]

        _, log = smoldyn.biosimulators.combine.exec_sedml_docs_in_combine_archive(
            archive_filename, out_dir, config=config)
        if log.exception:
            raise log.exception

        self._assert_combine_archive_outputs(doc, out_dir)

    def test_command_line_interface(self):
        doc, archive_filename = self._build_combine_archive()

        out_dir = os.path.join(self.dirname, 'out')
        with mock.patch.dict(os.environ, self._get_combine_archive_exec_env()):
            with smoldyn.biosimulators.__main__.App(argv=['-i', archive_filename, '-o', out_dir]) as app:
                app.run()

        self._assert_combine_archive_outputs(doc, out_dir)

    def test_raw_command_line_interface(self):
        with mock.patch('sys.argv', ['', '--help']):
            with self.assertRaises(SystemExit) as context:
                smoldyn.biosimulators.__main__.main()
                self.assertRegex(context.Exception, 'usage: ')

    def _build_combine_archive(self):
        task = Task(
            id='task',
            model=Model(
                id='model',
                source='bounce1.txt',
                language=ModelLanguage.Smoldyn.value,
            ),
            simulation=UniformTimeCourseSimulation(
                id='sim',
                initial_time=0.,
                output_start_time=0.1,
                output_end_time=0.2,
                number_of_points=10,
                algorithm=Algorithm(
                    kisao_id='KISAO_0000057',
                    changes=[
                        AlgorithmParameterChange(kisao_id='KISAO_0000488', new_value='10'),
                    ]
                )
            ),
        )

        variables = [
            Variable(id='time', symbol=Symbol.time.value, task=task),
            Variable(id='red', target='molcount red', task=task),
            Variable(id='green', target='molcount green', task=task),
        ]

        doc = SedDocument(
            models=[task.model],
            simulations=[task.simulation],
            tasks=[task],
            data_generators=[
                DataGenerator(
                    id='data_gen_time',
                    variables=[Variable(id='var_time', symbol=Symbol.time.value, task=task)],
                    math='var_time',
                ),
                DataGenerator(
                    id='data_gen_red',
                    variables=[Variable(id='var_red', target='molcount red', task=task)],
                    math='var_red',
                ),
                DataGenerator(
                    id='data_gen_green',
                    variables=[Variable(id='var_green', target='molcount green', task=task)],
                    math='var_green',
                ),
            ],
        )
        doc.outputs.append(Report(
            id='report',
            data_sets=[
                DataSet(id='data_set_time', label='time', data_generator=doc.data_generators[0]),
                DataSet(id='data_set_red', label='red', data_generator=doc.data_generators[1]),
                DataSet(id='data_set_green', label='green', data_generator=doc.data_generators[2]),
            ]
        ))

        archive_dirname = os.path.join(self.dirname, 'archive')
        os.makedirs(archive_dirname)
        shutil.copyfile(os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt'), os.path.join(archive_dirname, 'bounce1.txt'))
        sim_filename = os.path.join(archive_dirname, 'sim_1.sedml')
        SedmlSimulationWriter().run(doc, sim_filename)

        archive = CombineArchive(
            contents=[
                CombineArchiveContent(
                    'bounce1.txt', CombineArchiveContentFormat.Smoldyn.value),
                CombineArchiveContent(
                    'sim_1.sedml', CombineArchiveContentFormat.SED_ML.value),
            ],
        )
        archive_filename = os.path.join(self.dirname, 'archive.omex')
        CombineArchiveWriter().run(archive, archive_dirname, archive_filename)

        return doc, archive_filename

    def _get_combine_archive_exec_env(self):
        return {
            'REPORT_FORMATS': 'h5'
        }

    def _assert_combine_archive_outputs(self, doc, out_dir):
        results = ReportReader().run(doc.outputs[0], out_dir, os.path.join('sim_1.sedml', doc.outputs[0].id), format=ReportFormat.h5)

        self.assertEqual(set(results.keys()), set(['data_set_time', 'data_set_red', 'data_set_green']))
        numpy.testing.assert_allclose(results['data_set_time'], numpy.linspace(0.1, 0.2, 11))
        for result in results.values():
            self.assertEqual(result.shape, (11, ))
            self.assertFalse(numpy.any(numpy.isnan(result)))


if __name__ == "__main__":
    unittest.main()
