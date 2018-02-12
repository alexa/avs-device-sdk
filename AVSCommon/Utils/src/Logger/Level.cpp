/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <cassert>
#include <unordered_map>
#include "AVSCommon/Utils/Logger/Level.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace logger {

#define LEVEL_TO_NAME(name) \
    case Level::name:       \
        return #name;

std::string convertLevelToName(Level in) {
    switch (in) {
        LEVEL_TO_NAME(DEBUG9)
        LEVEL_TO_NAME(DEBUG8)
        LEVEL_TO_NAME(DEBUG7)
        LEVEL_TO_NAME(DEBUG6)
        LEVEL_TO_NAME(DEBUG5)
        LEVEL_TO_NAME(DEBUG4)
        LEVEL_TO_NAME(DEBUG3)
        LEVEL_TO_NAME(DEBUG2)
        LEVEL_TO_NAME(DEBUG1)
        LEVEL_TO_NAME(DEBUG0)
        LEVEL_TO_NAME(INFO)
        LEVEL_TO_NAME(WARN)
        LEVEL_TO_NAME(ERROR)
        LEVEL_TO_NAME(CRITICAL)
        LEVEL_TO_NAME(NONE)
        LEVEL_TO_NAME(UNKNOWN)
    }
    return "UNKNOWN";
}

#define LEVEL_TO_CHAR(name, ch) \
    case Level::name:           \
        return ch;

char convertLevelToChar(Level in) {
    switch (in) {
        LEVEL_TO_CHAR(DEBUG9, '9')
        LEVEL_TO_CHAR(DEBUG8, '8')
        LEVEL_TO_CHAR(DEBUG7, '7')
        LEVEL_TO_CHAR(DEBUG6, '6')
        LEVEL_TO_CHAR(DEBUG5, '5')
        LEVEL_TO_CHAR(DEBUG4, '4')
        LEVEL_TO_CHAR(DEBUG3, '3')
        LEVEL_TO_CHAR(DEBUG2, '2')
        LEVEL_TO_CHAR(DEBUG1, '1')
        LEVEL_TO_CHAR(DEBUG0, '0')
        LEVEL_TO_CHAR(INFO, 'I')
        LEVEL_TO_CHAR(WARN, 'W')
        LEVEL_TO_CHAR(ERROR, 'E')
        LEVEL_TO_CHAR(CRITICAL, 'C')
        LEVEL_TO_CHAR(NONE, 'N')
        LEVEL_TO_CHAR(UNKNOWN, 'U')
    }
    return '?';
}

#define NAME_TO_LEVEL(name) \
    { #name, Level::name }

Level convertNameToLevel(const std::string& in) {
    static std::unordered_map<std::string, Level> nameToLevel = {NAME_TO_LEVEL(DEBUG9),
                                                                 NAME_TO_LEVEL(DEBUG8),
                                                                 NAME_TO_LEVEL(DEBUG7),
                                                                 NAME_TO_LEVEL(DEBUG6),
                                                                 NAME_TO_LEVEL(DEBUG5),
                                                                 NAME_TO_LEVEL(DEBUG4),
                                                                 NAME_TO_LEVEL(DEBUG3),
                                                                 NAME_TO_LEVEL(DEBUG2),
                                                                 NAME_TO_LEVEL(DEBUG1),
                                                                 NAME_TO_LEVEL(DEBUG0),
                                                                 NAME_TO_LEVEL(INFO),
                                                                 NAME_TO_LEVEL(WARN),
                                                                 NAME_TO_LEVEL(ERROR),
                                                                 NAME_TO_LEVEL(CRITICAL),
                                                                 NAME_TO_LEVEL(NONE),
                                                                 NAME_TO_LEVEL(UNKNOWN)};
    auto it = nameToLevel.find(in);
    if (it != nameToLevel.end()) {
        return it->second;
    }
    return Level::UNKNOWN;
}

}  // namespace logger
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
