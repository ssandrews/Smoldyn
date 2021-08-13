from ._version import __version__

import smoldyn


def get_simulator_version():
    """ Get the version of Smoldyn

    Returns:
        :obj:`str`: version
    """
    return smoldyn.__version__
