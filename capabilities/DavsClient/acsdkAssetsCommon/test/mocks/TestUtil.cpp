/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#if defined(FILE_SYSTEM_UTILS_ENABLED)

#include "TestUtil.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>

#include <climits>
#include <iostream>
#include <thread>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace chrono;
using namespace alexaClientSDK::avsCommon::utils;

bool waitUntil(const function<bool()>& validate, milliseconds timeout) {
    auto stopTime = steady_clock::now() + timeout;
    do {
        if (validate()) return true;
        this_thread::sleep_for(milliseconds(1));
    } while (steady_clock::now() < stopTime);
    return false;
}

std::string createTmpDir(const std::string& postfix) {
    char dirName[L_tmpnam + 1]{};
    if (tmpnam(dirName) == nullptr) {
        cerr << "Could not get temporary directory name!" << endl;
        exit(1);
    }
    auto path = dirName + postfix;
    if (!filesystem::makeDirectory(path) || !filesystem::exists(path)) {
        cerr << "Could not create temporary path!" << endl;
        exit(1);
    }

#if defined(__linux__) || defined(__APPLE__)
    // on some OS, the temp path is symbolically linked, which can cause issues for prefix tests
    // to accommodate this, get the realpath of the temp directory
    char resolved_path[PATH_MAX + 1];
    if (::realpath(path.c_str(), resolved_path) == nullptr) {
        cerr << "Could not get real path of the temporary directory!" << endl;
        exit(1);
    }
    path = string(resolved_path);
#endif

    while (*path.rbegin() == '/') {
        path.pop_back();
    }
    return path;
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif