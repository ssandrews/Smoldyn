from ._version import __version__
from .combine import (
    preprocess_sed_task,
    exec_sed_task,
    exec_sed_doc,
    exec_sedml_docs_in_combine_archive,
)  # noqa: F401
import smoldyn

__all__ = [
    "__version__",
    "get_simulator_version",
    "preprocess_sed_task",
    "exec_sed_task",
    "exec_sed_doc",
    "exec_sedml_docs_in_combine_archive",
]


def get_simulator_version():
    """Get the version of Smoldyn

    Returns:
        :obj:`str`: version
    """
    return smoldyn.__version__
