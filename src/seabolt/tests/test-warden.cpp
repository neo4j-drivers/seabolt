/*
 * Copyright (c) 2002-2019 "Neo4j,"
 * Neo4j Sweden AB [http://neo4j.com]
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

#include "catch.hpp"
#include "integration.hpp"

extern "C" {
#include "bolt/mem.h"
}

SCENARIO("Test BoltMem_reverse_copy against memcpy")
{
    GIVEN("a block of memory") {
        const char* data = "ABCD";
        size_t size = strlen(data);
        WHEN("regular memcpy is used") {
            char* digits = new char[size];
            memcpy(&digits[0], data, size);
            THEN("the data should be copied in order") {
                REQUIRE(digits[0]=='A');
                REQUIRE(digits[1]=='B');
                REQUIRE(digits[2]=='C');
                REQUIRE(digits[3]=='D');
            }
            delete[] digits;
        }
        WHEN("reverse memcpy is used") {
            char* digits = new char[size];
            BoltMem_reverse_copy(&digits[0], data, size);
            THEN("the data should be copied in reverse order") {
                REQUIRE(digits[0]=='D');
                REQUIRE(digits[1]=='C');
                REQUIRE(digits[2]=='B');
                REQUIRE(digits[3]=='A');
            }
            delete[] digits;
        }
    }
}

SCENARIO("Test memcpy_be macro")
{
    GIVEN("a byte pair") {
        const char* sh_digits = "\xAB\xCD";
        WHEN("the data is copied into an unsigned 16-bit integer in big-endian order") {
            uint16_t sh;
            uint16_t* sh_p = &sh;
            memcpy_be(sh_p, &sh_digits[0], 2);
            THEN("the first byte should be most significant and the second byte least significant") {
                REQUIRE(sh==0xABCD);
            }
        }
    }
}
