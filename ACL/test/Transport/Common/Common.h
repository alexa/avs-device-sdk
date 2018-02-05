/*
 * Common.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates.
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
#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_COMMON_COMMON_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_COMMON_COMMON_H_

#include <string>

namespace alexaClientSDK {
namespace acl {
namespace test {

/**
 * Utility function to generate a random string of characters between 'a' - 'z'.
 *
 * @param stringSize The length of the string we will create.
 * @return The generated string.
 */
std::string createRandomAlphabetString(int stringSize);

/**
 * Utility function to generate a random number.  This function does not require seeding before use.
 *
 * @param min The minimum value in the desired distribution.
 * @param max The maximum value in the desired distribution.
 * @return The generated number.
 */
int generateRandomNumber(int min, int max);

}  // namespace test
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_COMMON_COMMON_H_
