#!/usr/bin/env python
# coding: utf-8


from ctypes import *
from os.path import dirname, join as path_join

_seabolt = CDLL(path_join(dirname(__file__), "..", "build", "lib", "libseabolt.so"))
