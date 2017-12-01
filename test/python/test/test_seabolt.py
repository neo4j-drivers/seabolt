#!/usr/bin/env python
# coding: utf-8


from unittest import TestCase, main

from seabolt import *


class SeaboltTestCase(TestCase):

    def test_null(self):
        value = BoltValue()
        value.dump()

    def test_int32(self):
        value = BoltValue().as_int32(23)
        value.dump()


if __name__ == "__main__":
    main()
