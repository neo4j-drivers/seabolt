/*
 * Copyright (c) 2002-2018 "Neo4j,"
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

TEST_CASE("StringBuilder")
{
    const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";
    const int charset_size = strlen(charset);

    StringBuilder* builder = StringBuilder_create();

    REQUIRE(builder!=nullptr);
    REQUIRE(StringBuilder_get_length(builder)==0);
    REQUIRE(StringBuilder_get_string(builder)!=nullptr);
    REQUIRE(StringBuilder_get_string(builder)[0]==0);

    SECTION("StringBuilder_append") {
        SECTION("null") {
            StringBuilder_append(builder, nullptr);

            REQUIRE(StringBuilder_get_length(builder)==0);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]==0);
        }

        SECTION("empty string") {
            StringBuilder_append(builder, "");

            REQUIRE(StringBuilder_get_length(builder)==0);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]==0);
        }

        SECTION("abcd") {
            StringBuilder_append(builder, "abcd");

            REQUIRE(StringBuilder_get_length(builder)==4);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]=='a');
            REQUIRE(StringBuilder_get_string(builder)[1]=='b');
            REQUIRE(StringBuilder_get_string(builder)[2]=='c');
            REQUIRE(StringBuilder_get_string(builder)[3]=='d');
            REQUIRE(StringBuilder_get_string(builder)[4]==0);

            SECTION("and whitespace") {
                StringBuilder_append(builder, " ");

                REQUIRE(StringBuilder_get_length(builder)==5);
                REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                REQUIRE(StringBuilder_get_string(builder)[4]==' ');
                REQUIRE(StringBuilder_get_string(builder)[5]==0);

                SECTION("and efg") {
                    StringBuilder_append(builder, "efg");

                    REQUIRE(StringBuilder_get_length(builder)==8);
                    REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                    REQUIRE(StringBuilder_get_string(builder)[5]=='e');
                    REQUIRE(StringBuilder_get_string(builder)[6]=='f');
                    REQUIRE(StringBuilder_get_string(builder)[7]=='g');
                    REQUIRE(StringBuilder_get_string(builder)[8]==0);
                }
            }
        }

        SECTION("long string") {
            const int size = 128000;
            char buffer[size+1];
            for (int i = 0; i<size; i++) {
                buffer[i] = charset[rand()%(charset_size-1)];
            }
            buffer[size] = 0;

            StringBuilder_append(builder, buffer);

            REQUIRE(StringBuilder_get_length(builder)==size);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            for (int i = 0; i<size+1; i++) {
                REQUIRE(StringBuilder_get_string(builder)[i]==buffer[i]);
            }
            REQUIRE(StringBuilder_get_string(builder)[size]==0);

            SECTION("and append a number") {
                StringBuilder_append(builder, "1");

                REQUIRE(StringBuilder_get_length(builder)==size+1);
                REQUIRE(StringBuilder_get_string(builder)[size]=='1');
                REQUIRE(StringBuilder_get_string(builder)[size+1]==0);
            }
        }
    }

    SECTION("StringBuilder_append_n") {
        SECTION("null") {
            StringBuilder_append_n(builder, nullptr, 0);

            REQUIRE(StringBuilder_get_length(builder)==0);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]==0);
        }

        SECTION("empty string") {
            StringBuilder_append_n(builder, "", 0);

            REQUIRE(StringBuilder_get_length(builder)==0);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]==0);
        }

        SECTION("abcd") {
            StringBuilder_append_n(builder, "abcd", 4);

            REQUIRE(StringBuilder_get_length(builder)==4);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]=='a');
            REQUIRE(StringBuilder_get_string(builder)[1]=='b');
            REQUIRE(StringBuilder_get_string(builder)[2]=='c');
            REQUIRE(StringBuilder_get_string(builder)[3]=='d');
            REQUIRE(StringBuilder_get_string(builder)[4]==0);

            SECTION("and whitespace") {
                StringBuilder_append_n(builder, " ", 1);

                REQUIRE(StringBuilder_get_length(builder)==5);
                REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                REQUIRE(StringBuilder_get_string(builder)[4]==' ');
                REQUIRE(StringBuilder_get_string(builder)[5]==0);

                SECTION("and efg") {
                    StringBuilder_append_n(builder, "efg", 3);

                    REQUIRE(StringBuilder_get_length(builder)==8);
                    REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                    REQUIRE(StringBuilder_get_string(builder)[5]=='e');
                    REQUIRE(StringBuilder_get_string(builder)[6]=='f');
                    REQUIRE(StringBuilder_get_string(builder)[7]=='g');
                    REQUIRE(StringBuilder_get_string(builder)[8]==0);
                }

                SECTION("and efg from efghij") {
                    StringBuilder_append_n(builder, "efghij", 3);

                    REQUIRE(StringBuilder_get_length(builder)==8);
                    REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                    REQUIRE(StringBuilder_get_string(builder)[5]=='e');
                    REQUIRE(StringBuilder_get_string(builder)[6]=='f');
                    REQUIRE(StringBuilder_get_string(builder)[7]=='g');
                    REQUIRE(StringBuilder_get_string(builder)[8]==0);
                }
            }
        }

        SECTION("long string") {
            const int size = 128000;
            char buffer[size+1];
            for (int i = 0; i<size; i++) {
                buffer[i] = charset[rand()%(charset_size-1)];
            }
            buffer[size] = 0;

            StringBuilder_append_n(builder, buffer, size);

            REQUIRE(StringBuilder_get_length(builder)==size);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            for (int i = 0; i<size+1; i++) {
                REQUIRE(StringBuilder_get_string(builder)[i]==buffer[i]);
            }
            REQUIRE(StringBuilder_get_string(builder)[size]==0);

            SECTION("and append a number") {
                StringBuilder_append_n(builder, "1", 1);

                REQUIRE(StringBuilder_get_length(builder)==size+1);
                REQUIRE(StringBuilder_get_string(builder)[size]=='1');
                REQUIRE(StringBuilder_get_string(builder)[size+1]==0);
            }
        }
    }

    SECTION("StringBuilder_append_f") {
        SECTION("null format") {
            StringBuilder_append_f(builder, nullptr);

            REQUIRE(StringBuilder_get_length(builder)==0);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]==0);
        }

        SECTION("empty format") {
            StringBuilder_append_f(builder, "");

            REQUIRE(StringBuilder_get_length(builder)==0);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]==0);
        }

        SECTION("string format - abcd") {
            StringBuilder_append_f(builder, "%s", "abcd");

            REQUIRE(StringBuilder_get_length(builder)==4);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            REQUIRE(StringBuilder_get_string(builder)[0]=='a');
            REQUIRE(StringBuilder_get_string(builder)[1]=='b');
            REQUIRE(StringBuilder_get_string(builder)[2]=='c');
            REQUIRE(StringBuilder_get_string(builder)[3]=='d');
            REQUIRE(StringBuilder_get_string(builder)[4]==0);

            SECTION("and whitespace") {
                StringBuilder_append_f(builder, "%c", ' ');

                REQUIRE(StringBuilder_get_length(builder)==5);
                REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                REQUIRE(StringBuilder_get_string(builder)[4]==' ');
                REQUIRE(StringBuilder_get_string(builder)[5]==0);

                SECTION("and efg") {
                    StringBuilder_append_f(builder, "%s", "efg");

                    REQUIRE(StringBuilder_get_length(builder)==8);
                    REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                    REQUIRE(StringBuilder_get_string(builder)[5]=='e');
                    REQUIRE(StringBuilder_get_string(builder)[6]=='f');
                    REQUIRE(StringBuilder_get_string(builder)[7]=='g');
                    REQUIRE(StringBuilder_get_string(builder)[8]==0);
                }

                SECTION("and number") {
                    StringBuilder_append_f(builder, "%03d", 3);

                    REQUIRE(StringBuilder_get_length(builder)==8);
                    REQUIRE(StringBuilder_get_string(builder)!=nullptr);
                    REQUIRE(StringBuilder_get_string(builder)[5]=='0');
                    REQUIRE(StringBuilder_get_string(builder)[6]=='0');
                    REQUIRE(StringBuilder_get_string(builder)[7]=='3');
                    REQUIRE(StringBuilder_get_string(builder)[8]==0);
                }
            }
        }

        SECTION("long string") {
            const int size = 128000;
            char buffer[size+1];
            for (int i = 0; i<size; i++) {
                buffer[i] = charset[rand()%(charset_size-1)];
            }
            buffer[size] = 0;

            StringBuilder_append_f(builder, "%s", buffer);

            REQUIRE(StringBuilder_get_length(builder)==size);
            REQUIRE(StringBuilder_get_string(builder)!=nullptr);
            for (int i = 0; i<size+1; i++) {
                REQUIRE(StringBuilder_get_string(builder)[i]==buffer[i]);
            }
            REQUIRE(StringBuilder_get_string(builder)[size]==0);

            SECTION("and append a number") {
                StringBuilder_append_f(builder, "%d", 1);

                REQUIRE(StringBuilder_get_length(builder)==size+1);
                REQUIRE(StringBuilder_get_string(builder)[size]=='1');
                REQUIRE(StringBuilder_get_string(builder)[size+1]==0);
            }
        }
    }

    StringBuilder_destroy(builder);
}