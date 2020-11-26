This directory contains documentation in rst format. 

The document is generated using `sphinx` and `doxygen`. If you are a developer
then see `CMakeLists.txt` file which is responsible for generating the
documents. However, on Read The Docs service, `conf.py` is used to generate the
docs. 

# On `Read The Docs`

To test the read-the-docs will be able to generate the document, try the
following commands.

```bash
$ sudo apt install doxygen          # if doxygen is not installed.
$ cd docs   # this directory.
$ python3 -m pip install -r requirements.txt
$ sphinx-build . html
```

Then open the html/index.html file in browser.

# Using cmake 

From the build directory

```
$ cmake -DOPTION_DOCS=ON /path/to/main/CMakeList.txt
$ make                      # build smoldyn
$ make html
```
