# -*- coding: utf-8 -*-
"""Run script.
"""

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

import smoldyn

import argparse
# Argument parser.
description = '''Runs smoldyn.'''
parser = argparse.ArgumentParser(description=description)
parser.add_argument('input', help = 'Input file')
args_ = parser.parse_args()
smoldyn.load_model(args_.input, "")
