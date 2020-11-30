# -*- coding: utf-8 -*-
"""Run script.
"""

__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2019-, Dilawar Singh"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"

import smoldyn._smoldyn

import argparse
# Argument parser.
description = '''Python interface to Smoldyn.'''
parser = argparse.ArgumentParser(description=description)
parser.add_argument('input', help = 'Input file')
parser.add_argument('--overwrite', '-w', action='store_true', help="Overwrite exsiting data file")
parser.add_argument('--quit-at-end', '-q', action='store_true', help="Quit at end")
parser.add_argument('--args', '-A', default='', type=str, help="smoldyn command line arguments")
args = parser.parse_args()
smoldyn._smoldyn.loadModel(args.input, args.args, args.overwrite, args.quit_at_end)
