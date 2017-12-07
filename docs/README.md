# Seabolt Docs

Seabolt documentation is built using a combination of Sphinx, Breathe and Doxygen.
These can be installed as follows:

```
$ sudo apt install doxygen
```

```
$ pip install --user sphinx breathe
```


## Building

To run both doxygen and sphinx in turn, simply use the `Makefile` in the `docs` directory.
```
$ make -C docs html
```

The generated HTML output will appear in `docs/build/html`.
