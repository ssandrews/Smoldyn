Installing Smoldyn
*******************

We provide easy to install [Python
wheels](https://wheel.readthedocs.io/en/latest/) for most update to date
operating systems such as Linux, Windows and OSX.

In most cases, following should just work


```
$ python3 -m pip install smoldyn
```


Building Smoldyn from the source
================================

Clone the source code
--------------------

Run the following command in terminal. Optionally you can pass `--depth 10`
to limit the download history (it is recommended).

```
$ git clone https://github.com/ssandrews/Smoldyn --depth 10
```

Install dependencies 
-------------------

Debian
""""""

```
$ sudo apt install cmake freeglut3-dev libtiff5-dev
```


