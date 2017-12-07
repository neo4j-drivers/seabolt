# Seabolt Docs

Seabolt documentation is built using a combination of [Sphinx](http://www.sphinx-doc.org/), [Breathe](https://breathe.readthedocs.io/) and [Doxygen](http://www.doxygen.org/).
These can be installed as follows:

```
$ sudo apt install doxygen
```

```
$ pip install --user sphinx breathe
```


## Building

To run both _doxygen_ and _sphinx_ in turn, simply use the `Makefile` in the `docs` directory.
```
$ make -C docs html
```

The generated HTML output will appear in `build/html` within the `docs` directory.
