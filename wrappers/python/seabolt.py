#!/usr/bin/env python
# coding: utf-8


from ctypes import *
from os.path import dirname, join as path_join
from unittest import TestCase, main


_seabolt = CDLL(path_join(dirname(__file__), "..", "..", "lib", "libseabolt.so"))


class _BoltValue(Structure):

    _fields_ = [
        ("type", c_char),
        ("is_array", c_char),
        ("code", c_int16),
        ("size", c_int32),
        ("data_size", c_size_t),
        ("data", c_void_p),
    ]


_seabolt.BoltValue_create.restype = POINTER(_BoltValue)


class BoltValue(object):

    def __init__(self):
        self._ = _seabolt.BoltValue_create()

    def __del__(self):
        _seabolt.BoltValue_destroy(self._)

    def dump(self):
        _seabolt.BoltValue_dumpLine(self._)


class BoltInt8(BoltValue):

    def __init__(self, x):
        super(BoltInt8, self).__init__()
        _seabolt.BoltValue_toInt8(self._, x)

    def get(self):
        return _seabolt.BoltInt8_get(self._)


class BoltInt16(BoltValue):

    def __init__(self, x):
        super(BoltInt16, self).__init__()
        _seabolt.BoltValue_toInt16(self._, x)

    def get(self):
        return _seabolt.BoltInt16_get(self._)


class BoltInt32(BoltValue):

    def __init__(self, x):
        super(BoltInt32, self).__init__()
        _seabolt.BoltValue_toInt32(self._, x)

    def get(self):
        return _seabolt.BoltInt32_get(self._)


class BoltInt32Array(BoltValue):

    def __init__(self, iterable):
        super(BoltInt32Array, self).__init__()
        values = list(iterable)
        count = len(values)
        int32_values = (c_int32 * count)(*values)
        _seabolt.BoltValue_toInt32Array(self._, pointer(int32_values), count)

    def get(self, index):
        return _seabolt.BoltInt32Array_get(self._, index)


# # # # # # # # # # # # # # # # #
# # # # # # # TESTS # # # # # # #
# # # # # # # # # # # # # # # # #


class SeaboltTestCase(TestCase):

    def test_null(self):
        value = BoltValue()
        value.dump()

    def test_int8(self):
        value = BoltInt8(123)
        self.assertEqual(value.get(), 123)
        value.dump()

    def test_int16(self):
        value = BoltInt16(12345)
        self.assertEqual(value.get(), 12345)
        value.dump()

    def test_int32(self):
        value = BoltInt32(1234567)
        self.assertEqual(value.get(), 1234567)
        value.dump()

    def test_int32Array(self):
        value = BoltInt32Array(range(10))
        for i in range(10):
            self.assertEqual(value.get(i), i)
        value.dump()


if __name__ == "__main__":
    main()
