============
Installation
============

Using pip
=========

We provide easy to install `Python wheels
<https://wheel.readthedocs.io/en/latest/>`_ for most update to date operating
systems such as Linux, Windows and OSX (10.14 onwards).

In most cases, following should just work


.. code-block:: bash

    python3 -m pip install smoldyn

``smoldyn`` binary can be mimicked by this module. In a terminal, ``python3 -m smoldyn`` is
roughly equivalent to the ``smoldyn`` command.

.. code-block:: bash

   python3 -m smoldyn template.txt

In cases where `pip` can't find a wheel for your distribution, you may have to build it
yourself.

Building Smoldyn
=================

Clone the source code
----------------------

Run the following command in terminal. Optionally you can pass `--depth 10`
to limit the download history (it is recommended).

.. code-block:: bash

    git clone https://github.com/ssandrews/Smoldyn --depth 10

Install dependencies
---------------------

Most dependencies are in source tree except for ``freeglut`` and ``libtiff``.
It is possible to build Smoldyn without them. These dependencies are easily
available from your package manager.

You will also need ``cmake`` (version 3.14+) and a recent C++ compiler such as
``gcc``, ``clang``, or Microsoft Visual Studio.

On Debian/Ubuntu  system, these dependencies can be installed using `apt`
package manager.

Debian
""""""

.. code-block:: bash

    sudo apt install freeglut3-dev libtiff5-dev


Smoldyn in headless environments
=================================

Smoldyn can be run in headless environments (e.g., clusters, CI) using xvfb.

Installation
------------

.. code-block:: Dockerfile

    RUN apt-get -y update \
        && apt-get install --no-install-recommends -y \
            xvfb \
        && rm -rf /var/lib/apt/lists/* \
        && mkdir /tmp/.X11-unix \
        && chmod 1777 /tmp/.X11-unix
    COPY xvfb-startup.sh .
    RUN chmod +x xvfb-startup.sh
    ARG RESOLUTION="1920x1080x24"
    ARG XARGS=""
    ENV XVFB_RES="${RESOLUTION}" \
        XVFB_ARGS="${XARGS}"

xvfb script for installation
----------------------------

Download `xvfb-startup.sh <https://raw.githubusercontent.com/ssandrews/Smoldyn/master/scripts/xvfb-startup.sh>`_.

Execution
----------

.. code-block:: bash

    /bin/bash ./xvfb-startup.sh python -m smoldyn
