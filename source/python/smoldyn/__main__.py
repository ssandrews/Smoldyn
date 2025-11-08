# -*- coding: utf-8 -*-

__author__ = "Dilawar Singh"
__email__ = "dilawar.s.rajput@gmail.com"

import smoldyn._smoldyn  # type: ignore
import argparse


def main() -> None:
    # Argument parser.
    description = """Command-line interface to Smoldyn."""
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("input", help="Input file")
    parser.add_argument(
        "--overwrite", "-w", action="store_true", help="Overwrite exsiting data file"
    )
    parser.add_argument("--quit-at-end", "-q", action="store_true", help="Quit at end")
    parser.add_argument(
        "--args", "-A", default="", type=str, help="smoldyn command line arguments"
    )
    args = parser.parse_args()
    smoldyn._smoldyn.loadModel(args.input, args.args, args.overwrite, args.quit_at_end)


if __name__ == "__main__":
    main()
