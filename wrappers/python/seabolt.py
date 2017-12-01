#!/usr/bin/env python
# coding: utf-8


from ctypes import *
from os.path import dirname, join as path_join
from unittest import TestCase, main


_seabolt = CDLL(path_join(dirname(__file__), "..", "..", "lib", "libseabolt.so"))


class _BoltValue(Structure):

    _fields_ = [
        ("type", c_int),
        ("code", c_int),
        ("is_array", c_int),
        ("logical_size", c_long),
        ("physical_size", c_size_t),
        ("data", c_void_p),
    ]


_seabolt.bolt_create_value.restype = POINTER(_BoltValue)


class BoltValue(object):

    def __init__(self):
        self._ = _seabolt.bolt_create_value()

    def __del__(self):
        _seabolt.bolt_destroy_value(self._)

    def dump(self):
        _seabolt.bolt_dump_ln(self._)


class BoltInt8(BoltValue):

    def __init__(self, x):
        super(BoltInt8, self).__init__()
        _seabolt.bolt_put_int8(self._, x)

    def __int__(self):
        return _seabolt.bolt_get_int8(self._)


class BoltInt16(BoltValue):

    def __init__(self, x):
        super(BoltInt16, self).__init__()
        _seabolt.bolt_put_int16(self._, x)

    def __int__(self):
        return _seabolt.bolt_get_int16(self._)


class BoltInt32(BoltValue):

    def __init__(self, x):
        super(BoltInt32, self).__init__()
        _seabolt.bolt_put_int32(self._, x)

    def __int__(self):
        return _seabolt.bolt_get_int32(self._)


# # # # # # # # # # # # # # # # #
# # # # # # # TESTS # # # # # # #
# # # # # # # # # # # # # # # # #


class SeaboltTestCase(TestCase):

    def test_null(self):
        value = BoltValue()
        value.dump()

    def test_int8(self):
        value = BoltInt8(123)
        self.assertEqual(int(value), 123)
        value.dump()

    def test_int16(self):
        value = BoltInt16(12345)
        self.assertEqual(int(value), 12345)
        value.dump()

    def test_int32(self):
        value = BoltInt32(1234567)
        self.assertEqual(int(value), 1234567)
        value.dump()


if __name__ == "__main__":
    main()
