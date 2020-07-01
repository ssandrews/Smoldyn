.. note:: 
    It is a rewrite of `Getting Started` chapter with a focus on Python users.

.. note::
   Smoldyn supports Python version 3.6 or higher. 

Installing Python module
========================

Use ``pip`` to install Smoldyn. Visit
`<https://pip.pypa.io/en/stable/installing/>`_  for how to install and use
``pip``.

.. code-block::

    $ python3 -m pip install smoldyn --user

After successful installation, you can verify the installation by running the
following command in the terminal.

.. program-output:: python3 -c "import smoldyn; print(smoldyn.__version__)"

If the installation fails or your platform is not supported, you can build the
Python module by yourself.  Refer to the section `Compiling Smoldyn`_. 

.. todo:: Add Python build instructions to the docs.

A quick Comparison 
==================

If you are familiar with the Smoldyn text based configuration files then I
recommend that you compare existing Smoldyn's plain text model with its
equivalent Python script in the ``examples`` directory.

Here is an example which shows plain text and Python module side by side.
In this example, the code size is reduced by half and you get stronger
error checks and whole ecosystem of Python at your finger tips.

.. list-table::
   :align: left
   :width: "600px"
   :header-rows: 1

   * - Plain Text Model
     - Python Model
   * - .. literalinclude:: /../examples/S10_boxes/box.txt
            :linenos:
     - .. literalinclude:: /../examples/S10_boxes/box.py
            :linenos:
            :language: python
