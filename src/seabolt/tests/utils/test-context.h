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
#ifndef SEABOLT_TEST_CONTEXT_H
#define SEABOLT_TEST_CONTEXT_H

#include <string>
#include <vector>
#include <queue>
#include <set>
#include <tuple>
#include "bolt/bolt.h"
#include "catch.hpp"

using namespace std;

class TestContext {
private:
    BoltLog* recording_log;
    vector<string> recorded_log_messages;
    queue<tuple<string, int, intptr_t*>> calls;
    vector<string> calls_vector;
    vector<intptr_t*> calls_cleanup;
public:
    TestContext();

    ~TestContext();

    BoltLog* log();

    vector<string>& recorded_calls() { return calls_vector; }

    vector<string>& recorded_messages() { return recorded_log_messages; }

    tuple<string, int, intptr_t*> next_call();

    void add_call(string name, intptr_t value);

    void add_call(string name, intptr_t value1, intptr_t value2);

    void record_call(string name);

    void reset();

    bool contains_log(string message) const;

};

// The matcher class
class ContainsLogMatcher : public Catch::MatcherBase<TestContext> {
    string m_msg;
    bool m_negate;
public:
    ContainsLogMatcher(string message, bool negate)
            :m_msg(message), m_negate(negate) { }

    // Performs the test for this matcher
    virtual bool match(TestContext const& ctx) const override
    {
        bool contains = ctx.contains_log(m_msg);
        return m_negate==!contains;
    }

    // Produces a string describing what this matcher does. It should
    // include any provided data (the begin/ end in this case) and
    // be written as if it were stating a fact (in the output it will be
    // preceded by the value under test).
    virtual std::string describe() const override
    {
        std::ostringstream ss;
        ss << "contains log message \"" << m_msg << "\"";
        return ss.str();
    }
};

inline ContainsLogMatcher ContainsLog(string message)
{
    return ContainsLogMatcher(message, false);
}

inline ContainsLogMatcher NotContainsLog(string message)
{
    return ContainsLogMatcher(message, true);
}

#endif //SEABOLT_TEST_CONTEXT_H
