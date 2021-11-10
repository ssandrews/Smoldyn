import smoldyn
from pathlib import Path

filename = (
    Path(__file__).parent / ".." / ".." / "examples" / "CMakeLists.txt"
)  # a file that is not valid Smoldyn configuration file

smoldyn_simulation = smoldyn.Simulation.fromFile(filename)
if not smoldyn_simulation.getSimPtr():
    error_code, error_msg = smoldyn.getError()
    msg = "Model source `{}` is not a valid Smoldyn file.\n\n  {}: {}".format(
        filename,
        error_code.name[0].upper() + error_code.name[1:],
        error_msg.replace("\n", "\n  "),
    )
    raise ValueError(msg)
