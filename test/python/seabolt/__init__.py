#!/usr/bin/env python
# coding: utf-8


from ctypes import *


class _BoltValue(Structure):
    _fields_ = [
        ("type", c_int),
        ("code", c_int),
        ("is_array", c_int),
        ("logical_size", c_long),
        ("physical_size", c_size_t),
        ("data", c_void_p),
    ]


_seabolt = CDLL("../../lib/libseabolt.so")

_seabolt.bolt_create_value.restype = POINTER(_BoltValue)


class BoltValue(object):

    def __init__(self):
        self._ = _seabolt.bolt_create_value()

    def __del__(self):
        _seabolt.bolt_destroy_value(self._)

    def as_int32(self, x):
        _seabolt.bolt_put_int32(self._, x)
        return self

    def dump(self):
        _seabolt.bolt_dump_ln(self._)
