##  Statements

### Statements about the configuration file

```
# text
```

Single-line comment. A `#` symbol indicates that the rest of the line is a comment.

```
/* text */
```

Multi-line comment. All lines between `/*` and the following `*/` are ignored.
These must be the first `words` on a line. Additional text on these lines is
ignored as well. In future versions, the syntax of these may be changed so as
to be identical to C-style block comments.


```
read_file filename
```

Read some other configuration file, returning to the present one when
that one has been read.

```
end_file
```

End of configuration file. This line is optional (but good programming
practice), as Smoldyn can also just read until the file ends.

```
quit_at_end value
```

Use a value of `yes` to tell the simulator to quit the program at the
end of the simulation, during simulations that use graphics. This has no
effect if simulations do not use graphics. Use `no` to turn this off,
which is the default behavior.

```
define key substitution
```

Definition of macro replacement text. Throughout the remainder of this
configuration file, but not files that are called by it, all incidents
of the string *key* are replaced with the string *substitution* before
further parsing is performed. It is permissible to not include any
substitution text.

```
define_global key substitution
```

Definition of macro replacement text, which is identical to define,
except that this definition applies throughout both this file and all
files that are called by it. Global definitions can also be entered on
the command line using the --define option.

```
undefine key
```

Removes a macro substitution definition that was made previously,
whether global or local. Global undefines apply to this file and all
files that are called by it, but not to a file that called this one.
Entering *key* as `all` undefines all definitions.

```
ifdefine key
```

The following lines of the configuration file are read only if *key* is
a term that was defined with define or define_global (or was defined
automatically, which includes FILEROOT). Reading, or not reading,
continues to any else statement. The end of the condition is given with
the endif statement.

```
ifundefine key
```

This is identical to ifdefine, except that reading continues only if
*key* has not been defined.

```
else
```

This is the else condition which is supposed to follow an `ifdefine` or
`ifundefine` statement.

```
endif
```

This ends a condition that is started by an `ifdefine` or `ifundefine`
statement.

```
display_define
```

Causes all current definitions to be displayed to the standard output.
This is only useful for debugging define issues in configuration files.

### Statements about variables

```
variable *var* = *value*
```

Sets the value of variable *var* to *value*, which needs to evaluate to
a numerical value. Note that spaces and the equals sign are required.

**Statements about space and time**

```
dim *dim*
```

Dimensionality of the system. Must be at least one, and is typically
between 1 and 3. Larger numbers are permitted as well.

```
boundaries *dim pos1 pos2*
```

```
boundaries *dim pos1 pos2 type*
```

Creates lower and upper boundaries to define the simulation volume on
dimension *dim*. The *dim* value should be ‘x’, ‘y’, or ‘z’ (however, 0,
1, and 2 work as well). These boundaries are located at *pos1* and
*pos2*. Using the first format, which is advised for systems that
include surfaces, boundaries are created that are transparent to
molecules, meaning that they do not contain or otherwise interact with
molecules. Surfaces need to be defined to keep molecules in the system.
The second format is preferable for systems that do not include any
surfaces. In this case, the boundary type can be ‘r’ for reflective, ‘t’
for transparent, ‘a’ for absorbing, or ‘p’ for periodic. For most
purposes, this statement replaces the low_wall and high_wall
statements.

```
low_wall *dim pos type*
```

This statement has been largely superseded by boundaries. This creates a
lower boundary for the simulation volume. This wall is perpendicular to
the dimension *dim* (‘x’, ‘y’, or ‘z’) such that all locations between
*pos* and the position of the corresponding upper boundary are
considered to be within the simulation volume. The type of wall is given
in *type*, which should be one of four single letter codes: ‘r’ means a
reflecting wall, ‘p’ means a periodic wall (also called wrap-around or
toroidal), ‘a’ means an absorbing wall, and ‘t’ means a transparent
wall. Transparent walls imply an unbounded system and may lead to slow
simulations. *If any surfaces are defined for the simulation, then walls
still must be entered to define the system volume, but these walls are
essentially non-functional (the sole exception is that reactions can
occur across periodic walls). Additional surfaces need to be defined to
serve as the system boundaries.*

```
high_wall *dim pos type*
```

This statement has been largely superseded by boundaries. This is
identical to the definition for low_wall, although this creates the
upper boundary for the simulation volume. See note about surfaces in
low_wall*.*

```
time_start *time*
```

Starting point for simulated time.

```
time_stop *time*
```

Stopping time of simulation, using simulated time. The simulation
continues past the time_stop value by less than one time step.

```
time_step *time*
```

Time step for the simulation. Longer values lead to a faster runtime,
while shorter values lead to higher accuracy. Also, longer values lead
to bimolecular reactions that behave more as though they are activation
limited, rather then diffusion limited.

```
time_now *time*
```

Another starting time of simulation. Default value is equal to
time_start. If this time is before time_start, the simulation starts
at time_start; otherwise, it starts at time_now.

### Statements about molecules

```
species *name*<sub>1</sub> *name*<sub>2</sub> *… name*<sub>n</sub>
```

Names of one of more molecular species present in the system. Standard
naming conventions are followed, in that the name should start with a
letter and spaces are not permitted.

```
species_group *group species*<sub>1</sub> *species*<sub>2</sub> *...*
```

Defines a group of species called *group* and adds
*species*<sub>1</sub>, *species*<sub>2</sub>, and potentially other
species to this group. Empty groups are allowed. Any number of species
can be added. If the group already exists, the named species will be
added to the existing group. These groups can be used in most statements
and commands that have species inputs, where they enable operations on
multiple species at once. However, they cannot be used in reactions.

```
difc *species* *value*
difc *species*(*state*) *value*
difc_rule *species*(*state*) *value*
```

Isotropic diffusion coefficient of molecule type *species*. Default
value is 0. The state, which is optional, refers to the surface-bound
state of the molecule: solution, front, back, up, or down; if omitted,
only the solution state is set with this statement. *name* may be `all`
and/or *state* may be `all` to set diffusion coefficients for multiple
species at once. If the rule form is used (generally with wildcard
characters), then the statement is not applied immediately but is stored
for use during rule expansion; during rule expansion, it is applied to
all species that match the given species pattern.

```
difm *species* *float*<sub>0</sub> *float*<sub>1</sub>  *float<sub>dim</sub>*<sub>\**dim*–1</sub>
difm *species*(*state*) *float*<sub>0</sub> *float*<sub>1</sub> *float<sub>dim</sub>*<sub>\**dim*–1</sub>
difm_rule *species*(*state*) *float*<sub>0</sub> *float*<sub>1</sub>  *float<sub>dim</sub>*<sub>\**dim*–1</sub>
```

Square root of diffusion matrix of *species* and maybe state *state*
(the dot product of this matrix and itself is the anisotropic diffusion
matrix). The matrix has *dim*<sup>2</sup> terms (*dim* is the system
dimensionality), listed row by row of the matrix; the matrix is supposed
to be symmetric. If this line is not entered, isotropic diffusion is
assumed, which leads to a faster runtime. While a matrix is used for
diffusion if one is given, the value stored with difc is used for
reaction rate calculations. If difc is not entered, the trace of the
square of this matrix, divided by the system dimensionality, is used as
a proxy for the isotropic diffusion coefficient to allow reaction rates
to be estimated. This line is most useful for restricting diffusion to a
plane or a line, in which case the square root of the diffusion
coefficient is given for each diagonal element of the matrix where there
is diffusion and 0s are place on diagonal elements for axes where
diffusion is not possible, as well as on off-diagonal elements.
*species* and or *state* may be `all` to set diffusion matrices for
multiple species at once. If the rule form is used (generally with
wildcard characters), then the statement is not applied immediately but
is stored for use during rule expansion; during rule expansion, it is
applied to all species that match the given species pattern.

```
drift *species* *float*<sub>0</sub> *float*<sub>1</sub>  *float<sub>dim</sub>*<sub>–1</sub>
drift *species*(*state*) *float*<sub>0</sub> *float*<sub>1</sub> … *float<sub>dim</sub>*<sub>–1</sub>
drift_rule *species*(*state*) *float*<sub>0</sub> *float*<sub>1</sub> … *float<sub>dim</sub>*<sub>–1</sub>
```

Drift velocity vector for molecules of type *species* and maybe state
*state*. The vector has *dim* terms (*dim* is the system
dimensionality). If this line is not entered, there is no net drift.
*species* and or *state* may be `all` to set drift vectors for multiple
species at once. If the rule form is used (generally with wildcard
characters), then the statement is not applied immediately but is stored
for use during rule expansion; during rule expansion, it is applied to
all species that match the given species pattern.

```
surface_drift *species*(*state*) *surface panel-shape* *float*<sub>0</sub> … *float<sub>dim</sub>*<sub>–2</sub>
surface_drift_rule *species*(*state*) *surface panel-shape* *float*<sub>0</sub> … *float<sub>dim</sub>*<sub>–2</sub>
```

Drift velocity vector for molecules of type *species* and state *state*,
relative to the local coordinates of the panel to which these molecules
are bound. The vector has *dim*-1 terms (*dim* is the system
dimensionality), which are for the natural coordinate system of the
local panel. *species* and or *state* may be `all` to set drift vectors
for multiple species and surface-bound states at once. If the rule form
is used (generally with wildcard characters), then the statement is not
applied immediately but is stored for use during rule expansion; during
rule expansion, it is applied to all species that match the given
species pattern.

```
mol *nmol* *name* *pos*<sub>0</sub> *pos*<sub>1</sub> … *pos<sub>dim</sub>*<sub>–1</sub>
```

Simulation starts with *nmol* type *name* molecules at location *pos*.
Each of the *dim* elements of the position may be a number to give the
actual position of the molecule or molecules; or the letter ‘u’ to
indicate that the position for each molecule should be a random value
between the bounding walls, chosen from a uniform density; or a position
range which is given as two numbers separated with a hyphen.

```
surface_mol *nmol* *species*(*state*) *surface pshape panel* *pos*<sub>0</sub> *pos*<sub>1</sub> … *pos<sub>dim</sub>*<sub>–1</sub>
surface_mol *nmol* *species*(*state*) *surface pshape panel*
```

Creates surface-bound molecules. *nmol* molecules of type *species* are
created on the surface named *surface*, on the panel with shape *pshape*
and name *panel*. They are all put in state *state*, which can be
`front`, `back`, `up`, or `down`. If additional text is entered, it
needs to be the Cartesian coordinates of the molecules, all of which are
put at the same spot and on the same panel. If the coordinates are not
given, the molecules are placed randomly on the surface with a constant
density, on average. For randomly placed molecules, it is permissible to
enter `all` for the panel, the *pshape*, and/or the surface.

```
compartment_mol *nmol species compartment*
```

Creates *nmol* solution-phase molecules of type *species* in the
compartment named *compartment*.

```
molecule_lists *listname<sub>1</sub>* *listname*<sub>2</sub> …
```

Creates and names a set of molecule lists, for molecules that are in the
system. This statement may be called multiple times.

```
mol_list *species listname*
mol_list *species*(*state*) *listname*
mol_list_rule *species*(*state*) *listname*
```

Assigns all molecules that are in the system and of type *species* and
state *state* (if *state* is not specified, then only the solution state
is assigned) to the list called *listname*. If the rule form is used
(generally with wildcard characters), then the statement is not applied
immediately but is stored for use during rule expansion; during rule
expansion, it is applied to all species that match the given species
pattern.

```
max_mol *int*
```

Optional statement (it was required up to version 2.22). This tells
Smoldyn to terminate if more than this many molecules end up being used
for the simulation.

### **Statements about graphics**

```
graphics *str*
```

Type of graphics to use during the simulation. The options are ‘none’
for no graphics, ‘opengl’ for basic and fast OpenGL graphics,
‘opengl_good’ for fair quality OpenGL graphics, and ‘opengl_better’
for pretty good graphics. Runtime gets slower with better quality. If
this line is not entered, no graphics are shown.

```
graphic_iter *int*
```

Number of time steps that should be run between each update of the
graphics. Default value is 1.

```
graphic_delay *float*
```

Minimum amount of time in milliseconds that Smoldyn should pause between
successive graphics updates. Default is 0.

```
quit_at_end *yes/no*
```

Whether Smoldyn should quit running as soon as the simulation is
complete or not. Enter yes (or 1) if it should and no (or 0) if not.

```
frame_thickness *int*
```

Thickness of the frame that is drawn around the simulation volume, in
points. Default value is 2.

```
frame_color *color* \[*alpha*\]
frame_color *red green blue* \[*alpha*\]
```

Color of the frame. All values should be between 0 and 1; use all 0s for
black and all 1s for white (default). The *alpha* value is optional and
also useless.

```
grid_thickness *int*
```

Thickness of the grid lines that can be drawn to show the virtual boxes.
Default value is 0, so that the grid is not drawn.

```
grid_color *color* \[*alpha*\]
grid_color *red green blue* \[*alpha*\]
```

Color of the grid. All values should be between 0 and 1; use all 0s for
black and all 1s for white (default). The *alpha* value is optional and
also useless.

```
background_color *color* \[*alpha*\]
background_color *red green blue* \[*alpha*\]
```

Color of the background. All values should be between 0 and 1; use all
0s for black and all 1s for white (default). The *alpha* value is
optional and may not work anyhow.

```
display_size *name* *float*
display_size *name*(*state*) *float*
display_size_rule *name*(*state*) *float*
```

Size of molecule of type *name* for display to the graphical output. If
the surface state is omitted, as in the first form shown, this display
size applies to all molecule states; otherwise it applies to only the
state listed. These states may be `solution`, `front`, `back`, `up`,
`down`, or `all`. The default value is 3, indicating that each
molecule is displayed with a small square; 0 indicates that a molecule
should not be displayed and larger numbers yield larger squares. If the
rule form is used (generally with wildcard characters), then the
statement is not applied immediately but is stored for use during rule
expansion; during rule expansion, it is applied to all species that
match the given species pattern.

```
color *name*(*state*) color \[*alpha*\]
color *name*(*state*) *red green blue* \[*alpha*\]
color_rule *name*(*state*) *red green blue* \[*alpha*\]
```

Color for displaying molecules of type *name*. If the surface state is
omitted, this color applies to just the solution state. States may be
`solution`, `front`, `back`, `up`, `down`, or `all`. Colors can be
words, or can be given with red, green, and blue values, each of which
should be between 0 and 1. Default values are 0 for each parameter,
which is black. Entering *alpha* is optional and useless. If the rule
form is used (generally with wildcard characters), then the statement is
not applied immediately but is stored for use during rule expansion;
during rule expansion, it is applied to all species that match the given
species pattern.

```
tiff_iter *int*
```

Number of time steps that should be run between each automatic saving of
a TIFF file. Default value is 0, meaning that TIFFs should not be saved
automatically.

```
tiff_name *name*
```

Root filename for TIFF files, which may include path information if
desired. Default is `OpenGL`, which leads to the first TIFF being saved
as `OpenGL001.tif`.

```
tiff_min *int*
```

Initial suffix number of TIFF files that are saved. Default value is 1.

```
tiff_max *int*
```

Largest possible suffix number of TIFF files that are saved. Once this
value has been reached, additional TIFFs cannot be saved. Default value
is 999.

```
light *number parameter color* \[*value<sub>4</sub>*\]
light *number parameter value<sub>1</sub> value<sub>2</sub> value<sub>3</sub>* \[*value<sub>4</sub>*\]
```

Set the parameters for a light source, for use with opengl_better
quality graphics. The light *number* should be between 0 and 7. The
*parameter* may be one of four strings: `ambient`, `diffuse`,
`specular`, or `position`. The first three parameters are for the
light’s colors, which are then specified with either a word or in the
*value*s as red, green, blue, and optionally alpha. The last parameter
type is for the light’s 3-dimensional position, which is specified as
*x*, *y*, and *z* in the *value*s. Lights specified this way are
automatically enabled (turned on).

```
text_color *color*
text_color *red green blue*
```

Color for text displayed on the graphics window.

```
text_display *item<sub>1</sub>* *item<sub>2</sub>* ...
```

Turns on text display of the listed items, which are listed as strings.
Possible items are `time`, which is the simulation time, and species
names and states (entered as species(state)), for which the number of
molecules of that species and state are displayed. Wildcards are
permitted.

### **Statements about run-time commands**

```
output_root *str*
```

Root of path where text output should be saved. Spaces are permitted.
Output files are saved in the same folder as the configuration file,
modified by this string. See the description for output_files. Make
sure that the destination folder has been created and that the string is
terminated with a colon (and started with a colon if needed).

```
output_files *str*<sub>1</sub> *str*<sub>2</sub> *… str*<sub>n</sub>
```

Declaration of filenames that can be used for output of simulation
results. Spaces are not permitted in these names. Any previous files
with these names will be overwritten. The path for these filenames
starts from the configuration file and may be modified by a root given
with output_root. For example, if the configuration file was called
with folder/config.txt and output_root was not used, then the output
file out.txt will appear in the folder folder too. If the configuration
file was called with folder/config.txt and the output root was given as
results/, then the output file goes to the results sub-folder of the
folder folder. The filename `stdout` results in output being sent to the
standard output (this does not need to be declared with the
output_files statement).

```
output_precision *int*
```

The precision that will be used for numerical output from commands,
meaning the number of digits displayed after a decimal point. Enter a
negative number for the default and a positive number for fixed
precision. For example, if you enter 5, then the output format string
will be ‘%.5g’.

```
append_files *str*<sub>1</sub> *str*<sub>2</sub> ... *str<sub>n</sub>*
```

Identical to output_file, except that the prior contents of these files
are not overwritten, but are appended to.

```
output_file_number *int*
```

Starting number of output file name. The default is 0, meaning that no
number is appended to a name (*e.g.* the file name out.txt is saved as
out.txt). A value larger than 0 leads to an appended file name (if 1 is
used, then out.txt is actually saved as out_001.txt). Note that the
command incrementfile increments the file number before it runs the rest
of the command.

```
output_format *str*
```

Set the output format for all observation commands. Options are the
string `ssv`, which is the default, or the string `csv`.

```
cmd b,a,e *string*
cmd @ *time string*
cmd n *int string*
cmd i *on off dt string*
cmd j *onit offit dtit string*
cmd x *on off dt xt string*
```

Declaration of a command to be run by the run-time interpreter, where
the final portion labeled *string* is the actual command. The character
following cmd is the command type, which may be ‘b’ for before the
simulation, ‘a’ for after the simulation, ‘e’ for every time step during
the simulation, ‘@’ for a single command execution at time *time*, ‘n’
for every *n*’th iteration of the simulation, ‘i’ for a fixed time
interval, ‘x’ for a fixed time multiplier, or ‘j’ for every *dtit* step
with a set starting iteration and stopping iteration. For type ‘i’, the
command is executed over the period from *on* to *off* with intervals of
at least *dt* (the actual intervals will only end at the times of
simulation time steps). For type ‘x’, the command is executed at on,
then on+dt, then on+dt\*xt, then on+dt\*xt<sup>2</sup>, and so forth.
See section 2.4 for the commands that are available.

```
max_cmd *int* (obsolete statement)
```

Maximum length of command queue. Default value is 10. As of version
1.55, this statement is no longer needed in configuration files, because
the command queue is now expanded as needed.

### **Statements about surfaces**

The statements shown below that are preceded by an asterisk need to be
entered within surface blocks, which start with start_surface and end
with end_surface. These statements can also be entered directly,
meaning not in a surface block, by preceding the statement with surface
and then the surface name.

```
max_surface *int* (obsolete statement)
```

As of version 2.19, this statement is optional. If used, it specifies
the maximum number of surfaces that will be defined. Each surface may
have many panels, including disjoint panels.

```
start_surface \[*name*\]
```

Start of surface definition block. The surface name may be given with
*name*, or it may be given afterwards with the name statement. If the
name has not been used yet for a surface, then a new surface is started.
Between this instruction and `end_surface`, all lines need to pertain
to surfaces. Parameters of one surface can be listed in multiple blocks,
or parameters for many surfaces can be listed in one block.

```
new_surface *name*
```

Defines a new surface called *name*, but does not start a surface block.
This statement is largely redundant with start_surface.

```
name *name*
```

Name of the surface for editing. This statement is not required because
the surface name can also be given with start_surface. This statement
gives the name of the current surface for editing, and creates a new
surface if needed.

```
\* action *species(state) face action* \[*new_spec*\]
```

```
\* action_rule *species(state) face action* \[*new_spec*\]
```

The behavior of molecules named *species* (and in state *state*, which
is assumed to be solution if it’s not entered) when they collide with
the *face* face of this surface. *face* can be `front`, `back`, or
`both`. If *species* is `all`, then this action applies to all
molecules. The action can be `reflect`, `absorb`, `transmit`, `jump`,
`port`, or `periodic.` If *new_spec* is entered, then the molecule
changes to this new species upon surface collision. In addition, it’s
permissible to enter the action as `multiple,` in which case the rates
need to be set with rate; alternatively, just setting the rates will
automatically set the action to `multiple.` The default is transmission
for all molecules. If the rule form is used (generally with wildcard
characters), then the statement is not applied immediately but is stored
for use during rule expansion; during rule expansion, it is applied to
all species that match the given species pattern.

```
\* rate *species(state) state1 state2 value* \[*new_spec*\]
\* rate_rule *species(state) state1 state2 value* \[*new_spec*\]
```

The rate constant for transitions from *state1* to *state2* of molecules
named *species* at this surface. For the species name, in *species*,
`all` is permitted; however, `all` is not permitted anywhere else.
Usually, *state* is omitted, but see below for where it is needed.
*state1* and *state2* can be any of: fsoln, bsoln (in solution, hitting
the front or back of the panel, respectively), front, back, up, or down.
*value* is the rate constant or rate coefficient. If *new_spec*, which
is an optional parameter, is entered, then molecules change to the
listed species at the same time as changing states. If the rule form is
used (generally with wildcard characters), then the statement is not
applied immediately but is stored for use during rule expansion; during
rule expansion, it is applied to all species that match the given
species pattern.

To specify interaction rates for molecules that collide with surface B,
while diffusing along surface A, use the first *state* parameter. In
this case: *state* is the starting surface-bound state on surface A;
*state1* is fsoln to indicate collision with the front side of surface B
or bsoln to indicate collision with the back side of surface B; and
*state2* is fsoln or bsoln to indicate transmission through surface B
and still bound to surface A (but cannot equal *state1*) or *state2* can
be a surface-bound state to indicate that the molecule hops from surface
A to surface-bound on surface B.

```
\* rate_internal *species(state) state1 state2 value* \[*new_spec*\]
\* rate_internal_rule *species(state) state1 state2 value* \[*new_spec*\]
```

This is identical to rate, except that a slightly different value is
entered. Instead of entering the surface action rate, enter the
probability of the action at each collision. Probabilities for
reflection are ignored since they are calculated as the probability that
the molecule does not transmit, absorb, or jump. If the rule form is
used (generally with wildcard characters), then the statement is not
applied immediately but is stored for use during rule expansion; during
rule expansion, it is applied to all species that match the given
species pattern.

```
\*neighbor_action *action*
```

Behavior of surface-bound molecules when they collide with a panel that
is a neighbor of the panel that they are bound to. There are only two
options: `hop` indicates that the molecule should hop onto the new panel
with a 50% probability and stay with a 50% probability, and `stay`
indicates that the molecule should stay on its own surface. The default
is `stay`.

```
\* color *face color* \[*alpha*\]
\* color *face red green blue* \[*alpha*\]
```

Color of the *face* face of the surface. *face* can be `front`, `back`,
or `both`. In the first format, *color* is a single word color, such as
`red`, `green`, `magenta`, `cyan`, etc. In the second format, color
values are numbers between 0 and 1, where 1 is maximum intensity a 0 is
minimum (1 1 1 is white). The *alpha* value is optional and describes
the opacity of the surface. If entered, it also needs to be between 0
and 1, where 1 is an opaque object (the default) and 0 is transparent.
OpenGL graphics do not work well with non-integer alpha values, so don’t
expect good results.

```
\* thickness *float*
```

Boldness of the surface in pixels for drawing purposes. This is only
relevant for 1-D and 2-D simulations, and for 3-D simulations in which
surfaces are drawn with just vertices or edges and not faces.

```
\* stipple *factor pattern*
```

Stippling of the surface edges, for drawing purposes. This is only
relevant for 3-D simulations in which surfaces are drawn with just edges
and not faces, and with opengl_good or better display method. In
*factor*, which is an integer, enter the repeat distance for the entire
stippling pattern (1 is a good choice). In *pattern*, which is a
hexidecimal integer, enter the stippling pattern between 0x0000 and
0xFFFF. 0x00FF has long dashes, 0x0F0F has medium dashes, 0x5555 has
dots, etc. Turn stippling off with 0xFFFF.

```
\* polygon *face drawmode*
```

Drawing method for the *face* face of the surface. *face* can be
`front`, `back`, or `both`. *drawmode* may be `none`, `vertex`,
`edge`, `face`, or combinations of ‘v’, ‘e’, or ‘f’ for multiple
renderings of vertices, edges, and/or faces. 2-D spheres and hemispheres
are either filled or are outlined depending on the polygon front
character. If multiple renderings are chosen in 3D, then panel faces are
shown in the requested color and other renderings are shown in black.

```
\* shininess *face value*
```

Shininess of the surface for drawing purposes. This value can range from
0 for visually flat surfaces to 128 for very shiny surfaces. This is
only relevant for some simulations.

```
\* max_panels *shape int* (obsolete statement)
```

Optional statement. This can be used to allocate memory for *int* panels
of shape *shape* for this surface, although it is usually best to let
Smoldyn allocate memory as needed. The shape may be `rect` for a
rectangle, `tri` for a triangle, `sph` for a sphere, `cyl` for a
cylinder, `hemi` for a hemisphere, or `disk` for a disk. The surface can
include panels with different shapes.

```
\* panel *shape float* … *float \[name\]*
```

Defines a new panel for the surface, where the panel has shape *shape*.
The shape may be `rect` for a rectangle, `tri` for a triangle, `sph` for
a sphere, `cyl` for a cylinder, `hemi` for a hemisphere, or `disk` for a
disk. Following the shape are numbers for the panel position, where
these depend on the shape. At the end, it is possible to enter a string
to name the panel, although this is optional (default names are the
numbers 0, 1, 2, …; names are used for jump surfaces). If the name was
used before, then this does not create a new panel, but modifies the
existing panel.

For `rect`, enter the axis number that the rectangle is perpendicular
to, preceded by a ‘+’ if the panel front faces the positive axis and a
‘-’ if it faces the negative axis (these signs must be entered); then
enter the coordinates of a corner point; then enter the dimensions of
the rectangle in sequential order of the axes, omitting the one that it
is perpendicular to. These dimensions are better called displacements
because they are added to the corner that is entered, so they may be
positive or negative. For example, for a square in a 3-D system that is
perpendicular to the *y*-axis, has sides of length 10 and is centered
about the origin, enter: `panel rect +1 -5 0 -5 10 10`. This same square
could be entered as `panel rect +1 5 0 5 -10 -10` , or with other
descriptions. A rectangle is always perpendicular to an axis.

For `tri`, enter the coordinates of the corners of the triangle. This is
one number for 1-D; 4 for 2-D, and 9 for 3-D. For 1-D, the front of the
triangle always faces the positive axis; rectangles are completely
equivalent and more versatile. For 2-D, triangles are really lines and
the front side of the line is the side on the right when traveling in
sequential order of the points that are entered. For 3-D, the triangle
front is determined by the winding direction of the corners: if one is
facing the front, the points wind counterclockwise. Unlike rectangles,
triangles do not have to be perpendicular to axes.

For `sph`, enter the coordinates of the sphere center followed by the
sphere radius and some drawing information. For 1-D, the center
coordinate is a single number and the radius is entered next. For 2-D,
the center coordinates are 2 numbers and then enter the radius followed
by the number of sides on the polygon that should be drawn to represent
the circle. For 3-D, the center coordinates are 3 numbers and then enter
the radius, followed by the number of slices (longitude lines) and
stacks (latitude lines) that are used for drawing the sphere. In the 2-D
and 3-D cases, the drawing entries are used only for drawing; the circle
or sphere functions as an accurate smooth shape. For all dimensions,
enter a positive radius to have the front of the surface on the outside
and a negative radius for it to be on the inside.

For `cyl`, enter the coordinates of the cylinder-axis start point and
the cylinder-axis end point, then the radius, and then drawing
information if appropriate. Cylinders are not permitted in 1-D. In 2-D,
two numbers give the start point and two give the end point, followed by
the radius. No drawing information is needed. In 3-D, enter three
numbers for the start point, three for the end point, the radius, and
then the number of slices and the number of stacks. For all dimensions,
enter a positive radius to have the front of the surface on the outside
and a negative radius for it to be on the inside.

For `hemi`, enter the coordinates of the hemisphere center, the radius,
and then the vector that points straight out of the hemisphere.
Hemispheres are not permitted in 1-D. In 2-D, the center coordinates are
2 numbers, the radius is 1 number, the outward vector is 2 numbers, and
finally enter the number of slices. For 3-D, the center is 3 numbers,
the radius is 1 number, the outward vector is 3 numbers, and then enter
2 numbers for the numbers of slices and stacks. The outward pointing
vector does not need to be normalized to unit length. For all
dimensions, enter a positive radius to have the front of the surface on
the outside and a negative radius for it to be on the inside.

For `disk`, enter the coordinates of the disk center, the radius, a
vector that points away from the front of the disk, and drawing
information if appropriate. Disks are not permitted in 1-D. In 2-D, the
center coordinates are 2 numbers, the radius is 1 number, and the normal
vector is 2 numbers. For 3-D, the center coordinates are 3 numbers, the
radius is 1 number, the normal vector is 3 numbers, and the number of
drawing slices is entered last. Normal vectors do not need to have unit
length.

```
\* jump *panel1 face1* -\> *panel2 face2*
\* jump *panel1 face1* \<-\> *panel2 face2*
```

Defines a molecule jumping condition for a face of a single panel. This
panel has name *panel1*, and face *face1*. The name of a panel can be
defined with the panel statement, or the default is just the shape and
panel number (rect0, sph5, etc.). A molecule that hits this face of the
panel, and that has `jump` action for this face, gets translated to the
face *face2* of the panel *panel2* (which needs to be the same shape as
the originating panel). A unidirectional arrow implies just jumping from
the first panel to the second, whereas a double-headed arrow implies
jumping in both directions.

```
\* neighbors *panel* *neigh1 neigh2 …*
```

Defines a list of panels that neighbor the panel named *panel*.
Surface-bound molecules can only diffuse from a panel to its neighbor if
the neighbors are defined in this way. This statement is unidirectional
in that it only sets, for example, *neigh1* as a neighbor of panel
*panel* but not vice versa. If the neighboring panel is not part of the
same surface as the origin panel, then specify the neighboring panel
using *surface*:*panel*.

```
\* unbounded_emitter *face species amount pos<sub>0</sub> pos<sub>1</sub> … pos<sub>dim</sub>*<sub>-1</sub>
```

Declares a molecular source for which this surface should absorb
molecules so as to yield a concentration distribution that is the same
as that which would arise with unbounded diffusion. This statement does
not create the molecular source, but only sets the panel absorption
coefficients to yield the correct concentrations, assuming the emitter
is created elsewhere (such as with a command or a zeroth order
reaction). *face* is the side of the surface that faces the emitter,
*species* is the emitted molecular species, *amount* is the emission
rate (it only matters if there is more than one emitter for this surface
and species, and then it is only the relative rates of the different
emitters that matters), and *pos* is the system-dimensional position of
the emitter. This statement is designed to be used with all emitters
strictly inside a closed surface and all of them with positive amount
values; however, neither of these criteria are checked, so other options
can be used although no promises are made regarding their behaviors.

```
\* end_surface
```

End of a block of surface definitions. Surface statements are no longer
recognized but other simulation statements are.

```
epsilon *float*
```

See `simulation settings` section. This is not entered in a surface
block.

```
margin *float*
```

See `simulation settings` section. This is not entered in a surface
block.

```
neighbor_dist *float*
```

See `simulation settings` section. This is not entered in a surface
block.

### **Statements about compartments**

The statements shown below that are preceded by an asterisk need to be
entered within compartment blocks, which start with start_compartment
and end with end_compartment. Most of these statements can also be
entered directly, preceded by the statement compartment and then the
compartment name. Both forms are shown below.

```
start_compartment *name*
```

Start of compartment definition block. The compartment name may be given
with *name*, or it may be given afterwards with the name statement. If
the name has not been used yet for a compartment, then a new compartment
is started. Between this instruction and `end_compartment`, all lines
need to pertain to compartments. Parameters of one compartment can be
listed in multiple blocks, or parameters for many compartments can be
listed in one block.

```
new_compartment *name*
```

Defines a new compartment called *name*, but does not start a
compartment block. This statement is largely redundant with
start_compartment.

```
\* name *name*
```

Name of the compartment for editing. This statement is not required
because the compartment name can also be given with start_compartment.
This statement gives the name of the current compartment for editing,
and creates a new compartment if needed.

```
\* surface *surface*
compartment *name* surface *surface*
```

Name of a bounding surface for this compartment.

```
\* point *pos<sub>0</sub> ... pos<sub>dim–1</sub>*
```

```
compartment *name* point *pos<sub>0</sub> ... pos<sub>dim–1</sub>*
```

An interior-defining point for this compartment.

```
\* compartment *logic compartment*
compartment *name* compartment *logic compartment*
```

Logically combines the compartment being defined as it has been defined
so far with the compartment that is listed in this statement. The logic
options are: equal, equalnot, and, andnot, or, ornot, and xor.

```
\* end_compartment
```

End of a block of compartment definitions. Compartment statements are no
longer recognized but other simulation statements are.

### **Statements about reactions**

```
reaction *rname* *reactant<sub>1</sub>* + *reactant<sub>2</sub>* -\> *product<sub>1</sub>* + *product<sub>2</sub>* *rate*

reaction *rname* *reactant<sub>1</sub>* + *reactant<sub>2</sub>* \<-\> *product<sub>1</sub>* + *product<sub>2</sub>* *rate<sub>fwd</sub> rate<sub>rev</sub>*

reaction compartment=*cname* *rname* *reactant<sub>1</sub>* + *reactant<sub>2</sub>* -\> *product<sub>1</sub>* + *product<sub>2</sub>* *rate*

reaction surface=*sname* *rname* *reactant<sub>1</sub>* + *reactant<sub>2</sub>* -\> *product<sub>1</sub>* + *product<sub>2</sub>* *rate*
```

The first form defines a new reaction which is named *rname*, has a list
of reactants, a list of products, and rate equal to *rate*. If there are
no reactants, meaning that it is zeroth order, enter ‘0’ as the
reactant. Similarly, if there are no products, enter ‘0’ as the sole
product. The rate value is optional. As usual, enter species states in
parentheses after the species names; `all` is permitted for reactant
states, but not for product states.

The second form shows that reversible reactions can be defined using
essentially the same statement, but with \<-\> for the arrow and,
optionally, with two rate constants. In this case, the reaction name is
appended with `fwd` for the forward reaction and with `rev` for the
reverse reaction. Entering a reversible reaction in this way is
completely equivalent to entering it as two one-way reactions; it has no
effect on the product placement, the binding radius, or other
parameters.

If this reaction should only occur in a specific compartment, enter it
with the format `compartment=*cname*` (no spaces) as one of the first
parameters. Likewise, if this reaction should only occur on a specific
surface, enter it with the format `surface=*sname*` as one of the first
parameters. These restrictions can be combined.

```
reaction_rule *rname* *reactant<sub>1</sub>* + *reactant<sub>2</sub>* -\> *product<sub>1</sub>* + *product<sub>2</sub>* *rate*
```

This is essentially identical to the reaction statement, including that
it allows the same reversibility notation and compartment and surface
restrictions. It differs in that any products that are listed here but
that have not been declared previously using the `species` statement,
get created at this point. When used with wildcards, this statement
enables rule-based modeling.

```
reaction_rate *rname rate*
```

Sets the rate constant to *rate* for reaction named *rname*.

```
reaction_multiplicity *rname multiplicity*
```

Sets the multiplicity value to *multiplicity* for reaction named
*rname*. This value is set to 1 by default, but can be changed if there
are multiple different ways for a single reaction to occur. The
requested reaction rate is multiplied by the multiplicity to give the
total reaction rate. Internally, this is an important parameter for
rule-based modeling, but the value should generally not be set using
this statement.

```
confspread_radius *rname rad*
```

Defines reaction *rname* as a conformational spread reaction. This
reaction must have two reactants and up to two products. If it has two
products, which is the most common case, then the first reactant gets
replaced by the first product, and the second with the second. They keep
their serial numbers and locations. The reaction domain extends over the
radius that is listed here (this is effectively a binding radius). If
this is entered, the reaction rate constant is interpreted as a first
order rate constant.

```
binding_radius *rname rad*
```

Sets the binding radius of reaction *rname* to *rad*.

```
reaction_probability *rname* *prob*
```


A fixed probability value for unimolecular or bimolecular reactions. For
unimolecular reactions, this is the probability of a reaction during one
time step. For bimolecular reactions, this is the probability of a
reaction occurring, given that the reactants are already closer than
their binding radius. Here, the default value is 1, which is assumed in
all rate calculations. For conformational spread reactions, this value
can be used to directly enter the reaction probability at each time
step, rather than letting it be calculated from the rate value. For
regular bimolecular reactions, this can be used to adjust the effective
reaction activation energy, although the theory has not been derived for
that yet.

```
reaction_chi *rname chi*
```

The diffusion-limited fraction (*χ*) of a bimolecular reaction. This
value is the ratio of the actual reaction rate constant to the
diffusion-limited reaction rate constant.

```
reaction_production *rname* *value*
```

Molecule production rate for zeroth order reactions. Instead of entering
the reaction rate with reaction_rate, this allows on to enter the
expectation number of molecules per time step in the entire simulation
volume.

```
product_placement *rname* *type parameters*
```

Placement method and parameters for the products of reaction *rname*.
This also affects the binding radius of the reverse reaction, as
explained in the text. The type irrev requires no parameters. Types
pgem, pgemmax, pgemmaxw, ratio, unbindrad, pgem2, pgemmax2, and ratio2
each require one parameter. Types offset and fixed each require first a
product molecule name and then a *dim*-dimensional vector as the
parameter list. If multiple products are identical, then this placement
instruction will only be applied to the first of the identical products.
For this reason, you can also specify that this statement applies to the
*n*’th product by entering the product name as product_*n* (e.g.
product_2 for the second product). The default placement method for
reversible reactions is pgemmaxw (the terminal ‘w’ implies that a
warning will be issued) with a parameter of 0.2. While it is suggested
that placement types be entered with full words, single letter codes
work as well.

To create a `bounce` type reaction, for simulating excluded volume,
enter the *type* as bounce. In this case, enter no parameter for the
default algorithm or one parameter. The default algorithm, also entered
with a -2 parameter, performs ballistic reflection for spherical
molecules. Enter a parameter of -1 for an algorithm in which the
reactant edges get separated by the same amount as they used to overlap,
along their separation vector (e.g. consider two reactants each of
radius 1, so the binding radius is set to 2; then, if the
center-to-center distance is found to be 1.6, the molecules get
separated to make the center-to-center distance equal to 2.4).
Alternatively, you can use the parameter value to define the new
separation, which should be larger than the binding radius.

```
expand_rules *iterations*
```

Expands all of the current reaction rules by *iterations* times. Enter
*iterations* as -1 for expansion to continue until all rules are fully
up-to-date (which will run for a very long time and then cause a
termination if the rules create an infinite list of species).

```
reaction_serialnum *rname rule_list*
```

Define rules for product molecule serial number assignments during
reaction *rname*. There should be as many rule values as there are
products for this reaction. The codes can be separated by ‘+’ symbols,
as in the reaction definition, but this isn’t required. Product options
include: `new` for a new serial number (the default), `r1` or `r2` for
the serial number of the first or second reactant, or `p1` to `p4` for
the serial number of the given product, or an integer greater than zero
for that value as the serial number. To use two-part serial numbers,
combine these with a dot, so for example, r1.r2 means that serial
numbers for reactants 1 and 2 should be concatenated (only pairwise
concatenation is supported). Specify a half of a two-part serial number
by suffixing the code with `R` for the right half (the default) or `L`
for the left half. For example, r1L and r1R are the left and right
halves of the serial number for reactant 1. Some of these options can
lead to multiple molecules having the same serial numbers, which is
allowed but may lead to unexpected behavior in some runtime commands.
This statement cannot be used together with the reaction_intersurface
statement for the same reaction.

```
reaction_intersurface *rname rule_list*
```

Define rules to allow bimolecular reaction named *rname* to operate when
its reactants are on different surfaces. In general, there should be as
many rule values as there are products for this reaction. For each
product choose `r1` if it should be placed on the first reactant’s
surface or relative to that surface, and `r2` if it should be placed on
the second reactant’s surface or relative to that surface (the relative
conditions are for `soln` or `bsoln` state products). The codes can be
separated by ‘+’ symbols, as in the reaction definition, but this isn’t
required. To turn off intersurface reactions, which is the default
behavior, give *rule_list* as `off`. To turn on intersurface reactions
for reactions that have no products, give *rule_list* as `on`. This
statement cannot be used together with the reaction_serialnum statement
for the same reaction.

```
reaction_log *filename rxnname serial_numbers*
```


Turns on reaction logging for all occurrences of the reaction *rxnname*
and for molecules with serial numbers that are in the serial number
list. The logging is sent to the file called *filename*. If the file is
not stdout, then it should be declared with the output_files statement.
Enter *rxnname* as `all` if all reactions should be logged. Likewise,
enter the serial number list as `all` if reactions with all molecules
should be logged. In the logging file, the output will be a single line
of text for each occurrence of the reaction with the following items:
the current simulation time, the name of the reaction, the location of
the reaction (2 numbers for 2D, 3 for 3D), the serial numbers of each
reactant, and the serial numbers of each product. If you request logging
for a specific serial number, then an entry will be created if a
molecule with this serial number is either a reactant or a product of
the reaction (however, if it is the second or higher product, then the
log entry will be missing the prior product serial numbers because these
are not recorded as they are generated). A specific reaction can only be
logged to a single place (e.g. either standard output or some file, but
not both at once).

```
reaction_log_off *rxnname serial_numbers*
```

Turns off reaction logging for the reaction *rxnname* and for molecules
with serial numbers that are listed in the serial number list. Either or
both of *rxnname* and the serial number list can be `all`.

### **Statements about ports**

The statements shown below that are preceded by an asterisk need to be
entered within port blocks, which start with start_port and end with
end_port. Most of these statements can also be entered directly,
preceded by the statement port and then the port name. Both forms are
shown below.

```
start_port *name*
```

Start of port definition block. The port name may be given with *name*,
or it may be given afterwards with the name statement. If the name has
not been used yet for a port, then a new port is started. Between this
instruction and `end_port`, all lines need to pertain to ports.
Parameters of one port can be listed in multiple blocks, or parameters
for many ports can be listed in one block.

```
new_port *name*
```

Defines a new port called *name*, but does not start a port block. This
statement is largely redundant with start_port.

```
\* name *name*
```

Name of the port for editing. This statement is not required because the
port name can also be given with start_port. This statement gives the
name of the current port for editing, and creates a new port if needed.

```
\* surface *surface*
```

```
port *name* surface *surface*
```

Name of the porting surface for this port.

```
face *face*

port *name* face *face*
```

Face of the surface that is active for porting. Enter `front` or `back`
for *face*.

```
\* end_port
```

End of a block of port definitions. Port statements are no longer
recognized but other simulation statements are.

```
*max_port int* (optional)
```

Maximum number of ports that may be defined.

### **Statements for rule-based modeling with BioNetGen**

The statements shown below that are preceded by an asterisk need to be
entered within bng blocks, which start with start_bng and end with
end_bng. Most of these statements can also be entered directly,
preceded by the statement bng and then the bng network name. Both forms
are shown below.

```
start_bng *name*
```

Start of BioNetGen block. The *name* is the network name. It may be
given here or it may be given afterwards with the name statement. If the
name has not been used yet for a network, then a network is started.
Between this instruction and `end_bng`, all lines need to pertain to
BioNetGen complexes. Parameters of one network can be listed in multiple
blocks, or parameters for many networks can be listed in one block.

```
end_bng
```

End of a block of bng definitions. Bng statements are no longer
recognized but other simulation statements are.

```
name *name*
```

Name of the bng network for editing. This statement is not required
because the network name can also be given with start_bng. This
statement gives the name of the current network for editing, and creates
a new network if needed.

```
BNG2_path *path*
```

Directory path and complete filename of BNG2.pl software. The default
path for Mac and Linux systems is /usr/local/bin/BioNetGen/BNG2.pl and
for Windows is C:\\Program Files\\Smoldyn\\BioNetGen\\BNG2.pl. The
*path* parameter is allowed to have spaces in it.

```
multiply unimolecular_rate *value*
multiply bimolecular_rate *value*
```

Factor that will be multiplied with unimolecular and bimolecular
reaction rates that are listed in the current network. This statement is
useful for converting units if the rules file and Smoldyn file were
written with different unit systems.

```
monomer *monomer*<sub>1</sub> *monomer*<sub>2</sub> ...
monomers *monomer*<sub>1</sub> *monomer*<sub>2</sub> ...
```

Declares one or more monomer names. This statement is optional because
monomer names are also inferred from the species long names and can be
given using monomer_state or other monomer functions. This is primarily
useful for enabling the use of the `all` designation for monomers in
other monomer statements.

```
monomer_difc *monomer difc*
```

The diffusion coefficient for monomer called *monomer*. The monomer
value can be a single monomer or can be `all` for all currently declared
monomers. A monomer has a single diffusion coefficient, independent of
its state.

```
monomer_display_size *monomer size*
```

The display size for monomer called *monomer*. The monomer value can be
a single monomer or can be `all` for all currently declared monomers. A
monomer has a single display size, independent of its state.

```
monomer_color *monomer color*
monomer_color *monomer red green blue*
```

The color for monomer called *monomer*. The monomer value can be a
single monomer or can be `all` for all currently declared monomers. The
color can be either a color word or the red, green, and blue color
values. A monomer has a single color, independent of its state.

```
monomer_state *monomer state*
```

The default state for a monomer. States can be any of: soln (same as
fsoln), bsoln, front, back, up, and down (this list is ordered from
lowest to highest precedence). These states are used when Smoldyn
assigns states to reaction products, which are often composed of many
monomers.

```
expand_rules *filename*
```

Filename for a rules file written in the BNGL language, which should
have a .bngl suffix. When Smoldyn encounters this statement, Smoldyn
calls BNG2.pl to expand the file and save it as a .net file but does not
then read the result. After this line, include a read_file statement
and list the filename, now with a .net suffix, so that Smoldyn reads in
the expanded network. There is typically very little error reporting if
BNG2.pl encounters an error in the .bngl file. To see the errors, run
Smoldyn with the -v command line option, for verbose operation, and then
Smoldyn will display all of the BNG2.pl output.

### **Statements for lattices**

The statements shown below that are preceded by an asterisk need to be
entered within lattice blocks, which start with start_lattice and end
with end_lattice. Most of these statements can also be entered
directly, preceded by the statement lattice and then the lattice name.
Both forms are shown below.

```
start_lattice *name*
```

Start of the lattice block. The lattice name may be given with *name*,
or it may be given afterwards with the name statement. If the name has
not been used yet for a lattice, then a new lattice is started. Between
this statement and end_lattice, all lines need to pertain to lattices.
Parameters of one lattice can be listed in multiple blocks, or
parameters for many lattices can be listed in one block.

```
\* name *name*
```

Name of the lattice for editing. This statement is not required because
the lattice name can also be given with start_lattice. This statement
gives the name of the current lattice for editing, and creates a new
lattice if needed.

```
\* type *type*
```

Type of the lattice. At present, this accepts two *type* strings, `nsv`
and `pde`, which stand for next-subvolume method and partial
differential equation method, respectively. However, only the NSV method
has been implemented, so that’s the only type that should be entered.
This statement is optional, with NSV assumed if it is not entered.

```
\* port *port*
```

Name of the port that the lattice uses to exchange molecules with the
particle-based simulation.

```
\* boundaries *dim pos1 pos2*
\* boundaries *dim pos1 pos2 type*
```

Creates lower and upper boundaries for the lattice region, where *dim*
is the dimension that is bounded in this statement, *pos1* is the lower
bound, and *pos2* is the upper bound. In the second form, *type* is a
character that represents the boundary type, which may be ‘r’ for
reflective or ‘p’ for periodic. This syntax is essentially identical to
the boundaries statement that is used to define the particle-based
simulation volume.

```
\* lengthscale *x1*
\* lengthscale *x1 x2*
\* lengthscale *x1 x2 x3*
```

Specifies the partition spacing within the lattice region of space. Use
the first form for 1D systems, the second for 2D systems, and the third
for 3D systems. The partition spacing values should be even divisors of
the lattice dimensions that are given with the boundaries statement.

```
\* species *species1 species2 ...*
```

List of species that should be used in the lattice region of space.
These species need to have been declared previously in the particle
region of space. This line may be entered multiple times. Rather than
listing all species, the `all` keyword can be used to state that all of
the current particle-side species should also be used on the lattice
side.

```
\* make_particle *face species1 species2* ...
```

Causes all molecules of the listed species to be converted from lattice
representation to particle representation if they diffuse across the
face called *face* (front, back, or both) of the lattice’s port.

```
\* reaction *reaction1 reaction2 ...*
\* reaction move *reaction1 reaction2 ...*
```

List of reactions that should be used in the lattice region of space.
These reactions need to have been fully defined previously in the
particle region of space. Rather than listing all reactions, the keyword
`all` can be used to state that all of the current particle-side
reactions should also be functional on the lattice side. If the keyword
`move` is given in the list, as in the latter form above, then all
subsequent listed reactions are `moved` to the lattice side, meaning
that they are functional on the lattice side but become non-functional
on the particle side. In this case, they are still defined on the
particle side, but are simply disabled.

```
\* mol *nmol* *name* *pos*<sub>0</sub> *pos*<sub>1</sub> … *pos<sub>dim</sub>*<sub>–1</sub>
```

This adds molecules to the starting state of the simulation in the
lattice region of space. This statement is essentially identical to the
statement with the same name that is in the particle portion of the
configuration file. The lattice regions starts with *nmol* type *name*
molecules at location *pos*. Each of the *dim* elements of the position
may be a number to give the actual position of the molecule or
molecules; or the letter ‘u’ to indicate that the position for each
molecule should be a random value between the bounding walls, chosen
from a uniform density; or a position range which is given as two
numbers separated with a hyphen.

```
\* end_lattice
```

End of a block of lattice definitions. Lattice statements are no longer
recognized but other simulation statements are.

### **Statements for simulation settings**

```
random_seed *seed*
```

Seed for random number generator, which can be any integer. If this line
is not entered (or if you set the *seed* value to `time`), the current
time is used as a seed, producing different sequences for each run.
(This statement was called rand_seed through version 2.28.)

```
accuracy *float*
```

A parameter that determines the quantitative accuracy of the simulation,
on a scale from 0 to 10. Low values are less accurate but run faster.
Default value is 10, for maximum accuracy. Bimolecular reactions are
only checked for pairs of reactants that are both within the same
virtual box when accuracy is 0 to 2.99, reactants in nearest neighboring
boxes are considered as well when accuracy is 3 to 6.99, and reactants
in all types of neighboring boxes are checked when accuracy is 7 to 10.

```
molperbox *float*
```

Virtual boxes are set up initially so the average number of molecules
per box is no more than this value. The default value is 5. boxsize is
an alternate way of entering comparable information.

```
boxsize *float*
```

Rather than using molperbox to specify the sizes of the virtual boxes,
boxsize can be used to request the width of the boxes. The actual box
volumes will be no larger than the volume calculated from the width
given here.

```
gauss_table_size *int*
```

This sets the size of a lookup table that is used to generate
Gaussian-distributed random numbers. It needs to be an integer power of
2. The default value is 4096, which should be appropriate for nearly all
applications.

```
epsilon *float*
```

Maximum allowed distance separation between a surface-bound molecule and
the surface. The default value, which is extremely small, is good for
most applications.

```
margin *float*
```

The distance inside of a panel edge to which Smoldyn moves surface-bound
molecules that diffuse off of a panel. The default value, which is
extremely small, is good for most applications.

```
neighbor_dist *float*
```

Maximum distance that surface-bound molecules will jump across space to
diffuse from one panel to a neighboring panel. In Smoldyn 2.37 and
higher versions, the default for this value is extremely small, just
large enough to prevent round-off error. It should not need to be
changed. In prior versions, the default value was 3 times the maximum
rms step length of surface-bound molecules, which was necessary due to a
different surface-bound molecule diffusion algorithm.

### **Deprecated statements**

The following statements are supported for backward compatibility and
might be removed in future Smoldyn releases.

```
*max_species int*
```

*This tells Smoldyn to allocate this many molecule species. As of
version 2.23, new species are allocated as needed.*

```
*max_names int*
```

*Maximum number of molecular species that will be used. Statement has
been superseded by max_species, which is now also obsolete.*

```
*name name*
```

*Name of a molecule. Statement has been superseded by species.*

```
*names name<sub>1</sub> name<sub>2</sub> … name<sub>n</sub>*
```

*Names of all of the types of molecules present in the system. Statement
has been superseded by species.*

```
*max_compartment int*
```

*Maximum number of compartments that will be defined. New compartments
are now allocated as needed.*

```
*reaction_cmpt cname rname reactant<sub>1</sub> + reactant<sub>2</sub> -\> product<sub>1</sub> + product<sub>2</sub>*

*reaction_cmpt cname rname reactant<sub>1</sub> + reactant<sub>2</sub> -\> product<sub>1</sub> + product<sub>2</sub> rate*
```

*This is identical to the reaction statement except that this reaction
only occurs within the compartment cname. All reaction orders work. For
bimolecular reactions, both reactants need to be in the compartment for
the reaction to occur. Statement has been superseded by `reaction
compartment=cname` format.*

```
*reaction_surface sname rname reactant<sub>1</sub> + reactant<sub>2</sub> -\> product<sub>1</sub> + product<sub>2</sub>*

*reaction_surface sname rname reactant<sub>1</sub> + reactant<sub>2</sub> -\> product<sub>1</sub> + product<sub>2</sub> rate*
```

*This is identical to the reaction statement except that this reaction
only occurs on the surface sname. All reaction orders work. For
bimolecular reactions, at least one reactant needs to be bound to the
named surface for the reaction to occur. Statement has been superseded
by `reaction surface=sname` format.*

```
*reaction_permit rname state*

*reaction_permit rname state<sub>1</sub> state<sub>2</sub>*
```

*Allow reaction rname to happen for the specified reactant states, which
need to be listed in the same order in which the reactants were entered.
This function has not proven to be useful; instead, it’s best to enter
each reaction individually, rather than to have some reactions that
operate on multiple states.*

```
*reaction_forbid rname state*

*reaction_forbid rname state<sub>1</sub> state<sub>2</sub>*
```

*Forbid reaction rname to happen for the specified reactant states,
which need to be listed in the same order in which the reactants were
entered. Again, this function has not proven to be useful.*

```
*pthreads int*
```

*Sets the number of threads that Smoldyn should run with. Enter 0 for
unthreaded operation (the default) or a larger number (including 1) for
multi-threaded operation. None of the multi-threaded functions have been
tested thoroughly, so they may create incorrect results or program
crashes. They are also not substantially faster than the unthreaded
versions of the same functions. Thus, use of multi-threaded operation is
not generally recommended.*

*The following reaction statements are for block-style input and are
only supported for backward compatibility.*

```
*start_reaction*

*start_reaction order*
```

*Start of reaction definition. Between this instruction and
`end_reaction`, all lines need to pertain to this order of reaction.
It is permissible to list reactions of the same order in multiple
blocks, provided that only the first block includes a max_rxn statement
and that sufficient reactions are declared with that statement.*

```
*order int*
```

*Order of the reactions being declared (0, 1, or 2).*

```
*max_rxn max_rxn*
```

*Maximum number of reactions that will be declared of the given order.*

```
*reactant r<sub>0</sub> r<sub>1</sub> … r<sub>nrxn–1</sub>*

*reactant name(state) r<sub>0</sub> r<sub>1</sub> … r<sub>nrxn–1</sub>*

*reactant name<sub>1</sub>(state<sub>1</sub>) + name<sub>2</sub>(state<sub>2</sub>) r<sub>0</sub> r<sub>1</sub> … r<sub>nrxn–1</sub>*
```

*Declaration of reactants and reaction names for zeroth order,
unimolecular, and bimolecular reactions, respectively. The listed
molecule names are the reactants and the following strings are the
respective reaction names. Note that there are spaces before and after
the ‘+’ symbol. States are optional. If they are omitted, only the
solution states are allowed to react, whereas if they are included then
only the states listed can react. States may be set to `all`.*

```
*permit name(state) r value*

*permit name(state) + name(state) r value*
```

*Set permission of reaction r, for the specified reactants, to value,
which should be 0 or 1; 0 means the reaction does not happen, 1 means it
does. The default depends on the states that were entered with the
reactant statement, if any.*

```
*rate r rate*
```

*Reaction rate constant for reaction called r. Units for the reaction
rate constant are (volume)<sup>order–1</sup> times inverse time. These
rates are converted by the program into probabilities or binding radii.
To enter the simulation parameters directly, use rate_internal.
Conformational spread reactions are entered with order 2, but this rate
constant has units of inverse time.*

```
*confspread_radius r float*
```

*Defines reaction r a a conformational spread reaction. This reaction
must have two reactants and two products; upon reaction, the first
reactant is replaced by the first product, and the second with the
second. The reaction domain extends over the radius that is listed here
(this is effectively a binding radius).*

```
*rate_internal r float*
```

*Internal value for reaction rate information, which can be used to
override the internal rates that are calculated from the rate entry. For
zeroth order reactions, this is the expectation total number of
reactions per time step; for unimolecular reactions, this is the
reaction probability per time step for each reactant molecule; and for
bimolecular reactions, this is the binding radius. This has no effect
for conformational spread reactions.*

```
*probability r float*
```

*A fixed probability value for bimolecular reactions. The default value
is 1, which is assumed in all rate calculations. However, this statement
allows the user to set a different probability, which is ignored in all
rate calculations, but can be useful in some situations. For
conformational spread reactions, this value can be used to directly
enter the reaction probability at each time step, rather than letting it
be calculated from the rate value. For simple reactions, this can be
used to adjust the effective reaction activation energy, although the
theory has not been derived for that yet.*

```
*product r name + name + … + name*

*product r name(state) + name(state) + … + name(state)*
```

*List of products for reaction r. States for products may be entered,
which can be `solution`, `fsoln`, `bsoln`, `front`, `back`, `up`, or
`down`; if no state is given, solution is assumed.*

```
*product_param r i*

*product_param r p,x,X,r,b,q,y,s float*

*product_param r o,f prod_name pos<sub>0</sub> pos<sub>1</sub>… pos<sub>dim–1</sub>*
```

*Parameters for the initial placement of products of reaction r. A
product parameter also affects the binding radius of the reverse
reaction. These are explained in section 3. In the first format, a type
of ‘i’ indicates that the reverse reaction is ignored for calculations.
The second format uses one of the type letters shown: ‘p’ and ‘q’ are
geminate rebinding probabilities, ‘x’ and ‘y’ are maximum geminate
rebinding probabilities, ‘r’ and ‘s’ are ratios of unbinding to binding
radii, and ‘b’ is a fixed unbinding radius. The third format yields
products that have a fixed relative orientation, which is either
randomly rotated with ‘o’, or not rotated with ‘f’. In the absence of
better information, a useful default parameter type is either ‘x’ or
‘y’, with a value of about 0.2. ‘X’ is identical to ‘x’, and is
intended only as an internal code to indicate that the user didn’t enter
a product_param line, which will yield a default value of 0.2 and a
warning.*

```
*end_reaction*
```

*End of reaction definition. Reaction instructions are no longer
recognized but other simulation instructions are.*


## Math operations and functions

**Mathematics operations, listed in order of precedence**

Functions see list below

- `()`, `[]`, `{}` parentheses, brackets, and braces (e.g. `x[y+z]`)
- `^` powers (e.g. *x*^*y*)
- `*` `/` `%` multiplication, division, modulo (e.g. `x*y`)
- `+` `-` addition, subtraction (e.g. `x+y`)

### **Functions of one variable**

- cos(*x*) acos(*x*) cosh(*x*)
- sin(*x*) asin(*x*) sinh(*x*)
- tan(*x*) atan(*x*) tanh(*x*)
- exp(*x*) log(*x*) log10(*x*)
- sqrt(*x*) fabs(*x*)
- floor(*x*) ceil(*x*)

### **Functions of two variables**

- atan2(*x*,*y*)
- pow(*x*,*y*)
- rand(*x*,*y*)

## Runtime Commands

**Simulation control commands**

```
stop
```

Stop the simulation.

```
pause
```

This puts the simulation in pause mode. If opengl graphics are used,
continuation occurs when the user presses the spacebar. When graphics
are not used, the user is told to press enter.

```
beep
```

The computer beeps when this is reached. Nothing else is done.

```
keypress *char*
```

Send a signal to the graphics manipulation component of the program to
execute the behavior that would occur when a key is pressed. For the
arrows, and shift-arrows, the character should be r for right, l for
left, u for up, d for down, and the respective upper case characters for
the shift-arrows.

```
setflag *number*
```

Sets the global command flag value to *number*, which can be a floating
point value. This is generally used after a conditional command, and is
then queried by one or more ifflag commands.

```
setrandseed *seed*
```

Sets the random number seed to the specified integer value. If the seed
listed is less than 0, the current time is used for the seed.

```
setgraphics *type*
```

Sets the display graphics to type *type*. If graphics were not set up
initially, using the graphics statement, this command does nothing.
Otherwise, options for type are `opengl` or `opengl_good`.

```
setgraphic_iter *timesteps*
```

Sets the graphics update interval to *timesteps* time steps. This is
only functional if graphics were set up initially, using the graphics
statement.

```
updategraphics
```

Update the graphics window.

## **File manipulation commands**

```
overwrite *filename*
```

Erases the output file called *filename* but leaves it open for more
writing. (In versions 1.77 and earlier, this called another command.)

```
incrementfile *filename*
```

A new output file is created based upon the *filename*. The first time
this is called the filename is appended with a `_001`, which is then
incremented with subsequent calls to `_002`, and so on. These numbers
precede any suffix on the filename. (In versions 1.77 and earlier, this
called another command.)

## **Conditional commands**

```
if *value1 symbol value2 command*
```

Runs command *command*, depending on the relative values of *value1* and
*value2*. The *symbol* character can be ‘\<’, ‘\>’ or ‘=’.

```
ifflag *symbol number command*
```

Run command *command*, depending on the value of the global command
flag. Enter *symbol* as ‘\<’ if *command* should be run if the flag
value is less than *number*, as ‘\>’ for the flag value greater than
*number*, and as ‘=’ for the flag value equal to *number*.

```
ifprob *value command*
```

Run command *command* with probability *value*, which should be between
0 and 1. If you want to run multiple commands with this probability use
the setflag and ifflag commands.

```
ifno *species*(*state*) *command*
```

Run command *command* if no molecule of type *species* remains. The
molecule state *state* is optional, with `solution` as a default. The
name and/or the state may be `all`.

```
ifless *species*(*state*) *num* *command*
```

Run command *command* if there are fewer than *num* molecules of type
*species* remaining. The molecule state *state* is optional, with
`solution` as a default. The name and/or the state may be `all`.

```
ifmore *species*(*state*) *num* *command*
```

Run command *command* if there are more than *num* molecules of type
*species*. The molecule state *state* is optional, with `solution` as a
default. The name and/or the state may be `all`.

```
ifincmpt *species*(*state*) *char* *number* *compartment* *command*
```

Run command *command* depending on how the number of molecules of type
*species* within compartment *compartment* compares with *number*. Enter
*char* as ‘\<’ if *command* should be run with less than *number*
molecules, as ‘\>’ for more than *number*, or as ‘=’ for equal to
*number*. A space is required between this symbol and *number*. The
molecules’ state *state* is optional, with `solution` as a default. The
species and/or state may be `all`.

```
ifchange *species*(*state*) *char num* *command*
```

Run command *command* if the number of molecules of type *species*
changes (or doesn’t change) relative to the previous time this command
was called. The molecule state *state* is optional, with `solution` as a
default. The name and/or the state may be `all`. Enter *char* as ‘\>’ to
respond to a difference that is greater than *num*, ‘\<’ to respond to a
difference that is less than *num*, ‘=’ to respond to a difference that
is exactly equal to *num*, or ‘!’ to respond to a difference that does
not equal *num*. For example, enter *char* and *num* as `! 0` to
respond to any change in molecule numbers, or as `\< -10` to respond to
any decrease that exceeds 10 molecules.

## **System observation commands**

For all of the observation commands, if *filename* is the last
parameter, then it may be omitted and results will be output to stdout.

```
warnescapee *species*(*state*) *filename*
```

Looks for molecules of type *species* that are outside the system
boundaries, printing results to *filename*. If there are none, nothing
is printed. For each molecule that escaped during the last time step,
this prints a line of text that says where the molecule was, where it
is, and where it crossed the last surface. Setting *species* to `all`
allows all molecule types to be checked. This needs to be run at every
time step to catch all escaping molecules.

```
echo *filename* "*string*"
```

Just prints the text within double quotes to the file, without any
terminal newline. If you want a newline, include \\n at the end of the
string. The quotes need to be plain quotes, not curly quotes.

```
evaluate *filename expression*
```

Evaluates the given mathematical expression, printing the result to the
file. The results are only likely to be interesting if the expression is
one of the Smoldyn functions.

```
molcountheader *filename*
```

This prints one line of display with the word `time` and then the name
of each molecule species. This is intended to be used as a header line
for the molcount, molcountinbox, etc. commands.

```
molcount *filename*
```

Each time this command is executed, one line of display is printed to
the listed file, giving the time and the number of molecules for each
molecular species. Molecule states are ignored. The ordering used is the
same as was given in the species command. This command accounts for
molecules in lattices and their associated ports.

```
molcountinbox *xlow xhigh filename*
molcountinbox *xlow xhigh ylow yhigh filename*
molcountinbox *xlow xhigh ylow yhigh zlow zhigh filename*
```

Each time this command is executed, one line of display is printed to
the listed file, giving the time and the number of molecules that are
within the box that is defined by the low and high parameter, for each
molecular species. Molecule states are ignored. The ordering used is the
same as was given in the species statement.

```
molcountincmpt *compartment filename*
```

Each time this command is executed, one line of display is printed to
the listed file, giving the time and the number of molecules that are
within the compartment *compartment* for each molecular species. Only
solution-phase molecules are listed. The ordering used is the same as
was given in the species statement.

```
molcountincmpts *compartment<sub>1</sub> compartment<sub>1</sub> …
compartment<sub>n</sub> filename*
```

Each time this command is executed, one line of display is printed to
the listed file, giving the time and the number of molecules that are
within each of the compartments listed, for each molecular species. Up
to 16 compartments may be listed. Only solution-phase molecules are
reported to the output. The molecule ordering used is the same as was
given in the species statement.

```
molcountincmpt2 *compartment state filename*
```

Identical to molcountincmpt except that this counts molecules that are
in state *state*. Entering state as `all` means that molecules of all
states are counted. Note that the surfaces that bound a compartment are
included in that compartment.

```
molcountonsurf *surface filename*
```

Each time this command is executed, one line of display is printed to
the listed file, giving the time and the number of molecules that are
bound to the surface *surface* for each molecular species. The molecule
state is not printed. The ordering used is the same as was given in the
species statement.

```
molcountspace *species*(*state*) *axis low high bins average filename*

molcountspace *species*(*state*) *axis low high bins low high average filename*

molcountspace *species*(*state*) *axis low high bins low high low high average filename*
```

This command measures a line profile of molecules. It only counts
molecules of type *species*, with an optional *state* specification,
although *species* and/or *state* can be `all`. The line profile is
along axis number *axis*, which is ‘x’, ‘y’, or ‘z’ (or a number between
0 and the system dimensionality minus 1), extends from *low* to *high*,
and is comprised of *bins* equally spaced bins (*i.e.* it’s a
histogram). These bins extend exactly from *low* to *high*, and thus do
not count any molecules that are outside this range. For two dimensions,
the line width and lateral position are specified with another pair of
*low* and *high* values; for three dimensions, two more pairs of *low*
and *high* values are required which now specify the sides of a
rectangular cross-section tube. To illustrate the sequence of
parameters, suppose the command is used in a 3-D system to show
concentration variation along the *y*-axis. In this case, axis is y, the
first *low* and *high* indicate the ends of the measurement range along
the *y*-axis, the next *low* and *high* indicate the domain on the
*x*-direction, and the third *low* and *high* indicate the domain on the
*z*-direction. Set the average input to 0 to not use averaging, in which
case there is output at every command execution. Otherwise, this only
produces an output every average iterations, at which point it outputs
means that were collected over the preceding iterations. At each output
time, the command outputs a single line of text to *filename* with the
time followed by the numbers (or average numbers) of the specified
molecules in each histogram bin. This command accounts for molecules in
lattices, but not for molecules in ports.

```
molcountspace2d *species*(*state*) z *low<sub>x</sub> high<sub>x</sub> bins<sub>x</sub> low<sub>y</sub> high<sub>y</sub> bins<sub>y</sub> average filename*

molcountspace2d *species*(*state*) *axis low high bins low high bins low high average filename*
```

This command measures a 2D histogram of molecules. It is identical to
molcountspace, but for 2D. The area is perpendicular to the listed axis
(must be entered as z for 2D simulations and selectable for 3D
simulations). It extends from *low* to *high*, and is composed of *bins*
equally spaced bins, entered separately for each of the two axes. For 3D
simulations, region thickness is specified with another pair of *low*
and *high* values. Set the average input to 0 to not use averaging, in
which case there is output at every command execution. Otherwise, this
only produces an output every average iterations, at which point it
outputs means that were collected over the preceding iterations. At each
output time, the command outputs the time to *filename*, followed by a
grid of numbers of the molecules in each histogram bin. This command
does not account for molecules in lattices or ports.

```
molcountspaceradial *species*(*state*) *center<sub>x</sub> radius bins average filename*

molcountspaceradial *species*(*state*) *center<sub>x</sub> center<sub>y</sub> radius bins average filename*

molcountspaceradial *species*(*state*) *center<sub>x</sub> center<sub>y</sub> center<sub>z</sub> radius bins average filename*
```

This command measures a radial profile of molecules. It is identical to
molcountspace, except that counts molecules in radial rings, rather than
along a line profile. It only counts molecules of type *species*, with
an optional *state* specification, although *species* and/or *state* can
be `all`. Enter the center position in *center*, with the number of
entries equal to the system dimensionality, and the investigated radius
in *radius*. The histogram is comprised of *bins* equally spaced bins.
The volume investigated includes all edges. Set the average input to 0
to not use averaging, in which case there is output at every command
execution. Otherwise, this only produces an output every average
iterations, at which point it outputs means that were collected over the
preceding iterations. At each output time, the command outputs a single
line of text to *filename* with the time followed by the numbers (or
average numbers) of the specified molecules in each histogram bin. This
command does not account for molecules in lattices or ports.

```
molcountspacepolarangle *species*(*state*) *center<sub>x</sub> center<sub>y</sub> pole<sub>x</sub> pole<sub>y</sub> radius<sub>min</sub> radius<sub>max</sub> bins average filename*

molcountspacepolarangle *species*(*state*) *center<sub>x</sub> center<sub>y</sub> center<sub>z</sub> pole<sub>x</sub> pole<sub>y</sub> pole<sub>z</sub> radius<sub>min</sub> radius<sub>max</sub> bins average filename*
```

This command measures a polar angle profile of molecules. It is
identical to molcountspace (and molcountspaceradial), except that counts
molecules in angular segments, rather than along a line profile. It only
counts molecules of type *species*, with an optional *state*
specification, although *species* and/or *state* can be `all`. Enter the
center position in *center*, with the number of entries equal to the
system dimensionality. Enter the vector that points towards the pole in
*pole*, again with the number of entries equal to the system
dimensionality. All molecules between *radius<sub>min</sub>* and
*radius<sub>max</sub>* will be included; set either or both to -1 to
remove the respective constraint. The histogram is comprised of *bins*
equally spaced bins. These bins extend from 0 to 2π for a 2D system and
from 0 to π in a 3D system. The volume investigated includes all edges.
Set the average input to 0 to not use averaging, in which case there is
output at every command execution. Otherwise, this only produces an
output every average iterations, at which point it outputs means that
were collected over the preceding iterations. At each output time, the
command outputs a single line of text to *filename* with the time
followed by the numbers (or average numbers) of the specified molecules
in each histogram bin. This command does not account for molecules in
lattices or ports.

```
radialdistribution *species<sub>1</sub>*(*state*) *species<sub>2</sub>*(*state*) *radius bins average filename*
```

Computes the radial distribution function for molecules of
*species*<sub>2</sub> about those of *species*<sub>1</sub>. These are
allowed to be the same species, can include wildcards, can be `all`,
etc. Enter the maximum radius to be considered in *radius*, the number
of bins in the histogram in *bins*, the number of iterations to be
averaged together in *average* (or 0 for no averaging), and the output
filename in *filename*. Outputs the time followed by the radial
distribution function, where the radial distribution function values are
the average molecule densities (units of inverse volume) in each radial
bin. Divide these results by the overall average molecule density to get
the conventional radial distribution function, which typically
approaches 1 for large distances. This function accounts for periodic
boundaries if they are used.

```
radialdistribution2 *species<sub>1</sub>*(*state*) *species<sub>2</sub>*(*state*) *low<sub>x</sub>* *high<sub>x</sub>* *low<sub>y</sub>* *high<sub>y</sub>* *low<sub>z</sub>* *high<sub>z</sub> radius bins average filename*
```

Identical to radialdistribution except that this also allows selection
of a volume region. Enter the low and high values for as many dimensions
as the simulation uses. This computes the radial distribution about all
molecules of *species<sub>1</sub>* that are within the given volume;
*species<sub>2</sub>* molecules are considered that are outside of this
volume (up to distance *radius* outside of it).

```
molcountspecies *species*(*state*) *filename*
```

Prints out a single line of text to *filename* with time and the number
of molecules of the listed species that are in state *state*. Either or
both of *species* and *state* may be `all`. If *state* is not included,
solution is assumed.

```
molcountspecieslist *filename species*(*state*) *species*(*state*) ...  *species*(*state*)
```

Prints out a single line of text to *filename* with time and the number
of molecules of each of the listed species and states. Either or both of
*species* and *state* may be `all`. If *state* is not included, solution
is assumed.

```
mollistsize *listname filename*
```

Prints out a single line of text to *filename* with the total number of
molecules in the molecule list named *listname*. This is allowed to be
`all`.

```
listmols *filename*
```

This prints out the identity, state, location, and serial number of
every molecule in the system to the listed file name, using a separate
line of text for each molecule.

```
listmols2 *filename*
```

This is very similar to listmols but has a slightly different output
format. Each line of text is preceded by the `time counter`, which is an
integer that starts at 1 and is incremented each time the routine is
called. Also, the species and state names of molecules are not printed,
but instead the species and state numbers are printed.

```
listmols3 *species*(*state*) *filename*
```

This is identical to listmols2 except that it only prints information
about molecules of type *species*. *state* is optional; *species* and/or
*state* can be `all`.

```
listmols4 *species*(*state*) *filename*
```

This is identical to listmols3 except that it accounts for wrap-arounds
when displaying molecule positions. In other words, if the *x*-axis
ranges from 0 to 10, a molecule starts in the middle at *x* = 5 and
diffuses to the right for a full lap, returning to the middle of the
system, then its *x* value is printed here as 15, rather than as 5 as it
is for listmols3. *state* is optional; *species* and/or *state* can be
`all`.

```
listmolscmpt *species*(*state*) *cmpt filename*
```

This prints out the time counter (see listmols2), species, state, and
location, and serial number of every molecule that is within compartment
*cmpt*. It only prints information about molecules of type *species*.
*state* is optional; *species* and/or *state* can be `all`.

```
molpos *species*(*state*) *filename*
```

This prints out the time and then the positions of all molecules of type
*species* on a single line of text, to the listed filename. *state* is
optional; *species* and/or *state* can be `all`.

```
trackmol *serno filename*
```

Outputs the time and the species, state, serial number, location, and
inside vs. outside compartment status for each compartment of the single
molecule with serial number *serno*. This stops after it finds the first
molecule with the requested serial number. This supports two-part serial
numbers (see reaction_serialnum) in which a match occurs if *serno*
exactly matches either the whole molecule serial number or either half
of it.

```
molmoments *species*(*state*) *filename*
```

This prints out the positional moments of the molecule type given to the
listed file name. All the moments are printed on a single line of text;
they are the number of molecules, the mean position vector (*dim*
values), and the variances on each axis and combination of axes
(*dim*<sup>2</sup> values). *state* is optional; neither *species* nor
*state* can be `all`.

```
savesim *filename*
```

This writes the complete state of the current system to the listed file
name, in a format that can be loaded in later as a configuration file.
Note that minor file editing is often desirable before simulating a file
saved in this manner. In particular, the saved file will declare its own
name as an output file name, which will erase the configuration file.

```
meansqrdisp *species*(*state*) *dim* *filename*
```

This function is used to measure mean square displacements (diffusion
rates) of molecules of type *species*, along dimension *dim* (‘x’, ‘y’,
or ‘z’, or 0, 1, or 2) printing the results to *filename*. When it is
first invoked, it records the positions of all molecules of type
*species*. Then, and every subsequent time it is called, it compares the
current positions of all molecules that still exist to the old ones,
calculates the average squared displacement (〈*r*<sup>2</sup>〉), and
prints the time and that number to a single line in the output file. If
*dim* is `all`, this sums the mean square displacement for all
dimensions, otherwise *dim* should be a dimension number. As of version
1.73, this accounts for periodic boundaries. *state* is optional;
neither *species* nor *state* can be `all`. As of version 1.84, this
prints out three numbers in each line: time, 〈*r*<sup>2</sup>〉, and
〈*r*<sup>4</sup>〉. This command does not work if multiple molecules
have the same serial number (which can only happen if you use the
reaction_serialnum statement) and, for molecules with two-part serial
numbers, it determines molecule identity based on only the right part.

```
meansqrdisp2 *species*(*state*) *dim start report max_mol max_moment* *filename*
```

This function is an expanded version of meansqrdisp. As above, it
measures mean square displacements of molecules of type *species*, along
dimension *dim* (‘x’, ‘y’, or ‘z’, or 0, 1, or 2), and prints the
results to *filename*. The *start* and *report* arguments control when
this command starts tracking molecules and when it reports their mean
square displacements, respectively. For *start*, enter ‘i’ to track
molecules that exist when the command is initially invoked, enter ‘c’ to
track those that are created after the first call, and enter ‘a’ (all)
to track both sets of molecules. For *report*, enter ‘e’ to report on
every molecule that is being tracked, or ‘r’ to report on only those
that reacted since the command was last called. In this latter case, the
position that is used for a reacted molecule is its most recently
tracked position, since it no longer exists. For example, if you want to
see how far molecules diffuse between their creation in one reaction and
their destruction in another reaction, set *start* to ‘c’ and *report*
to ‘r’. Or, set *start* to ‘i’ and *report* to ‘e’ for this function to
be identical to meansqrdisp. It can track up to *max_mol* molecules.
This function prints out the time and then results for all moments, even
and odd, from 〈*r*<sup>0</sup>〉 (the number of molecules being reported
on) to 〈*r<sup>max_moment</sup>*〉. This command accounts for periodic
boundaries. *state* is optional; neither *species* nor *state* can be
`all`. This command does not work if multiple molecules have the same
serial number (which can only happen if you use the reaction_serialnum
statement) and, for molecules with two-part serial numbers, it
determines molecule identity based on only the right part.

```
meansqrdisp3 *species*(*state*) *dim start report max_mol change* *filename command*
```

This function is quite similar to meansqrdisp and meansqrdisp2. It
measures mean square displacements of molecules of type *species*, along
dimension *dim* (‘x’, ‘y’, or ‘z’, or 0, 1, or 2). Then, this function
divides these values by the molecules’ ages to compute effective
diffusion coefficients and prints the results to *filename*. The
effective diffusion coefficient average is weighted using the molecule
ages, so that old molecules have proportionately greater weight in the
average than young molecules. The *start* and *report* arguments control
when this command starts tracking molecules and when it reports their
mean square displacements, respectively. For *start*, enter ‘i’ to track
molecules that exist when the command is initially invoked, enter ‘c’ to
track those that are created after the first call, and enter ‘a’ (all)
to track both sets of molecules. For *report*, enter ‘e’ to report on
every molecule that is being tracked, or ‘r’ to report on only those
that reacted since the command was last called. In this latter case, the
position that is used for a reacted molecule is its most recently
tracked position, since it no longer exists. It can track up to
*max_mol* molecules. This function prints out the time, the number of
molecules reported on, and the effective diffusion coefficient. Note
that the first command run will always output an effective diffusion
coefficient of NaN (or \#1.IND on Windows systems) due to a 0/0 division
error arising from 0 displacement divided by 0 time difference. If the
effective diffusion coefficient changed less than *change* since the
last time this function was executed, then the command *command* is run
(e.g. if *change* is 0.01 then a fractional diffusion coefficient change
of 1% or less will cause *command* to be run). See the example file
called meansqrdisp3.txt. This command accounts for periodic boundaries.
The *species* cannot be `all`. The state is optional. If the state is
`all`, then molecules for all states of this species must be stored in
the same molecule list. No warning is issued if this is not the case,
but some molecules simply won’t be counted. This command does not work
if multiple molecules have the same serial number (which can only happen
if you use the reaction_serialnum statement) and, for molecules with
two-part serial numbers, it determines molecule identity based on only
the right part.

```
residencetime *species*(*state*) *start report summary_out list_out max_mol filename*
```

This function computes residence times of individual molecules of type
*species*, thus showing how long they have existed in the system. As
with meansqrdisp2 and meansqrdisp3, the *start* and *report* arguments
control when this command starts tracking molecules and when it reports
their residence times, respectively. For *start*, enter ‘i’ to track
molecules that exist when the command is initially invoked, enter ‘c’ to
track those that are created after the first call, and enter ‘a’ (all)
to track both sets of molecules. For *report*, enter ‘e’ to report on
every molecule that is being tracked, or ‘r’ to report on only those
that reacted since the command was last called. It can track up to
*max_mol* molecules. This function needs to be invoked at every time
step so that molecules can be tracked accurately. However, output may
not be wanted at every time step, so set *summary_out* to the number of
invocations between when this should print a summary output, with the
time number of molecules, and mean residence time, and set *list_out*
to the number of invocations between when this should print a list
output, with a list of all molecules with their IDs and their current
ages. In both cases, set values to 0 or less to not have this type of
output. *state* is optional; neither *species* nor *state* can be `all`.
See the residencetime.txt example file. This command does not work if
multiple molecules have the same serial number (which can only happen if
you use the reaction_serialnum statement).

```
diagnostics *type*
```

Displays diagnostics about various parts of the data structures to the
screen. These are identical to those that are displayed at program
initialization, but for later times. The options for the type word are:
`simulation`, `wall`, `molecule`, `surface`, `command`, `box`,
`reaction`, `compartment`, `port`, `check`, and `all`.

```
executiontime *filename*
```

Prints a single line of text with the current simulation time and the
number of seconds of real time that the simulation has taken to execute
since the simulation was started.

```
printLattice *filename*
```

Displays diagnostics about all lattices.

```
writeVTK *filepath/filename*
```

Outputs VTK format data for viewing with applications such as Visit or
Paraview. This creates a stack of files in the working directory, or
somewhere else depending on the *filepath*, for which the names start
with *filename*Lattice00_00001.vtu and *filename*Molecules00001.vtu,
where *filename* is the entered file name. The *filepath* directory
needs to have been created beforehand. In contrast to most filenames,
this path and name should not be declared with the `output_files`
statement. The filename numbers are incremented for each snapshot. If
molecules have two-part serial numbers, this only considers the right
part.

## **System manipulation commands**

```
set *statement*
```

This command lets you use essentially any statement that can be entered
in a configuration file. The statement can, for example, create new
reactions, add surfaces, change rate constants, etc. It has not been
fully debugged.

```
pointsource *species num pos<sub>x</sub>*

pointsource *species num pos<sub>x</sub> pos<sub>y</sub>*

pointsource *species num pos<sub>x</sub> pos<sub>y</sub> pos<sub>z</sub>*
```

Creates *num* new molecules of type *species* and at location *pos*.
Molecule states are set to solution. This creates exactly *num*
molecules if *num* is an integer and a Poisson-distributed random number
of molecules with expected value equal to *num* if *num* is a
non-integer.

```
volumesource *species num pos<sub>x,low</sub> pos<sub>x,high</sub>*

volumesource *species num pos<sub>x,low</sub> pos<sub>x,high</sub> pos<sub>y,low</sub> pos<sub>y,high</sub>*

volumesource *species num pos<sub>x,low</sub> pos<sub>x,high</sub> pos<sub>y,low</sub> pos<sub>y,high</sub> pos<sub>z.low</sub> pos<sub>z,high</sub>*
```

Creates *num* new molecules of type *species* and within the location
bounded by *pos<sub>low</sub>* and *pos<sub>high</sub>*. Molecule states
are set to solution. This creates exactly *num* molecules if *num* is an
integer and a Poisson-distributed random number of molecules with
expected value equal to *num* if *num* is a non-integer.

```
gaussiansource *species num mean<sub>x</sub> sigma<sub>x</sub>*

gaussiansource *species num mean<sub>x</sub> sigma<sub>x</sub> mean<sub>y</sub> sigma<sub>y</sub>*

gaussiansource *species num mean<sub>x</sub> sigma<sub>x</sub> mean<sub>y</sub> sigma<sub>y</sub> mean<sub>z</sub> sigma<sub>z</sub>*
```

Creates *num* new molecules of type *species* with a Gaussian
distribution centered at mean and with standard deviation sigma on each
axis. Molecules are not placed beyond the system boundaries; any
attempted placements are rejected and sampled again. Molecule states are
set to solution. This creates exactly *num* molecules if *num* is an
integer and a Poisson-distributed random number of molecules with
expected value equal to *num* if *num* is a non-integer.

```
movesurfacemol *species(state) prob
surface*<sub>1</sub>:*panel*<sub>1</sub>
*surface*<sub>2</sub>:*panel*<sub>2</sub> \[*state*<sub>2</sub>\]
```

Moves molecules of type *species* and state *state*, and which are bound
to *panel*<sub>1</sub> of *surface*<sub>1</sub>, to *panel*<sub>2</sub>
of *surface*<sub>2</sub> with probability *prob*. If entered, the new
molecular state becomes *state*<sub>2</sub>, which may be any state
including fsoln or bsoln; otherwise the molecule state is unchanged. The
new molecule location is at a random location on *panel*<sub>2</sub>
(which contrasts the behavior of the jump statement). Either or both of
*panel*<sub>1</sub> and *panel*<sub>2</sub> can be `all` for all panels
of the respective surface. Also, the *species* and/or the *state* may be
`all`.

```
killmol *species*(*state*)
```

Kills all molecules of type *species*. *state* is optional; *species*
and/or *state* may be `all`.

```
killmolprob *species*(*state*) *prob*
```

Kills some molecules of type *species*; the probability of a molecule
being killed is *prob* (between 0 and 1). *state* is optional; *species*
and/or *state* may be `all`. *prob* can be a function of the molecule
location using its *x*, *y*, and *z* values.

```
killmolinsphere *species*(*state*) *surface*
```

Kill all molecules of type *species* that are in any sphere that is a
panel of surface *surface*. If *surface* is `all` then every surface is
scanned. *state* is optional; *species* and/or *state* may be `all`.

killmolincmpt *species*(*state*) *compartment*

Kill all molecules of type *species* that are in compartment
*compartment*. *state* is optional; *species* and/or *state* may be
`all`.

```
killmoloutsidesystem *species*(*state*)
```

Kill all molecules of type *species* that are outside of the system
boundaries. *state* is optional; *species* and/or *state* may be `all`.

```
fixmolcount *species num*
```

Counts the number of solution-phase molecules of type *species*. If this
count is more than *num*, then molecules of this type, chosen at random,
are removed until *num* is reached. Otherwise, molecules are added to
random positions in the system to achieve a total count of *num*. This
function considers the entire system volume.

```
fixmolcountrange *species low_num high_num*
```

Exactly like fixmolcount, except that the molecule count is unchanged if
it is between *low_num* and *high_num*, and is otherwise modified to
bring it to within the range.

```
fixmolcountonsurf *species*(*state*) *num surface*
```

Counts the number of surface-bound molecules of type *species* and state
*state*. If this count is more than *num*, then molecules of this type,
chosen at random, are removed until *num* is reached. Otherwise,
molecules with the proper state are added to random positions on the
surface to achieve a total count of *num*.

```
fixmolcountrangeonsurf *species(state) low_num high_num surface*
```

Exactly like fixmolcountonsurf, except that the molecule count is
unchanged if it is between *low_num* and *high_num*, and is otherwise
modified to bring it to within the range.

```
fixmolcountincmpt *species num compartment*
```

Counts the number of solution-phase molecules of type *species* and in
compartment *compartment*. If this count is more than *num*, then
molecules of this type, chosen at random, are removed until *num* is
reached. Otherwise, molecules are added to random positions in the
compartment to achieve a total count of *num*.

```
fixmolcountrangeincmpt *species low_num high_num compartment*
```

Exactly like fixmolcountincmpt, except that the molecule count is
unchanged if it is between *low_num* and *high_num*, and is otherwise
modified to bring it to within the range.

```
equilmol *species<sub>1</sub>*(*state<sub>1</sub>*) *species<sub>2</sub>*(*state<sub>2</sub>*) *prob*
```

Equilibrate these molecules. All molecules of type *species*<sub>1</sub>
and *species*<sub>2</sub> will be randomly replaced with one of the two
types, where type *species*<sub>2</sub> has probability *prob*.
*state<sub>1</sub>* and *state<sub>2</sub>* are optional; defaults are
`solution`. Neither species nor states may be `all`. *prob* can be a
function of the molecule location using its *x*, *y*, and *z* values.

```
replacemol *species<sub>1</sub>*(*state<sub>1</sub>*) *species<sub>2</sub>*(*state<sub>2</sub>*) *prob*
```

Molecules of type *species<sub>1</sub>* are replaced with ones of type
*species<sub>2</sub>* with probability *prob*. States are optional and
are solution by default; neither species nor states may be `all`. *prob*
can be a function of the molecule location using its *x*, *y*, and *z*
values.

```
replacexyzmol *species*(*state*) *pos*<sub>0</sub> *pos*<sub>1</sub> … *pos<sub>dim</sub>*<sub>–1</sub>
```

If there is a non-diffusing molecule at exactly position *pos*, it is
replaced with one of type *species*. This command stops after one
molecule is found. *state* is optional and may not be `all`; default is
solution.

```
replacevolmol *species<sub>1</sub>*(*state<sub>1</sub>*) *species<sub>2</sub>*(*state<sub>2</sub>*) *frac* *pos*<sub>0,low</sub> *pos*<sub>0,high</sub> *pos*<sub>1,low</sub> *pos*<sub>1,high</sub> …
*pos<sub>dim</sub>*<sub>–1,high</sub>
```

Fraction *frac* molecules of type *species<sub>1</sub>* in the volume
bounded by *pos*<sub>low</sub>, *pos*<sub>high</sub> are replaced with
ones of type *species<sub>2</sub>*. States are optional and are solution
by default; neither species nor states may be `all`. *frac* can be a
function of the molecule location using its *x*, *y*, and *z* values.

```
replacecmptmol *species<sub>1</sub>*(*state<sub>1</sub>*) *species<sub>2</sub>*(*state<sub>2</sub>*) *frac* *compartment*
```

Fraction *frac* molecules of type *species<sub>1</sub>* in the
compartment named *compartment* are replaced with ones of type
*species<sub>2</sub>*. States are optional and are solution by default;
neither species nor states may be `all`. *frac* can be a function of the
molecule location using its *x*, *y*, and *z* values.

```
modulatemol *species<sub>1</sub>*(*state<sub>1</sub>*) *species<sub>2</sub>*(*state<sub>2</sub>*) *freq* *shift*
```

Modulates molecules of types *species*<sub>1</sub> and
*species*<sub>2</sub>, just like equilmol, but with a variable
probability. Every time this command executes, any of the two types of
molecules in the system are replaced with a molecule of type
*species*<sub>1</sub> with probability cos(*freq*\**t*+*shift*), where
*t* is the simulation time, and otherwise with a molecule of type
*species*<sub>2</sub>. States are optional and are solution by default;
neither species nor states may be `all`.

```
react1 *species*(*state*) *rxn*
```

All molecules of type *species* are instantly reacted, resulting in the
products and product placements given by the unimolecular reaction named
*rxn*. Note that *species* does not have to be the normal reactant for
reaction *rxn*. The state is optional; *species* and/or *state* may be
`all`.

```
setrateint *rxn rate*
```

Sets the internal reaction rate of the reaction named *rxn* to *rate*.
See the description above for rate_internal for the meanings of *rate*
for the different reaction orders.

```
shufflemollist *listname*
```

Randomly shuffles the sequence of molecules in the molecule list called
*listname*. Enter `all` for all lists. This is useful for systems that
are especially ordered or for simulations with unusually long time
steps, because these situations may make simulation results depend on
the molecule list sequences.

```
shufflereactions *reactant1 reactant2*
```

Randomly shuffles the sequence of bimolecular reactions that reactant
species reactant1 and reactant2 can undergo (all states are indexed
together). Either or both or reactant1 and reactant2 can be `all`.

Deleted, starting with Smoldyn 2.19:

```
*setsurfcoeff surface_name species state1 state2 rate*
```

*Sets the rate coefficient for the conversion of species from state1 to
state2 at surface surface_name.*

```
settimestep *dt*
```

Changes the simulation time step to *dt*. This changes the diffusion
step lengths, reaction binding and unbinding radii, and surface action
probabilities. Caution should be used if the time step is increased to
longer than the original value because no checks are made to ensure that
the simulation will still yield accurate results.

```
porttransport *port1 port2*
```

Transports molecules from the output buffer of *port1* to the input of
*port2*. These may be the same ports.

```
excludebox *xlo xhi*

excludebox *xlo xhi ylo yhi*

excludebox *xlo xhi ylo yhi zlo zhi*
```

This keeps all molecules from entering a rectanguloid box within the
system volume. Use the first form for one dimension, the second for two
dimensions, and the third for three dimensions. Molecules that start
within the box can stay there, but any molecule that tries to diffuse
into the box is returned to its location at the previous time step. This
command needs to be run at every time step to work properly.

```
excludesphere *x rad*

excludesphere *x y rad*

excludesphere *x y z rad*
```

This keeps all molecules from entering a sphere within the system
volume. Use the first form for one dimension, the second for two
dimensions, and the third for three dimensions; the coordinates given
are the sphere center and *rad* is the sphere radius. Molecules that
start within the sphere can stay there, but any molecule that tries to
diffuse into the sphere is returned to its location at the previous time
step. This command needs to be run at every time step to work properly.

```
includeecoli
```

An *E. coli* shape is defined as a cylinder with hemispherical endcaps,
where the long axis of the bacterium extends the length of the *x*-axis
within the system walls and the radius of both the cylinder and the
endcaps is half the spacing between the walls that bound the *y*-axis.
This command moves any molecule that diffuses out of the *E. coli* shape
back to its location at the previous time step, or to the nearest
surface of the *E. coli* if it was outside at the previous time step as
well. This command does not need to be run at every time step to work
properly. This only works with a 3 dimensional system.

```
setreactionratemolcount *rxn c0 c1 species1*(*state1*) *c2 species2*(*state2*) ...
```

This sets the rate of the reaction named *rxn* to: *c0*, plus *c1* times
the number of molecules of *species1* and *state1*, plus *c2* times the
number of molecules of *species2* and *state2*, plus any additional
coefficients and species that are listed here. Species and/or states may
be `all` and wildcards are permitted. If the reaction rate is computed
to be a negative value, it is set to zero instead (and no warning is
issued).

```
expandsystem *expandx expandy expandz*
```

Expand, or contract, everything in system, which includes molecule
locations and surfaces, about the center of the system. Expands by
*expandx* along the x-coordinate, by *expandy* along the y-coordinate,
and by *expandz* along the z-coordinate. Enter as many numbers as there
are dimensions. Each number should be 1 for no change, a number larger
than 1 for expansion and a number smaller than 1 for contraction.
Negative numbers perform system inversion. This command can be used, for
example, to mimic lengthwise or diameter growth of a cell. *Warning*:
isotropic expansion or contraction, in which all three expansion values
are equal, generally works well, with no unintentional transfer of
molecules across surfaces. However, anisotropic expansion or contraction
would normally cause some of Smoldyn’s panel shapes to become distorted,
including spheres, hemispheres, cylinders, and disks. Smoldyn does not
support this, so these panels are expanded but not distorted. In the
process, molecules often cross the surfaces unintentionally and need to
be dealt with separately (e.g. killed off). See the expandsystem.txt
example file.

```
translatecmpt *compartment code ∆x*

translatecmpt *compartment code ∆x ∆y*

translatecmpt *compartment code ∆x ∆y ∆z*
```

Translate the compartment called *compartment* by the shift value given
in *∆x*, *∆y*, and *∆z* (with fewer values for lower dimensions). The
*code* value describes which attributes of the compartment should be
moved; add the codes for the individual attributes for the final code
value. The individual codes are: 1 for translating the compartment
surfaces, 2 for translating the molecules that are bound to those
surfaces, 4 for translating the molecules that are within the
compartment, and 8 for shifting the molecules that are outside of the
compartment but that get bumped into by the moving surfaces. Thus, use a
code of 15 to get all of these behaviors at once. With code of 4, all
molecules within the compartment are translated, regardless of their
surface actions. With code of 8, all molecules that a compartment
surface would bump into get translated, unless their surface action is
`transmit`. If a molecule gets `squeezed`, meaning that it gets bumped
into by the moving compartment, but then gets bumped back due to some
other surface, then that molecule will end up inside of the compartment.

```
diffusecmpt *compartment code stddev<sub>x</sub>* \[*cmpt_bound radius nsample*\]

diffusecmpt *compartment code stddev<sub>x</sub> stddev<sub>y</sub>* \[*cmpt_bound radius nsample*\]

diffusecmpt *compartment code stddev<sub>x</sub> stddev<sub>y</sub> stddev<sub>z</sub>* \[*cmpt_bound radius nsample*\]
```

This is similar to translatecmpt, except that this translates the
compartment by a Gaussian distributed random amount that has standard
deviation on each axis of *stddev<sub>x</sub>*, *stddev<sub>y</sub>*,
and *stddev<sub>z</sub>*. Also, this has three optional parameters:
*cmpt_bound*, *radius*, and *nsample*. If they are entered, then
Smoldyn keeps the diffusing compartment within the bounding compartment
called *cmpt_bound* (assuming it was inside initially). It does this by
computing *nsample* test points that are on a spherical shell of radius
*radius* around each of the compartment’s interior-defining points, and
determines if any of these test points are not within the bounding
compartment. If a test point is outside, then the diffusing compartment
is moved back away from the edge of the bounding compartment.

## Copyright and Citation

If you use Smoldyn to a significant extent, then I would appreciate
citations to the research papers that describe the program, as
appropriate. These papers are:

<span class="underline">Description of the Smoldyn program</span>

• Andrews, Steven S., Nathan J. Addy, Roger Brent, and Adam P. Arkin
`Detailed simulations of cell biology with Smoldyn 2.1` *PLoS Comp.
Biol.* 6:e1000705, 2010.

<span class="underline">How to use Smoldyn</span>

• Andrews, Steven S. `Smoldyn User’s Manual` http://www.smoldyn.org.

• Andrews, Steven S. `Spatial and stochastic cellular modeling with the
Smoldyn simulator` *Methods for Molecular Biology*, 804:519-542, 2012.

<span class="underline">Description of Smoldyn algorithms</span>

• Andrews, Steven S. and Dennis Bray `Stochastic simulation of chemical
reactions with spatial resolution and single molecule detail` *Phys.
Biol.* 1:137-151, 2004.

• Andrews, Steven S. `Accurate particle-based simulation of adsorption,
desorption, and partial transmission` *Phys. Biol.* 6:046015, 2009.

Nearly all of the core Smoldyn program was written by myself (Steve
Andrews). Exceptions include the following. (1) Smoldyn includes few
short routines copied from *Numerical Recipes in C* (Press, Flannery,
Teukolsky, and Vetterling, Cambridge University Press, 1988), which are
acknowledged where appropriate. (2) Martin Robinson wrote the
NextSubVolume code and parts of the smollattice.c file, which integrates
that code with Smoldyn. (3) The BioNetGen code, which is only integrated
loosely with Smoldyn, was written by Michael Blinov, Jim Faeder, Bill
Hlavacek and several others who are listed in the file
BioNetGen/CREDITS.txt. (4) Some filament code was written by Edward
Rolls. The compiled version of Smoldyn, the components of the source
code that are not copyrighted by others, and this documentation are
copyrighted by myself. It is distributed under the terms of the Lesser
Gnu General Public License (LGPL). No warranty is made for the
performance or suitability of any portion of Smoldyn.

I expect to maintain a working copy of the program indefinitely. The
download site for Smoldyn is http://www.smoldyn.org, where the program
may be obtained for free. If improvements are made to the code or bugs
are fixed, then I would appreciate a copy of the modified source code.
If you find any bugs in the code, please let me know! My e-mail address
is mailto:steven.s.andrews@gmail.com.
