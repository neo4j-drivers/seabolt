/*
 * Copyright (c) 2002-2017 "Neo Technology,"
 * Network Engine for Objects in Lund AB [http://neotechnology.com]
 *
 * This file is part of Neo4j.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SEABOLT_TEST_MEM
#define SEABOLT_TEST_MEM

#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include "warden.h"


void test_memcpyr()
{
    const char* data = "ABCD";
    size_t size = strlen(data);
    char digits[size];
    memcpy(&digits[0], data, size);
    assert(digits[0] == 'A');
    assert(digits[1] == 'B');
    assert(digits[2] == 'C');
    assert(digits[3] == 'D');
    memcpyr(&digits[0], data, size);
    assert(digits[0] == 'D');
    assert(digits[1] == 'C');
    assert(digits[2] == 'B');
    assert(digits[3] == 'A');

    int16_t sh;
    int16_t* sh_p = &sh;
    const char* sh_digits = "\x0A\x0B";
    memcpy_be(sh_p, &sh_digits[0], 2);
    assert(sh == 0x0B0A);
}

#endif // SEABOLT_TEST_MEM
