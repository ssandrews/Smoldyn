import smoldyn


def get_simulator_version():
    """ Get the version of Smoldyn

    Returns:
        :obj:`str`: version
    """
    return smoldyn.__version__
