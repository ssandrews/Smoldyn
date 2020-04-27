"""Install script.
"""

__author__     = "Dilawar Singh"
__copyright__  = "Copyright 2019-, Dilawar Singh"
__maintainer__ = "Dilawar Singh"
__email__      = "dilawar.s.rajput@gmail.com"

import os
import setuptools

version_ = '2.6.12'

setuptools.setup(
    name="smoldyn",
    version=version_,
    description= 'Python scripting interface of Smoldyn simulator',
    long_description_content_type='text/markdown',
    author='Dilawar Singh',  # author of this python package.
    author_email='dilawar.s.rajput@gmail.com',
    url='http://www.smoldyn.org/',
    #  include_package_data=True,
    packages=setuptools.find_packages(),
    package_data={"smoldyn": [ "_smoldyn.so" ]},
    install_requires=["numpy"],
)
