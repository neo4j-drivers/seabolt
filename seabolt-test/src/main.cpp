/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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


#define CATCH_CONFIG_RUNNER  // This tells Catch to provide a main()
#include "catch.hpp"

extern "C" {
	#include "bolt/lifecycle.h"
}

int main(int argc, char* argv[]) {
	Bolt_startup(stderr);

	int result = Catch::Session().run(argc, argv);

	Bolt_shutdown();

	return result;
}