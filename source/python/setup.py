"""Install script.
"""

__author__     = "Dilawar Singh"
__copyright__  = "Copyright 2019-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__      = "dilawar.s.rajput@gmail.com"

import os
from distutils.core import setup

sdir_ = os.path.dirname(os.path.realpath(__file__))
with open(os.path.join(sdir_, "../../README.md")) as f:
    readme = f.read()

version_ = '2.6.12'

setup(
    name="smoldyn",
    version=version_,
    description= 'Python scripting interface of Smoldyn simulator',
    long_description=readme,
    long_description_content_type='text/markdown',
    author='Dilawar Singh',  # author of this python package.
    author_email='dilawar.s.rajput@gmail.com',
    url='http://www.smoldyn.org/',
    packages=['smoldyn'],
    package_dir = { 'smoldyn' : '.' },
    package_data={ 'smoldyn': [ 'smoldyn.so' ] },
)
