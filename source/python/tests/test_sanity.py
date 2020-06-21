"""test_sanity.py: 

"""
    
__author__           = "Dilawar Singh"
__copyright__        = "Copyright 2016, Dilawar Singh"
__credits__          = ["NCBS Bangalore"]
__license__          = "GNU GPL"
__version__          = "1.0.0"
__maintainer__       = "Dilawar Singh"
__email__            = "dilawars@ncbs.res.in"
__status__           = "Development"

from smoldyn import _smoldyn

def main():
    s = _smoldyn.newSim(3, [0, 0, 0], [10,10,10])
    print(dir(s))

if __name__ == "__main__":
    main()
