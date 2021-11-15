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

#include "DavsServiceMock.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <rapidjson/document.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#include "CurlWrapperMock.h"
#include "TestUtil.h"
#include "acsdkAssetsCommon/Base64Url.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace chrono;
using namespace rapidjson;
using namespace alexaClientSDK::avsCommon::utils;

DavsServiceMock::DavsServiceMock() {
    CurlWrapperMock::root = createTmpDir("davs_service");
}

DavsServiceMock::~DavsServiceMock() {
    filesystem::removeAll(CurlWrapperMock::root);
    CurlWrapperMock::root = "";
}

void DavsServiceMock::uploadBinaryArtifact(
        const string& type,
        const string& key,
        const FilterMap& metadata,
        const string& filePath,
        milliseconds ttlDelta,
        const std::string& id) {
    fstream input(filePath, ios::in | ios::binary);
    if (input.good()) {
        uploadArtifact(type, key, metadata, input, ttlDelta, id);
    }
}

void DavsServiceMock::uploadBase64Artifact(
        const string& type,
        const string& key,
        const FilterMap& metadata,
        const string& encodedBinary,
        milliseconds ttlDelta,
        const std::string& id) {
    string content;
    if (Base64Url::decode(encodedBinary, content)) {
        uploadArtifact(type, key, metadata, stringstream(content), ttlDelta, id);
    }
}

void DavsServiceMock::uploadArtifact(
        const string& type,
        const string& key,
        const FilterMap& metadata,
        const istream& input,
        milliseconds ttlDelta,
        const std::string& id) {
    auto file = type + "_" + key + "_" + getId(metadata);

    fstream artifact(CurlWrapperMock::root + "/" + file + ".artifact", ios::out | ios::binary);
    if (artifact.fail()) {
        return;
    }
    artifact << input.rdbuf();

    auto size = artifact.tellp();
    auto ttl = duration_cast<milliseconds>(system_clock::now().time_since_epoch() + ttlDelta).count();
    string url = "https://device-artifacts-v2.s3.amazonaws.com/" + file + ".tar.gz";

    fstream response(CurlWrapperMock::root + "/" + file + ".response", ios::out);
    response << "{"
             << R"("urlExpiryEpoch": )" << ttl << "," << endl
             << R"("artifactType": ")" << type << "\"," << endl
             << R"("artifactSize": )" << size << "," << endl
             << R"("artifactKey": ")" << key << "\"," << endl
             << R"("artifactTimeToLive": )" << ttl << "," << endl
             << R"("downloadUrl": ")" << url << "\"," << endl
             << R"("artifactIdentifier": ")" << (id.empty() ? file : id) << "\"" << endl
             << "}";
}

string DavsServiceMock::getId(const FilterMap& map) {
    stringstream ss;
    for (const auto& elem : map) {
        ss << elem.first << ":";
        for (const auto& subElem : elem.second) {
            ss << subElem;
        }
    }
    return to_string(hash<string>()(ss.str()));
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK