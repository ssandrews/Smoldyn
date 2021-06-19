__author__ = "Jonathan Karr"
__email__ = "karr@mssm.edu"

from smoldyn.biosimulators.data_model import Simulation, SimulationInstruction
from smoldyn.biosimulators.utils import read_simulation, _read_simulation_line
try:
    import biosimulators_utils
    from biosimulators_utils.combine.data_model import CombineArchive, CombineArchiveContent, CombineArchiveContentFormat
    from biosimulators_utils.combine.io import CombineArchiveWriter
    from biosimulators_utils.report.data_model import ReportFormat
    from biosimulators_utils.report.io import ReportReader
    from biosimulators_utils.sedml.data_model import (
        SedDocument,
        Model, ModelLanguage, ModelAttributeChange,
        UniformTimeCourseSimulation, Algorithm, AlgorithmParameterChange,
        Task, DataGenerator, Report, DataSet, Variable, Symbol)
    from biosimulators_utils.sedml.io import SedmlSimulationWriter
except ModuleNotFoundError:
    biosimulators_utils = None
from unittest import mock
import datetime
import dateutil.tz
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
        output_file = smoldyn.biosimulators.combine.SmoldynOutputFile('out.txt', '/tmp/out.txt')
        self.assertEqual(output_file.name, 'out.txt')
        self.assertEqual(output_file.filename, '/tmp/out.txt')

    def test_SmoldynCommand(self):
        output_file = smoldyn.biosimulators.combine.SmoldynCommand('molcount', 'E')
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

    def test_apply_sed_model_change_to_smoldyn_simulation_configuration(self):
        filename = os.path.join(self.EXAMPLES_DIRNAME, 'S1_intro', 'bounce1.txt')
        config = smoldyn.biosimulators.combine.read_smoldyn_simulation_configuration(filename)
        self.assertEqual(config[13], 'difc red 3')

        change = ModelAttributeChange(target=' difc  red ', new_value=' 5 ')

        smoldyn.biosimulators.combine.apply_sed_model_change_to_smoldyn_simulation_configuration(change, config)
        self.assertEqual(config[13], 'difc red 5')

        config[13] = 'difc red 3 # comment'
        smoldyn.biosimulators.combine.apply_sed_model_change_to_smoldyn_simulation_configuration(change, config)
        self.assertEqual(config[13], 'difc red 5 # comment')

        config[13] = 'difc  red  3  #  comment  '
        smoldyn.biosimulators.combine.apply_sed_model_change_to_smoldyn_simulation_configuration(change, config)
        self.assertEqual(config[13], 'difc red 5 #  comment')

        config[13] = 'surface_mol 100 E(front) membrane all all'
        change = ModelAttributeChange(target=' surface_mol  E(front)  membrane  all  all  ', new_value='5')
        smoldyn.biosimulators.combine.apply_sed_model_change_to_smoldyn_simulation_configuration(change, config)
        self.assertEqual(config[13], 'surface_mol 5 E(front) membrane all all')

        change = ModelAttributeChange(target='not supported', new_value='5')
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.apply_sed_model_change_to_smoldyn_simulation_configuration(change, config)

        change = ModelAttributeChange(target='difc notdefined', new_value='5')
        with self.assertRaises(ValueError):
            smoldyn.biosimulators.combine.apply_sed_model_change_to_smoldyn_simulation_configuration(change, config)

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
        output_file = smoldyn.biosimulators.combine.add_smoldyn_output_file(filename2, sim)
        self.assertIsInstance(output_file, smoldyn.biosimulators.combine.SmoldynOutputFile)

        smoldyn.biosimulators.combine.add_commands_to_smoldyn_output_file(sim, output_file, [
            smoldyn.biosimulators.combine.SmoldynCommand(command='molcount', type='E'),
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
        smoldyn_output_files = smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(filename2, [var], sim)
        self.assertEqual(set(smoldyn_output_files.keys()), set(['molcount']))

        var.symbol = 'undefined'
        with self.assertRaises(ValueError):
            smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(filename2, [var], sim)

        var.symbol = None
        var.target = 'undefined '
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(filename2, [var], sim)

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
        smoldyn_output_files = smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(filename2, vars, sim)

        sim.run(start=0., stop=0.2, dt=0.01, overwrite=True, display=False, quit_at_end=False)

        results = smoldyn.biosimulators.combine.get_variable_results(10, vars, smoldyn_output_files)
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
            smoldyn.biosimulators.combine.get_variable_results(10, vars, smoldyn_output_files)

        vars = [
            Variable(id='red', target='molcount blue'),
        ]
        with self.assertRaises(ValueError):
            smoldyn.biosimulators.combine.get_variable_results(10, vars, smoldyn_output_files)

        vars = [
            Variable(id='red', target='undefined blue'),
        ]
        with self.assertRaises(NotImplementedError):
            smoldyn.biosimulators.combine.get_variable_results(10, vars, smoldyn_output_files)

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
        smoldyn_output_files = smoldyn.biosimulators.combine.add_smoldyn_output_files_for_sed_variables(filename2, vars, sim)

        sim.run(start=0., stop=0.2, dt=0.01, overwrite=True, display=False, quit_at_end=False)

        results = smoldyn.biosimulators.combine.get_variable_results(10, vars, smoldyn_output_files)
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
                changes=[
                    ModelAttributeChange(target='difc red', new_value='4'),
                ],
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
            'setInstanceAttributes': {'setRandomSeed': 10},
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
            'setInstanceAttributes': {'setRandomSeed': 10},
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

    def test_exec_sedml_docs_in_combine_archive(self):
        doc, archive_filename = self._build_combine_archive()

        out_dir = os.path.join(self.dirname, 'out')
        log = smoldyn.biosimulators.combine.exec_sedml_docs_in_combine_archive(
            archive_filename, out_dir, report_formats=[ReportFormat.h5])

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
                changes=[
                    ModelAttributeChange(target='difc red', new_value='4'),
                ],
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

        updated = datetime.datetime(2020, 1, 2, 1, 2, 3, tzinfo=dateutil.tz.tzutc())
        archive = CombineArchive(
            contents=[
                CombineArchiveContent(
                    'bounce1.txt', CombineArchiveContentFormat.Smoldyn.value, updated=updated),
                CombineArchiveContent(
                    'sim_1.sedml', CombineArchiveContentFormat.SED_ML.value, updated=updated),
            ],
            updated=updated,
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
