#!/usr/bin/env python
# coding: utf-8


from ctypes import *
from os.path import dirname, join as path_join
from socket import inet_ntop, AF_INET, AF_INET6
from sys import stdout
from unittest import TestCase, main


libc = cdll.LoadLibrary("libc.so.6")
c_stdout = c_void_p.in_dll(libc, "stdout")

_seabolt = CDLL(path_join(dirname(__file__), "..", "build", "lib", "libseabolt.so"))


class _BoltAddress(Structure):

    _fields_ = [
        ("host", c_char_p),
        ("port", c_char_p),
        ("n_resolved_hosts", c_int),
        ("resolved_hosts", c_char_p),
        ("resolved_port", c_int16),
        ("gai_status", c_int),
    ]


_seabolt.BoltAddress_create.restype = POINTER(_BoltAddress)
_seabolt.BoltAddress_resolved_host.restype = c_void_p


class _BoltConnection(Structure):

    _fields_ = [
        ("transport", c_int),
        ("ssl_context", c_void_p),
        ("ssl", c_void_p),
        ("socket", c_int),
        ("protocol_version", c_int32),
        ("protocol_state", c_void_p),
        ("tx_buffer", c_void_p),
        ("rx_buffer", c_void_p),
        ("status", c_int),
        ("error", c_int),
    ]


# _seabolt.BoltConnection_open_b.argtypes = [c_int, POINTER(_BoltAddress)]
_seabolt.BoltConnection_open_b.restype = POINTER(_BoltConnection)


class _BoltValue(Structure):

    _fields_ = [
        ("type", c_int16),
        ("code", c_int16),
        ("size", c_int32),
        ("data_size", c_size_t),
        ("data", c_void_p),
    ]


_seabolt.BoltValue_create.restype = POINTER(_BoltValue)


class BoltAddress(object):

    def __init__(self, host, port):
        self._ = _seabolt.BoltAddress_create(host, port)

    def destroy(self):
        _seabolt.BoltAddress_destroy(self._)

    def resolve_b(self):
        _seabolt.BoltAddress_resolve_b(self._)

    @property
    def host(self):
        return self._.contents.host

    @property
    def port(self):
        return self._.contents.port

    @property
    def resolved_hosts(self):
        for i in range(self._.contents.n_resolved_hosts):
            ipv6 = string_at(_seabolt.BoltAddress_resolved_host(self._, i), 16)
            if _seabolt.BoltAddress_resolved_host_is_ipv4(self._, i):
                yield inet_ntop(AF_INET, ipv6[12:16])
            else:
                yield inet_ntop(AF_INET6, ipv6)

    @property
    def resolved_port(self):
        return self._.contents.resolved_port


class BoltConnection(object):

    def __init__(self, address, secure=True):
        self._ = _seabolt.BoltConnection_open_b(0 if secure else 1, address._)

    def close(self):
        _seabolt.BoltConnection_close_b(self._)

    @property
    def protocol_version(self):
        return self._.contents.protocol_version


class BoltValue(object):

    def __init__(self):
        self._ = _seabolt.BoltValue_create()

    def __del__(self):
        _seabolt.BoltValue_destroy(self._)

    def dump(self):
        _seabolt.BoltValue_write(self._, c_stdout)


class BoltInt8(BoltValue):

    def __init__(self, x):
        super(BoltInt8, self).__init__()
        _seabolt.BoltValue_to_Int8(self._, x)

    def get(self):
        return _seabolt.BoltInt8_get(self._)


class BoltInt16(BoltValue):

    def __init__(self, x):
        super(BoltInt16, self).__init__()
        _seabolt.BoltValue_to_Int16(self._, x)

    def get(self):
        return _seabolt.BoltInt16_get(self._)


class BoltInt32(BoltValue):

    def __init__(self, x):
        super(BoltInt32, self).__init__()
        _seabolt.BoltValue_to_Int32(self._, x)

    def get(self):
        return _seabolt.BoltInt32_get(self._)


class BoltInt32Array(BoltValue):

    def __init__(self, iterable):
        super(BoltInt32Array, self).__init__()
        values = list(iterable)
        count = len(values)
        int32_values = (c_int32 * count)(*values)
        _seabolt.BoltValue_to_Int32Array(self._, pointer(int32_values), count)

    def get(self, index):
        return _seabolt.BoltInt32Array_get(self._, index)


# # # # # # # # # # # # # # # # #
# # # # # # # TESTS # # # # # # #
# # # # # # # # # # # # # # # # #


class SeaboltTestCase(TestCase):

    def test_null(self):
        value = BoltValue()
        value.dump()
        stdout.write("\n")

    def test_int8(self):
        value = BoltInt8(123)
        self.assertEqual(value.get(), 123)
        value.dump()
        stdout.write("\n")

    def test_int16(self):
        value = BoltInt16(12345)
        self.assertEqual(value.get(), 12345)
        value.dump()
        stdout.write("\n")

    def test_int32(self):
        value = BoltInt32(1234567)
        self.assertEqual(value.get(), 1234567)
        value.dump()
        stdout.write("\n")

    def test_int32Array(self):
        value = BoltInt32Array(range(10))
        for i in range(10):
            self.assertEqual(value.get(i), i)
        value.dump()
        stdout.write("\n")


if __name__ == "__main__":
    #main()
    addr = BoltAddress("localhost", "7687")
    addr.resolve_b()
    print(list(addr.resolved_hosts))
    print(addr.resolved_port)
    conn = BoltConnection(addr)
    print(conn.protocol_version)
    conn.close()
    addr.destroy()
