## Changelog

### 2.65 (unreleased)

https://github.com/ssandrews/Smoldyn/compare/v2.64...v2.65

- `Box`, `addBox` and `moleculePerBox` are deprecated. Use `setPartition`
  instead (e4465b1e5).
- (developer) Major refactoring in `Simulation` class (#50, #51, #52). `smoldyn.Command` is
  moved to C++ (now available as `_smoldyn.Command`). This does not change the
  API.

### 2.64 (3/24/21)

https://github.com/ssandrews/Smoldyn/compare/v2.63...v2.64

- Added support for creating concurrent simulation in one Python script (see
  `smoldyn.Simulation` class).
- Fixed minor bugs with reactions on surfaces.
- Converted user's manual to LaTeX
- Merged Libsmoldyn manual with regular manual.
- Update to readthedocs.

### 2.63 (12/16/20)

https://github.com/ssandrews/Smoldyn/compare/v2.62...v2.63

- Improved Python bindings.
- Added Python example files.
- Fixed bug with moving surfaces.

### 2.62 (10/14/20)

- Added Python bindings.
- Improved Windows build now with MSVC.
- Subtantial build system refactoring for other platforms as well.

## Old releases (pre Github)

2.61 (7/22/19)
- Minor bug fixes: improved radialdistfunction and surface detection near system boundaries.

2.60 (6/7/19)
- Smoldyn really works on Windows. Also, some minor bug fixes.

2.59 (5/15/19)
- Major progress in getting Smoldyn to work on Windows again.

2.58 (3/29/19)
- Improved math precision, improved single-molecule tracking, fixed bug in
  probabilistic commands (e.g. killmolprob), enabled command probability to
  depend on molecule location.

2.57 (3/21/19)
- Math supports scientific notation, allowed species change with surface
  actions, improved single-molecule tracking. Smoldyn is now on github.

2.56 (9/18/18)
- Got MinGW compiling to work again for Windows. Fixed minor bugs. Largely
  completed lambda-rho reaction algorithm support, although some minor
  calculation errors remain.

2.55 (7/16/18)
- Several minor bug fixes. This version was not compiled for Windows due to
  compiler problems.

2.54 (8/27/17)
- Fixed minor bug that affected drift of surface-bound molecules.

2.53 (5/25/17)
- Fixed minor bugs that arose when counting molecules during hybrid simulations
  and fixed errors that arose with certain compilers.

2.52 (5/16/17)
- Fixed very minor bug that arose when counting molecules during hybrid
  simulations.

2.51 (3/15/17)
- Fixed trivial bug that caused compiling errors and bug that affected
  surface-interactions of surface-bound molecules.

2.50 (2/28/17)
- Improved support for moving surfaces and developed infrastructure for
  state-observation functions.

2.49 (2/17/17)
- Added partial support for moving surfaces and for the use of basic math
  functions in input files.

2.48 (11/16/16)
- Fixed a few minor bugs which caused crashes in unusual situations.

2.47 (8/30/16)
- Improved rule-based modeling with wildcards with fixed bugs, rules for
  molecular properties, and properties for reaction rules.

2.46 (7/30/16)
- Rule-based modeling with wildcards is now true on-the-fly, expanding only as
  needed.

2.45 (7/15/16)
- Fixed several minor bugs, including in equilmol command, math parsing, and
  hybrid simulations.

2.44 (6/27/16)
- Simplified input of reverse reactions and returned freeglut.dll to the
  windows download.

2.43 (5/18/16)
- Improved Libsmoldyn compiling and linking, renamed documentation, and added
  some commands.

2.42 (4/29/16)
- Fixed minor bugs. Excluded volume reactions now work well and added
  overlapping space hybrid simulation.

2.41 (4/8/16)
- Fixed several bugs that arose in v. 2.40, added tutorial section to User's
  Manual, and improved algorithm for excluded volume reactions.

2.40 (3/22/16)
- Lots of new features: rule-based modeling with BioNetGen, rule-based modeling
  with wildcards, variables and arithmetic in input files, fixed several more
  bugs in surface diffusion, several new commands.

2.39 (1/15/16)
- Fixed another important bug in surface diffusion. Quantitatively validated
  results. Accelerated virtual box assignment, which especially helps when
  molecules are unevenly distributed.

2.38 (10/22/15)
- Lots of minor bug fixes and feature additions. New features: fixmolcountrange
  commands, quit_at_end statement, reaction_intersurface statement, more
  versatile bounce and confspread reactions, works on older Mac systems.

2.37 (10/7/15)
- Fixed a major bug that affected diffusion of surface-bound molecules. In the
  process, sped up surface-bound diffusion. Also added support for single
  molecule tracking with serial number control and reaction logging.

2.36 (6/9/15)
- Fixed a minor bug that affected some bounce type reactions.

2.35 (4/23/15) - Fixed a bug that led to incorrect reaction rates for some
first order reactions. The affected ones are those in which a single reactant
can react via multiple first order reactions and reaction probabilities are
high.

2.34 (1/8/15)
- Made the lattice code part of the pre-compiled distribution and nearly
  finished BNG parsing.

2.33 (10/9/14)
- Fixed several small bugs and added a few example files (e.g. MinD
  oscillation).

2.32 (8/29/14)
- Added adjacent-volume hybrid simulation with NSV module, improved wildcard
  support, enabled molecule clustering simulations, added VTK visualization,
  and removed Libmoleculizer and threading support.

2.31 (9/9/13)
- Fixed a minor bug in meansqrdisp3 command and added setreactionratemolcount
  command.

2.30 (8/21/13)
- Fixed some very minor bugs and added several runtime commands.

2.29 (4/10/13)
- Small improvements to Libmoleculizer and a few minor bug fixes.

2.28 (8/29/12)
- Improved CMake build system and fixed a few minor bugs. Added rudimentary filament code.

2.27 (7/26/12)
- Added molecular drift relative to surface coordinates and settable precision
  for numerical output. Improved simulation of molecules with excluded volume
  so rebound is not to fixed distance but equals prior overlap. Also, changed
  code from C to C++, changed build system from AutoTools to CMake, improved
  internal error handling, and coordinated development with VCell team.

2.26 (3/2/12)
- Finished rule-based modeling with Libmoleculizer, added partial wildcard
  support, and enabled excluded volume reactions with periodic boundaries.

2.25 (9/26/11)
- Fixed minor bugs and improved Ubuntu installation.

2.24 (7/27/11)
- Major work on Libsmoldyn to the point that it's fairly complete and it seems
  to work. Also, major work on the build system, which enabled the distribution
  of pre-compiled code. Finally, of course, fixed some minor bugs and added a
  few commands.

2.23 (6/24/11)
- Added preliminary Libsmoldyn API, made all memory allocation dynamic, added
  molecule list and reaction list shuffling, added display of time and molecule
  counts to graphics window, added several commands, improved mid-simulation
  modifications, and fixed several minor bugs.

2.22 (3/22/11)
- Fixed a bug that prevented compiling when OpenGL was disabled. Also added a
  skeleton for libsmoldyn.

2.21 (3/11/11)
- Fixed minor bugs that affected surface-bound molecule diffusion and surface
  detection.

2.20 (3/4/11)
- Fixed major bug with non-threading installations (including the Windows
  version), improved define statements, and modified configure defaults.

2.19 (2/11/11)
- Surface-bound molecules can interact with other surfaces, added --define
  command line option, added append_files statement, added ifincmpt and
  killmolincmpt commands, and cleaned up surface code.
2.18 (1/6/11)
- Fixed bug that affected desorbed molecules.

2.17 (11/19/10)
- Improved Ubuntu installation and fixed a bug for partially transmitting
  surfaces with large transmission coefficients.

2.16 (9/24/10)
- Fixed bug that affected desorbed molecules.

2.15 (7/20/10)
- Fixed bugs that were introduced with version 2.14.

2.14 (7/18/10)
- Fixed integer command queue for Windows and improved continuation reactions.

2.13 (7/15/10)
- Better reaction detail reporting and improved libmoleculizer operation.

2.12 (6/10/10)
- Added commands: molcountincmpts and replacecmptmol.

2.11 (5/4/10)
- Slight improvement to build system.

2.10 (3/24/10)
- Major code improvement on libmoleculizer module.

2.09 (1/6/10)
- Substantially improved graphics and added movesurfacemol and molcountincmpt2 commands.

2.08 (11/20/09)
- Released unofficially. This fixed a few minor bugs.

2.07 (11/17/09)
- Fixed bugs. In particular, fixed diffusion of surface-bound molecules.

2.06 (11/6/09)
- Molecules can change species at surfaces and surface-bound molecules jump at
  "jump" surfaces. Also, bug fixes and general code cleanup.

2.05 (7/23/09)
- Nathan got the build system to work well for most Mac and Linux systems. Some
  documentation updates.

2.04 (6/27/09)
- Debugged and improved build system and support for multimeric complexes,
  although they still lack complete documentation. Also added support for
  effectve unbounded diffusion.

2.03 (5/22/09)
- Changed from a make file build to the GNU build system (autoconf, automake,
  etc.), added support for multimeric complexes and for multiple program
  threads. These new features are unstable.

2.02 (5/5/09)
- Added a few commands, cleaned up some code, and fixed a few bugs.

2.01 (3/3/09)
- Improved diffusion of surface-bound molecules and simplified block-style
  input formats.

2.0 (2/17/09)
- Fixed a few trivial bugs, and increased the version number to 2.0.

#### 1.89 (2/11/09)
- Adsorption, desorption, and partial transmission algorithms now work well and
  have been verified to be accurate. Also added molecules with excluded volume.
  Lots of new code structure improvements will enable several planned features
  (including macromolecular complexes and a command line user input).

#### 1.88 (1/16/09)
- First release of SmolCrowd utility program, adsorption, desorption and
  partial transmission should be correct but have not been verified, improved
  integer-time commands, and simplified the configuration file format slightly.

#### 1.87 (12/7/08)
- Vastly improved the wrl2smol utility program.

#### 1.86 (11/17/08)
- Fixed several minor bugs, improved makefiles for more reliable installation,
  and made Smoldyn compatible with 64-bit machines.

#### 1.85 (6/3/08)
- The Mersenne Twister is now used for random numbers, added define statements
  to parser, renamed allosteric reactions to conformational spread reactions,
  and fixed lots of minor bugs and one significant one (reaction products ended
  up across surfaces).

#### 1.84 (4/11/08)
- First Windows release. Also, added reactions that are only active in
  specified compartments, added several commands, and fixed the change time
  step command.

#### 1.83 (3/14/08)
- Improved unimolecular reaction rates for multiple reaction channels, improved
  allosteric reactions, and added molecular drift.

#### 1.82 (2/28/08)
- Fixed a bug in which reaction products ended up across
surfaces. Also, reactions were completely overhauled and can now be entered
with a vastly improved format. Allosteric reactions were added.

#### 1.81 (1/23/08)
- Surface-bound molecules now work, lots of minor bugs were fixed, multiple
  molecule lists were implemented, porting works well, and documentation was
  improved.

v1.80 (12/22/07)
- 'Q' now quits the program, time steps can be changed mid-simulation, and
  basic porting between Smoldyn and MOOSE was added.

v1.79 (12/6/07)
- Fixed more bugs with commands. Also added basic compartments.

#### 1.78 (11/30/07)
- Fixed lots of bugs with surface-bound molecules and commands, and improved
  input formats of several statements.

#### 1.77 (11/18/07)
- Fixed a few minor bugs and modified several functions to make Smoldyn easier
  to parallelize.

#### 1.76 (11/7/07)
- Main source file was split to two files and assorted function rearranging.
  More command-line options are now available.

#### 1.75 (11/6/07)
- Added support of surface-bound molecules, which still need some debugging.
  Also, changed several surface input statement formats. The documentation is
  current, but incomplete.

#### 1.74 (10/22/07)
- Fixed yet more bugs in surfaces, added disk-shaped surface panels, and made
  some changes in surface input statement formats. Only the code got updated
  for this release, so the documentation is slightly out of date.

#### 1.73 (9/25/07)
- Minor surface improvements, added exponential time-stepping option for
  run-time commands, and improved multi-platform compiling.

#### 1.72 (2/26/07)
- Surfaces are finally totally functional for both diffusion and reactions (or
  so I thought for a while).

#### 1.71 (12/8/06)
- Better surface support with cylinder and hemisphere panels, and improved
  reflective surfaces.

#### 1.70 (5/17/06)
- Reasonably good surface support, where surfaces can be reflective, absorbing
  or transparent, and they are comprised of panels that can be rectangular,
  triangular, or spherical.

#### 1.59 (?)
- Displays the random number seed.

#### 1.58 (7/22/05)
- A new runtime command, a randomized Brownian dynamics lookup table to remove
  artifacts that can arise from random number generator problems, and a more
  versitile mol statement.

#### 1.57 (2/17/05)
- A new runtime command.

#### 1.56 (1/14/05)
- Improved graphics and a few other minor changes.

#### 1.55 (8/20/04)
- Minor graphics changes, some improvements in the design of the command queue
  for the runtime command interpreter, and it is now possible for commands to
  return error messages.

#### 1.53 (2/9/04)
- Cleaned up code for the runtime command interpreter, it is now possible to
  save file stacks, minor display and user interface improvements, and several
  new commands for text output and excluded volumes.

#### 1.52 (10/24/03)
- A significant bug was fixed in the equation for absorbing walls (thanks to
  Dan Gillespie for catching it), it is now possible to pause simulations while
  using graphics, and a few minor bugs were fixed.

#### 1.51 (9/5/03)
- Unimolecular reactions can now have much lower reaction rates, a memory error
  upon termination was fixed, parameter checking was improved, several minor
  bugs were fixed, and two new interpreter commands were added.

#### 1.5 (7/2003)
- First public release.
