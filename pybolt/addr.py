#!/usr/bin/env python
# coding: utf-8


from ctypes import *
from .imp import _seabolt
from socket import inet_ntop, AF_INET, AF_INET6


class _BoltAddress(Structure):

    _fields_ = [
        ("host", c_char_p),
        ("port", c_char_p),
        ("n_resolved_hosts", c_int),
        ("resolved_hosts", c_char_p),
        ("resolved_port", c_int16),
        ("gai_status", c_int),
    ]


_seabolt.BoltAddress_create.argtypes = [c_char_p, c_char_p]
_seabolt.BoltAddress_create.restype = POINTER(_BoltAddress)


def c_str(s):
    if isinstance(s, bytes):
        return c_char_p(s)
    elif isinstance(s, bytearray):
        return c_char_p(bytes(s))
    elif isinstance(s, str):
        return c_char_p(s.encode("utf-8"))
    else:
        return c_str(str(s))


class BoltAddress(object):

    __host = None
    __port = None

    def __init__(self, host, port):
        self.__host = c_str(host)
        self.__port = c_str(port)
        self._ = _seabolt.BoltAddress_create(self.__host, self.__port)

    def __del__(self):
        _seabolt.BoltAddress_destroy(self._)

    @property
    def host(self):
        return self._.contents.host

    @property
    def port(self):
        return self._.contents.port

    def resolve_b(self):
        status = _seabolt.BoltAddress_resolve_b(self._)
        if status != 0:
            raise OSError("Cannot resolve host %r for port %r", self.__host, self.__port)
        host_buffer = create_string_buffer(40)
        port = self._.contents.resolved_port
        for i in range(self._.contents.n_resolved_hosts):
            _seabolt.BoltAddress_copy_resolved_host(self._, i, host_buffer, 40)
            yield host_buffer.value, port
