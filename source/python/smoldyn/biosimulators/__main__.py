"""BioSimulators-compliant command-line interface"""

__author__ = "Jonathan Karr"
__email__ = "karr@mssm.edu"

__all__ = [
    "App",
    "main",
]

from . import __version__, get_simulator_version
from .combine import exec_sedml_docs_in_combine_archive
from biosimulators_utils.simulator.cli import build_cli

App = build_cli(
    "smoldyn",
    __version__,
    "Smoldyn",
    get_simulator_version(),
    "http://www.smoldyn.org",
    exec_sedml_docs_in_combine_archive,
)


def main():
    with App() as app:
        app.run()


if __name__ == "__main__":
    main()
