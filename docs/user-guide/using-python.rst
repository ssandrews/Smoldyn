.. warning:: This is a work in progress.

:Authors:
    Dilawar Singh <dilawars@ncbs.res.in>

Installing Python module
**********************

Use ``pip`` to install Smoldyn.

.. code-block:: 

    $ pip install smoldyn --user

If this methods fails for some reason, you can also build the Python module.
Refer to `Compiling Smoldyn`_.

Using Python module
*******************

If you are already familiar with Smoldyn then I recommend that you compare
Smoldyn's plain text model with equivalent Python model in ``examples``
directory. Not all models are translated to Python but there are enough to get
you started.

.. code-block:: python

    >>> import smoldyn
    >>> print(smoldyn.__version)
    >>> '2.62.dev'


Here is an example which shows plain text and Python module side by side

.. literalinclude:: /../examples/S10_boxes/box.txt

.. literalinclude:: /../examples/S10_boxes/box.py
    :language: python
