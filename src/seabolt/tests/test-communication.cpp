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

#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include "integration.hpp"
#include "catch.hpp"
#include "utils/test-context.h"

using namespace Catch::Matchers;
using namespace std;

#define RENDER_STD_MOCK_CALL(type, name) \
    auto ctx = (TestContext*) comm->context; \
    tuple<string, int, intptr_t*> call = ctx->next_call(); \
    if (get<0>(call)!=name) { \
        throw "expected"+get<0>(call)+", but called "+name;\
    }\
    ctx->record_call(name);\
    return (type)(get<2>(call)[0])

int test_open(BoltCommunication* comm, const struct sockaddr_storage* address)
{
    UNUSED(address);
    RENDER_STD_MOCK_CALL(int, "open");
}

int test_close(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(int, "close");
}

int test_send(BoltCommunication* comm, char* buf, int length, int* sent)
{
    UNUSED(buf);
    UNUSED(length);
    UNUSED(sent);
    auto ctx = (TestContext*) comm->context;
    tuple<string, int, intptr_t*> call = ctx->next_call();
    if (get<0>(call)!="send") {
        throw "expected send but called "+get<0>(call);
    }
    ctx->record_call("send");
    intptr_t* values = get<2>(call);
    *sent = (int) values[1];
    return (int) (values[0]);
}

int test_recv(BoltCommunication* comm, char* buf, int length, int* received)
{
    UNUSED(buf);
    UNUSED(length);
    UNUSED(received);
    auto ctx = (TestContext*) comm->context;
    tuple<string, int, intptr_t*> call = ctx->next_call();
    if (get<0>(call)!="recv") {
        throw "expected recv but called "+get<0>(call);
    }
    ctx->record_call("recv");
    intptr_t* values = get<2>(call);
    *received = (int) values[1];
    return (int) (values[0]);
}

int test_destroy(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(int, "destroy");
}

int test_ignore_sigpipe(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(int, "ignore_sigpipe");
}

int test_restore_sigpipe(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(int, "restore_sigpipe");
}

int test_last_error(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(int, "last_error");
}

int test_transform_error(BoltCommunication* comm, int error_code)
{
    UNUSED(error_code);
    RENDER_STD_MOCK_CALL(int, "transform_error");
}

BoltAddress* test_local_endpoint(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(BoltAddress*, "local_endpoint");
}

BoltAddress* test_remote_endpoint(BoltCommunication* comm)
{
    RENDER_STD_MOCK_CALL(BoltAddress*, "remote_endpoint");
}

TEST_CASE("communication", "[unit]")
{
    BoltSocketOptions* sock_opts = BoltSocketOptions_create();
    BoltStatus* status = BoltStatus_create_with_ctx(1024);
    BoltAddress* local = BoltAddress_create("127.0.0.1", "32000");

    TestContext* test_ctx = new TestContext();

    auto comm_under_test = new(BoltCommunication);
    comm_under_test->log = test_ctx->log();
    comm_under_test->sock_opts = sock_opts;
    comm_under_test->context = test_ctx;
    comm_under_test->status = status;

    comm_under_test->open = &test_open;
    comm_under_test->close = &test_close;
    comm_under_test->send = &test_send;
    comm_under_test->recv = &test_recv;
    comm_under_test->destroy = &test_destroy;
    comm_under_test->ignore_sigpipe = &test_ignore_sigpipe;
    comm_under_test->restore_sigpipe = &test_restore_sigpipe;
    comm_under_test->last_error = &test_last_error;
    comm_under_test->transform_error = &test_transform_error;
    comm_under_test->get_local_endpoint = &test_local_endpoint;
    comm_under_test->get_remote_endpoint = &test_remote_endpoint;

    SECTION("BoltCommunication_open") {
        WHEN("unresolved address is given") {
            BoltAddress* unresolved = BoltAddress_create("host", "port");

            THEN("should return BOLT_ADDRESS_NOT_RESOVED") {
                int result = BoltCommunication_open(comm_under_test, unresolved, nullptr);

                REQUIRE(result==BOLT_ADDRESS_NOT_RESOLVED);
            }

            BoltAddress_destroy(unresolved);
        }

        WHEN("resolved address with unsupported address family is given") {
            BoltAddress* unsupported = BoltAddress_create("127.0.0.1", "7687");
            REQUIRE(BoltAddress_resolve(unsupported, NULL, NULL)==0);
            unsupported->resolved_hosts[0].ss_family = AF_IPX;

            int result = BoltCommunication_open(comm_under_test, unsupported, "id-0");

            THEN("should return error") {
                REQUIRE(result==BOLT_UNSUPPORTED);
            }

            AND_THEN("should create a log entry") {
                REQUIRE_THAT(*test_ctx,
                        ContainsLog("ERROR: [id-0]: Unsupported address family "+to_string(AF_IPX)));
            }

            BoltAddress_destroy(unsupported);
        }

        WHEN("resolved address with 1 socket address is given") {
            BoltAddress* remote = BoltAddress_create("127.0.0.1", "7687");
            REQUIRE(BoltAddress_resolve(remote, NULL, NULL)==0);

            test_ctx->reset();

            AND_WHEN("connection attempt fails") {
                test_ctx->add_call("open", BOLT_CONNECTION_REFUSED);

                int result = BoltCommunication_open(comm_under_test, remote, "id-0");

                THEN("should return error") {
                    REQUIRE(result==BOLT_CONNECTION_REFUSED);
                }

                THEN("should log connection attempt") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7687"));
                }
            }

            AND_WHEN("connection attempt succeeds") {
                test_ctx->add_call("open", BOLT_SUCCESS);
                test_ctx->add_call("remote_endpoint", (intptr_t) remote);
                test_ctx->add_call("local_endpoint", (intptr_t) local);

                int result = BoltCommunication_open(comm_under_test, remote, "id-0");

                THEN("should return success") {
                    REQUIRE(result==BOLT_SUCCESS);
                }

                THEN("should log connection attempt") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7687"));
                }

                THEN("should log remote endpoint") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Remote endpoint is 127.0.0.1:7687"));
                }

                THEN("should log local endpoint") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Local endpoint is 127.0.0.1:32000"));
                }
            }

            BoltAddress_destroy(remote);
        }

        WHEN("resolved address with 2 socket address is given") {
            BoltAddress* remote1 = BoltAddress_create("127.0.0.1", "7687");
            REQUIRE(BoltAddress_resolve(remote1, NULL, NULL)==0);
            BoltAddress* remote2 = BoltAddress_create("127.0.0.1", "7688");
            REQUIRE(BoltAddress_resolve(remote2, NULL, NULL)==0);
            BoltAddress* remote = BoltAddress_create("127.0.0.1", "7687");
            remote->n_resolved_hosts = 2;
            remote->resolved_hosts = (struct sockaddr_storage*) malloc(2*sizeof(struct sockaddr_storage));
            memcpy((void*) &remote->resolved_hosts[0], (void*) &remote1->resolved_hosts[0],
                    sizeof(struct sockaddr_storage));
            memcpy((void*) &remote->resolved_hosts[1], (void*) &remote2->resolved_hosts[0],
                    sizeof(struct sockaddr_storage));

            test_ctx->reset();

            AND_WHEN("all connection attempts fail") {
                test_ctx->add_call("open", BOLT_CONNECTION_REFUSED);
                test_ctx->add_call("open", BOLT_NETWORK_UNREACHABLE);

                int result = BoltCommunication_open(comm_under_test, remote, "id-0");

                THEN("should return last error") {
                    REQUIRE(result==BOLT_NETWORK_UNREACHABLE);
                }

                THEN("should log connection attempt 1") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7687"));
                }

                THEN("should log connection attempt 2") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7688"));
                }
            }

            AND_WHEN("first connection attempt succeeds") {
                test_ctx->add_call("open", BOLT_SUCCESS);
                test_ctx->add_call("remote_endpoint", (intptr_t) remote);
                test_ctx->add_call("local_endpoint", (intptr_t) local);

                int result = BoltCommunication_open(comm_under_test, remote, "id-0");

                THEN("should return success") {
                    REQUIRE(result==BOLT_SUCCESS);
                }

                THEN("should log connection attempt") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7687"));
                }

                THEN("should log remote endpoint") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Remote endpoint is 127.0.0.1:7687"));
                }

                THEN("should log local endpoint") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Local endpoint is 127.0.0.1:32000"));
                }
            }

            AND_WHEN("first connection attempt fails") {
                test_ctx->add_call("open", BOLT_CONNECTION_REFUSED);
                test_ctx->add_call("open", BOLT_SUCCESS);
                test_ctx->add_call("remote_endpoint", (intptr_t) remote2);
                test_ctx->add_call("local_endpoint", (intptr_t) local);

                int result = BoltCommunication_open(comm_under_test, remote, "id-0");

                THEN("should return success") {
                    REQUIRE(result==BOLT_SUCCESS);
                }

                THEN("should log connection attempt 1") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7687"));
                }

                THEN("should log connection attempt 2") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Opening IPv4 connection to 127.0.0.1 at port 7688"));
                }

                THEN("should log remote endpoint") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Remote endpoint is 127.0.0.1:7688"));
                }

                THEN("should log local endpoint") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog("INFO: [id-0]: Local endpoint is 127.0.0.1:32000"));
                }
            }

            BoltAddress_destroy(remote1);
            BoltAddress_destroy(remote2);
            BoltAddress_destroy(remote);
        }
    }

    SECTION("BoltCommunication_close") {
        test_ctx->reset();

        WHEN("close succeeds") {
            test_ctx->add_call("ignore_sigpipe", 0);
            test_ctx->add_call("close", 0);
            test_ctx->add_call("restore_sigpipe", 0);

            int result = BoltCommunication_close(comm_under_test, "id-1");

            THEN("should return success") {
                REQUIRE(result==BOLT_SUCCESS);
            }

            THEN("should create a log entry") {
                REQUIRE_THAT(*test_ctx, ContainsLog("DEBUG: [id-1]: Closing socket"));
            }
        }

        WHEN("close fails") {
            test_ctx->add_call("ignore_sigpipe", 0);
            test_ctx->add_call("close", BOLT_END_OF_TRANSMISSION);
            test_ctx->add_call("restore_sigpipe", 0);

            int result = BoltCommunication_close(comm_under_test, "id-1");

            THEN("should return success") {
                REQUIRE(result==BOLT_END_OF_TRANSMISSION);
            }

            THEN("should create a log entry") {
                REQUIRE_THAT(*test_ctx, ContainsLog("DEBUG: [id-1]: Closing socket"));
            }

            THEN("should create a warning log entry") {
                REQUIRE_THAT(*test_ctx, ContainsLog("WARNING: [id-1]: Unable to close socket, return code is "
                        +to_string(BOLT_END_OF_TRANSMISSION)));
            }
        }

        WHEN("ignore_sigaction fails") {
            test_ctx->add_call("ignore_sigpipe", BOLT_UNKNOWN_ERROR);

            int result = BoltCommunication_close(comm_under_test, "id-1");

            THEN("should fail") {
                REQUIRE(result==BOLT_UNKNOWN_ERROR);
            }

            THEN("should not create a log entry") {
                REQUIRE_THAT(*test_ctx, NotContainsLog("DEBUG: [id-1]: Closing socket"));
            }

            THEN("should set status") {
                REQUIRE(status->error==BOLT_UNKNOWN_ERROR);
                REQUIRE_THAT(status->error_ctx, Contains("unable to ignore SIGPIPE"));
            }
        }

        WHEN("restore_sigaction fails") {
            test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
            test_ctx->add_call("close", BOLT_SUCCESS);
            test_ctx->add_call("restore_sigpipe", BOLT_UNKNOWN_ERROR);

            int result = BoltCommunication_close(comm_under_test, "id-1");

            THEN("should fail") {
                REQUIRE(result==BOLT_UNKNOWN_ERROR);
            }

            THEN("should create a log entry") {
                REQUIRE_THAT(*test_ctx, ContainsLog("DEBUG: [id-1]: Closing socket"));
            }

            THEN("should set status") {
                REQUIRE(status->error==BOLT_UNKNOWN_ERROR);
                REQUIRE_THAT(status->error_ctx, Contains("unable to restore SIGPIPE"));
            }
        }
    }

    SECTION("BoltCommunication_send") {
        test_ctx->reset();

        WHEN("size is 0") {
            int result = BoltCommunication_send(comm_under_test, NULL, 0, "id-0");

            THEN("should succeed") {
                REQUIRE(result==BOLT_SUCCESS);
            }

            THEN("should not make any unexpected calls") {
                REQUIRE(test_ctx->recorded_calls().empty());
            }
        }

        WHEN("send fails") {
            AND_WHEN("in first round") {
                test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                test_ctx->add_call("send", BOLT_END_OF_TRANSMISSION, 0);
                test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                int result = BoltCommunication_send(comm_under_test, NULL, 100, "id-1");

                THEN("should fail") {
                    REQUIRE(result==BOLT_STATUS_SET);
                }

                THEN("should not create log entry") {
                    REQUIRE(test_ctx->recorded_messages().empty());
                }

                THEN("should set status") {
                    REQUIRE(status->error==BOLT_END_OF_TRANSMISSION);
                    REQUIRE_THAT(status->error_ctx,
                            Contains("unable to send data: "+to_string(BOLT_END_OF_TRANSMISSION)));
                }

                THEN("should not make any unexpected calls") {
                    REQUIRE_THAT(test_ctx->recorded_calls(),
                            Equals(vector<string>{"ignore_sigpipe", "send", "restore_sigpipe"}));
                }
            }

            AND_WHEN("in later rounds") {
                test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                test_ctx->add_call("send", BOLT_SUCCESS, 75);
                test_ctx->add_call("send", BOLT_END_OF_TRANSMISSION, 0);
                test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                int result = BoltCommunication_send(comm_under_test, NULL, 100, "id-1");

                THEN("should fail") {
                    REQUIRE(result==BOLT_STATUS_SET);
                }

                THEN("should not create log entry") {
                    REQUIRE(test_ctx->recorded_messages().empty());
                }

                THEN("should set status") {
                    REQUIRE(status->error==BOLT_END_OF_TRANSMISSION);
                    REQUIRE_THAT(status->error_ctx,
                            Contains("unable to send data: "+to_string(BOLT_END_OF_TRANSMISSION)));
                }

                THEN("should not make any unexpected calls") {
                    REQUIRE_THAT(test_ctx->recorded_calls(),
                            Equals(vector<string>{"ignore_sigpipe", "send", "send", "restore_sigpipe"}));
                }
            }
        }

        WHEN("size is 100") {
            AND_WHEN("send processes all in one go") {
                test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                test_ctx->add_call("send", BOLT_SUCCESS, 100);
                test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                int result = BoltCommunication_send(comm_under_test, NULL, 100, "id-1");

                THEN("should succeed") {
                    REQUIRE(result==BOLT_SUCCESS);
                }

                THEN("should create a log entry") {
                    REQUIRE_THAT(test_ctx->recorded_messages(),
                            VectorContains(string("INFO: [id-1]: (Sent 100 of 100 bytes)")));
                }

                THEN("should not make any unexpected calls") {
                    REQUIRE_THAT(test_ctx->recorded_calls(),
                            Equals(vector<string>{"ignore_sigpipe", "send", "restore_sigpipe"}));
                }
            }

            AND_WHEN("send processes smaller chunks") {
                test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                test_ctx->add_call("send", BOLT_SUCCESS, 25);
                test_ctx->add_call("send", BOLT_SUCCESS, 25);
                test_ctx->add_call("send", BOLT_SUCCESS, 45);
                test_ctx->add_call("send", BOLT_SUCCESS, 5);
                test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                int result = BoltCommunication_send(comm_under_test, NULL, 100, "id-1");

                THEN("should succeed") {
                    REQUIRE(result==BOLT_SUCCESS);
                }

                THEN("should create a log entry") {
                    REQUIRE_THAT(test_ctx->recorded_messages(),
                            VectorContains(string("INFO: [id-1]: (Sent 100 of 100 bytes)")));
                }

                THEN("should not make any unexpected calls") {
                    REQUIRE_THAT(test_ctx->recorded_calls(),
                            Equals(vector<string>{"ignore_sigpipe", "send", "send", "send", "send",
                                                  "restore_sigpipe"}));
                }
            }
        }

        WHEN("ignore_sigaction fails") {
            test_ctx->add_call("ignore_sigpipe", BOLT_UNKNOWN_ERROR);

            int result = BoltCommunication_send(comm_under_test, NULL, 1, "id-0");

            THEN("should fail") {
                REQUIRE(result==BOLT_UNKNOWN_ERROR);
            }

            THEN("should not create a log entry") {
                REQUIRE(test_ctx->recorded_messages().empty());
            }

            THEN("should set status") {
                REQUIRE(status->error==BOLT_UNKNOWN_ERROR);
                REQUIRE_THAT(status->error_ctx, Contains("unable to ignore SIGPIPE"));
            }

            THEN("should not make any unexpected calls") {
                REQUIRE_THAT(test_ctx->recorded_calls(), Equals(vector<string>{"ignore_sigpipe"}));
            }
        }

        WHEN("restore_sigaction fails") {
            test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
            test_ctx->add_call("send", BOLT_SUCCESS, 25);
            test_ctx->add_call("send", BOLT_SUCCESS, 25);
            test_ctx->add_call("send", BOLT_SUCCESS, 45);
            test_ctx->add_call("send", BOLT_SUCCESS, 5);
            test_ctx->add_call("restore_sigpipe", BOLT_UNKNOWN_ERROR);

            int result = BoltCommunication_send(comm_under_test, NULL, 100, "id-1");

            THEN("should succeed") {
                REQUIRE(result==BOLT_UNKNOWN_ERROR);
            }

            THEN("should set status") {
                REQUIRE(status->error==BOLT_UNKNOWN_ERROR);
                REQUIRE_THAT(status->error_ctx, Contains("unable to restore SIGPIPE"));
            }

            THEN("should not make any unexpected calls") {
                REQUIRE_THAT(test_ctx->recorded_calls(),
                        Equals(vector<string>{"ignore_sigpipe", "send", "send", "send", "send", "restore_sigpipe"}));
            }
        }

    }

    SECTION("BoltCommunication_receive") {
        test_ctx->reset();

        WHEN("min size is 0") {
            int received = 0;
            int result = BoltCommunication_receive(comm_under_test, NULL, 0, 100, &received, "id-0");

            THEN("should succeed") {
                REQUIRE(result==BOLT_SUCCESS);
            }

            THEN("should not make any unexpected calls") {
                REQUIRE(test_ctx->recorded_calls().empty());
            }
        }

        WHEN("min size is 100") {
            int min_size = 100;
            AND_WHEN("max size is 100") {
                int max_size = 100;
                AND_WHEN("recv receives all in one go") {
                    test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 100);
                    test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                    int received = 0;
                    int result = BoltCommunication_receive(comm_under_test, NULL, min_size, max_size, &received,
                            "id-2");

                    THEN("should succeed") {
                        REQUIRE(result==BOLT_SUCCESS);
                    }

                    THEN("returns correct received byte count") {
                        REQUIRE(received==100);
                    }

                    THEN("should create a log entry") {
                        REQUIRE_THAT(test_ctx->recorded_messages(),
                                VectorContains(string("INFO: [id-2]: Received 100 of 100 bytes")));
                    }

                    THEN("should not make any unexpected calls") {
                        REQUIRE_THAT(test_ctx->recorded_calls(),
                                Equals(vector<string>{"ignore_sigpipe", "recv", "restore_sigpipe"}));
                    }

                }

                AND_WHEN("recv receives in smaller chunks") {
                    test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 50);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 30);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 20);
                    test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                    int received = 0;
                    int result = BoltCommunication_receive(comm_under_test, NULL, min_size, max_size, &received,
                            "id-2");

                    THEN("should succeed") {
                        REQUIRE(result==BOLT_SUCCESS);
                    }

                    THEN("returns correct received byte count") {
                        REQUIRE(received==100);
                    }

                    THEN("should create a log entry") {
                        REQUIRE_THAT(test_ctx->recorded_messages(),
                                VectorContains(string("INFO: [id-2]: Received 100 of 100 bytes")));
                    }

                    THEN("should not make any unexpected calls") {
                        REQUIRE_THAT(test_ctx->recorded_calls(),
                                Equals(vector<string>{"ignore_sigpipe", "recv", "recv", "recv", "restore_sigpipe"}));
                    }
                }
            }

            AND_WHEN("max size is 200") {
                int max_size = 200;
                AND_WHEN("recv receives all in one go") {
                    test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 100);
                    test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                    int received = 0;
                    int result = BoltCommunication_receive(comm_under_test, NULL, min_size, max_size, &received,
                            "id-2");

                    THEN("should succeed") {
                        REQUIRE(result==BOLT_SUCCESS);
                    }

                    THEN("returns correct received byte count") {
                        REQUIRE(received==100);
                    }

                    THEN("should create a log entry") {
                        REQUIRE_THAT(test_ctx->recorded_messages(),
                                VectorContains(string("INFO: [id-2]: Received 100 of 100..200 bytes")));
                    }

                    THEN("should not make any unexpected calls") {
                        REQUIRE_THAT(test_ctx->recorded_calls(),
                                Equals(vector<string>{"ignore_sigpipe", "recv", "restore_sigpipe"}));
                    }

                }

                AND_WHEN("recv receives in smaller chunks") {
                    test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 50);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 30);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 20);
                    test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                    int received = 0;
                    int result = BoltCommunication_receive(comm_under_test, NULL, min_size, max_size, &received,
                            "id-2");

                    THEN("should succeed") {
                        REQUIRE(result==BOLT_SUCCESS);
                    }

                    THEN("returns correct received byte count") {
                        REQUIRE(received==100);
                    }

                    THEN("should create a log entry") {
                        REQUIRE_THAT(test_ctx->recorded_messages(),
                                VectorContains(string("INFO: [id-2]: Received 100 of 100..200 bytes")));
                    }

                    THEN("should not make any unexpected calls") {
                        REQUIRE_THAT(test_ctx->recorded_calls(),
                                Equals(vector<string>{"ignore_sigpipe", "recv", "recv", "recv", "restore_sigpipe"}));
                    }

                }

                AND_WHEN("recv receives more than min size") {
                    test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 50);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 30);
                    test_ctx->add_call("recv", BOLT_SUCCESS, 50);
                    test_ctx->add_call("restore_sigpipe", BOLT_SUCCESS);

                    int received = 0;
                    int result = BoltCommunication_receive(comm_under_test, NULL, min_size, max_size, &received,
                            "id-2");

                    THEN("should succeed") {
                        REQUIRE(result==BOLT_SUCCESS);
                    }

                    THEN("returns correct received byte count") {
                        REQUIRE(received==130);
                    }

                    THEN("should create a log entry") {
                        REQUIRE_THAT(test_ctx->recorded_messages(),
                                VectorContains(string("INFO: [id-2]: Received 130 of 100..200 bytes")));
                    }

                    THEN("should not make any unexpected calls") {
                        REQUIRE_THAT(test_ctx->recorded_calls(),
                                Equals(vector<string>{"ignore_sigpipe", "recv", "recv", "recv", "restore_sigpipe"}));
                    }

                }
            }
        }
        WHEN("ignore_sigaction fails") {
            test_ctx->add_call("ignore_sigpipe", BOLT_UNKNOWN_ERROR);

            int received = 0;
            int result = BoltCommunication_receive(comm_under_test, NULL, 100, 100, &received, "id-0");

            THEN("should fail") {
                REQUIRE(result==BOLT_UNKNOWN_ERROR);
            }

            THEN("should not create a log entry") {
                REQUIRE(test_ctx->recorded_messages().empty());
            }

            THEN("should set status") {
                REQUIRE(status->error==BOLT_UNKNOWN_ERROR);
                REQUIRE_THAT(status->error_ctx, Contains("unable to ignore SIGPIPE"));
            }

            THEN("should not make any unexpected calls") {
                REQUIRE_THAT(test_ctx->recorded_calls(), Equals(vector<string>{"ignore_sigpipe"}));
            }
        }

        WHEN("restore_sigaction fails") {
            test_ctx->add_call("ignore_sigpipe", BOLT_SUCCESS);
            test_ctx->add_call("recv", BOLT_SUCCESS, 25);
            test_ctx->add_call("recv", BOLT_SUCCESS, 25);
            test_ctx->add_call("recv", BOLT_SUCCESS, 45);
            test_ctx->add_call("recv", BOLT_SUCCESS, 5);
            test_ctx->add_call("restore_sigpipe", BOLT_UNKNOWN_ERROR);

            int received = 0;
            int result = BoltCommunication_receive(comm_under_test, NULL, 100, 100, &received, "id-0");

            THEN("should succeed") {
                REQUIRE(result==BOLT_UNKNOWN_ERROR);
            }

            THEN("should set status") {
                REQUIRE(status->error==BOLT_UNKNOWN_ERROR);
                REQUIRE_THAT(status->error_ctx, Contains("unable to restore SIGPIPE"));
            }

            THEN("should not make any unexpected calls") {
                REQUIRE_THAT(test_ctx->recorded_calls(),
                        Equals(vector<string>{"ignore_sigpipe", "recv", "recv", "recv", "recv", "restore_sigpipe"}));
            }
        }

    }

    delete test_ctx;
    delete comm_under_test;
    BoltStatus_destroy(status);
    BoltSocketOptions_destroy(sock_opts);
    BoltAddress_destroy(local);
}

