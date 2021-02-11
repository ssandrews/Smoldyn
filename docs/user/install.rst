Using pip
=========

We provide easy to install `Python wheels
<https://wheel.readthedocs.io/en/latest/>`_ for most update to date operating
systems such as Linux, Windows and OSX.

In most cases, following should just work


.. code block:: bash

    $ python3 -m pip install smoldyn


Building Smoldyn 
=================

Clone the source code
--------------------

Run the following command in terminal. Optionally you can pass `--depth 10`
to limit the download history (it is recommended).

.. code-block:: bash
    
    git clone https://github.com/ssandrews/Smoldyn --depth 10

Install dependencies 
-------------------

Debian
""""""

.. code-block:: bash

    sudo apt install cmake freeglut3-dev libtiff5-dev
