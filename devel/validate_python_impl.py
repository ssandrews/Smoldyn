#!/usr/bin/env python3

__author__ = "Dilawar Singh"
__email__ = "dilawars@ncbs.res.in"

import pathlib
import subprocess
import argparse
import sys
import os
import difflib

args_ = None

env_ = os.environ
env_["SMOLDYN_NON_INTERACTIVE"] = "1"


def executeAndCompare(pyfile, txtfile):
    global args_, env_
    print(f"\t Executing {args_.smoldyn} {txtfile} -tw")
    txtoutput = subprocess.check_output([args_.smoldyn, txtfile], env=env_)
    txtoutput = txtoutput.decode("utf8")
    cmd = f"{sys.executable} {pyfile}"
    print(f"\t Executing {cmd}")
    pyoutput = subprocess.check_output(cmd.split(" "), env=env_)
    pyoutput = pyoutput.decode("utf8")
    s = difflib.SequenceMatcher(None, txtoutput, pyoutput)
    if s.ratio() < 0.8:
        print(txtoutput)
        print(pyoutput)
        raise RuntimeError(f"{pyfile} and {txtfile} does not match")
    return True


def main():
    global args_
    pyfiles = pathlib.Path(args_.DIR).glob("**/*.py")
    for pyfile in pyfiles:
        txtfile = pyfile.with_suffix(".txt")
        if txtfile.exists():
            print(f"Testing {pyfile} and {txtfile}")
            executeAndCompare(pyfile, txtfile)


parser = argparse.ArgumentParser(description="Compare Python and smoldyn")
parser.add_argument(
    "-s",
    "--smoldyn",
    type=str,
    default="/usr/bin/smoldyn",
    help="Smoldyn executable path",
)
parser.add_argument("DIR", help="Directory to search the smoldyn files")
args_ = parser.parse_args()
main()
