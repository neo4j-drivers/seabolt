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
#include "test-context.h"
#include <tuple>
#include <algorithm>

void record_debug_log(void* state, const char* message)
{
    vector<string>* messages = (vector<string>*) state;
    messages->push_back(string("DEBUG: ")+message);
}

void record_info_log(void* state, const char* message)
{
    vector<string>* messages = (vector<string>*) state;
    messages->push_back(string("INFO: ")+message);
}

void record_warning_log(void* state, const char* message)
{
    vector<string>* messages = (vector<string>*) state;
    messages->push_back(string("WARNING: ")+message);
}

void record_error_log(void* state, const char* message)
{
    vector<string>* messages = (vector<string>*) state;
    messages->push_back(string("ERROR: ")+message);
}

BoltLog* create_recording_logger(vector<string>* messages)
{
    BoltLog* logger = BoltLog_create(messages);
    BoltLog_set_debug_func(logger, &record_debug_log);
    BoltLog_set_info_func(logger, &record_info_log);
    BoltLog_set_warning_func(logger, &record_warning_log);
    BoltLog_set_error_func(logger, &record_error_log);
    return logger;
}

TestContext::TestContext()
{
    this->recording_log = create_recording_logger(&this->recorded_log_messages);
}

TestContext::~TestContext()
{
    for (auto it = this->calls_cleanup.cbegin(); it!=this->calls_cleanup.cend(); ++it) {
		delete[] *it;
    }

    BoltLog_destroy(this->recording_log);
}

void TestContext::reset()
{
    queue<tuple<string, int, intptr_t*>> empty;
    swap(this->calls, empty);
    this->calls_vector.clear();
    this->recorded_log_messages.clear();
}

void TestContext::add_call(string name, intptr_t value)
{
    intptr_t* values = new intptr_t[1];
    values[0] = value;
    this->calls_cleanup.push_back(values);
    this->calls.push(tuple<string, int, intptr_t*>(name, 1, values));
}

void TestContext::add_call(string name, intptr_t value1, intptr_t value2)
{
    intptr_t* values = new intptr_t[2];
    values[0] = value1;
    values[1] = value2;
    this->calls_cleanup.push_back(values);
    this->calls.push(tuple<string, int, intptr_t*>(name, 2, values));
}

tuple<string, int, intptr_t*> TestContext::next_call()
{
    tuple<string, int, intptr_t*> expected = this->calls.front();
    this->calls.pop();
    return expected;
}

void TestContext::record_call(string name)
{
    this->calls_vector.push_back(name);
}

BoltLog* TestContext::log()
{
    return this->recording_log;
}

bool TestContext::contains_log(string message) const
{
    auto begin = this->recorded_log_messages.cbegin();
    auto end = this->recorded_log_messages.cend();
    return std::find(begin, end, message)!=end;
}
