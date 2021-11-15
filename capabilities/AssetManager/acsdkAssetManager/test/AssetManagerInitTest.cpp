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

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "AssetManagerTest.h"

using namespace rapidjson;

static constexpr bool WILL_LOAD = true;
static constexpr bool WILL_BE_ERASED = false;

// clang-format off
static auto REQUESTER_VALID                = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_EMPTY_TYPE           = R"({"artifactType":"", "artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_MISSING_TYPE         = R"({                   "artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_EMPTY_KEY            = R"({"artifactType":"T","artifactKey":"", "filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_MISSING_KEY          = R"({"artifactType":"T",                  "filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_EMPTY_FILTER_KEY     = R"({"artifactType":"T","artifactKey":"K","filters":{"" :["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_INVALID_FILTER_VALUE = R"({"artifactType":"T","artifactKey":"K","filters":{"F" :[]},  "endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_MISSING_FILTER_VALUE = R"({"artifactType":"T","artifactKey":"K",                      "endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_INVALID_ENDPOINT     = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":9, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_EMPTY_ENDPOINT       = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":"","unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_MISSING_ENDPOINT     = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},              "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_INVALID_UNPACK       = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":"huh","resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_MISSING_UNPACK       = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0,                "resourceId":"R","priority":3, "usedTimestamp":10})";
static auto REQUESTER_EMPTY_RESOURCE_ID    = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"", "priority":3, "usedTimestamp":10})";
static auto REQUESTER_MISSING_RESOURCE_ID  = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,                 "priority":3, "usedTimestamp":10})";
static auto REQUESTER_INVALID_PRIORITY     = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":7, "usedTimestamp":10})";
static auto REQUESTER_EMPTY_PRIORITY       = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":"","usedTimestamp":10})";
static auto REQUESTER_MISSING_PRIORITY     = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R",              "usedTimestamp":10})";
static auto REQUESTER_EMPTY_TIMESTAMP      = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3, "usedTimestamp":""})";
static auto REQUESTER_MISSING_TIMESTAMP    = R"({"artifactType":"T","artifactKey":"K","filters":{"F":["A"]},"endpoint":0, "unpack":false,"resourceId":"R","priority":3                    })";

static auto RESOURCE_VALID        = R"({"id":"R","size":1 ,"name":"file"})";
static auto RESOURCE_EMPTY_ID     = R"({"id":"" ,"size":1 ,"name":"file"})";
static auto RESOURCE_MISSING_ID   = R"({         "size":1 ,"name":"file"})";
static auto RESOURCE_EMPTY_SIZE   = R"({"id":"R","size":"","name":"file"})";
static auto RESOURCE_MISSING_SIZE = R"({"id":"R",          "name":"file"})";
static auto RESOURCE_EMPTY_NAME   = R"({"id":"R","size":1 ,"name":""    })";
static auto RESOURCE_MISSING_NAME = R"({"id":"R","size":1               })";
// clang-format on

static size_t RESOURCE_SIZE = 1;
static const string RESOURCE_NAME = "file";
static const string RESOURCE_ID = "R";
static const string RESOURCE_METADATA_JSON = "metadata.json";

struct MetadataFileState {
    string requester;
    string resource;
    bool loadsSuccessfully;
    string description;
};

class InitTest
        : public AssetManagerTest
        , public WithParamInterface<MetadataFileState> {
public:
    void SetUp() override {
        AssetManagerTest::SetUp();
        artifact.commsHandler = commsHandler;
        uploadArtifact();
        ASSERT_TRUE(assetManager->downloadArtifact(artifact.request));
        ASSERT_TRUE(artifact.waitUntilStateEquals(State::LOADED));
        ASSERT_TRUE(artifact.hasAllProps());
    }

    void uploadArtifact() {
        filesystem::makeDirectory(TESTING_DIRECTORY);
        auto file = TESTING_DIRECTORY + "/" + RESOURCE_NAME;
        ofstream(file, ios::trunc) << string(RESOURCE_SIZE, 'a');
        auto davsRequest = static_pointer_cast<DavsRequest>(artifact.request);
        if (davsRequest != nullptr) {
            service.uploadBinaryArtifact(
                    davsRequest->getType(), davsRequest->getKey(), davsRequest->getFilters(), file, seconds(10));
        }
    }

    ArtifactUnderTest artifact{nullptr, RequestFactory::create(REQUESTER_VALID)};
};

TEST_P(InitTest, AssetManagerRestarts) {  // NOLINT
    auto& p = GetParam();
    shutdownAssetManager();

    auto requesterFile = DAVS_REQUESTS_DIR + "/" + filesystem::list(DAVS_REQUESTS_DIR)[0];
    ofstream(requesterFile, ios::trunc) << p.requester;

    // do this because resources will derive the ID from the directory name if metadata.json fails or is not found, and
    // we're using id R
    auto forcedResourceIdPath = DAVS_RESOURCES_DIR + "/" + RESOURCE_ID;
    filesystem::move(
            DAVS_RESOURCES_DIR + "/" + filesystem::list(DAVS_RESOURCES_DIR, filesystem::FileType::DIRECTORY)[0],
            forcedResourceIdPath);
    ofstream(forcedResourceIdPath + "/" + RESOURCE_METADATA_JSON, ios::trunc) << p.resource;

    // do the same for the file inside the resource
    auto fileList = filesystem::list(forcedResourceIdPath);
    fileList.erase(remove(fileList.begin(), fileList.end(), RESOURCE_METADATA_JSON), fileList.end());
    filesystem::move(forcedResourceIdPath + "/" + fileList[0], forcedResourceIdPath + "/" + RESOURCE_NAME);

    startAssetManager();
    ASSERT_TRUE(p.loadsSuccessfully == waitUntil([this] { return artifact.hasPathProp(); }, milliseconds(10)));
    ASSERT_TRUE(p.loadsSuccessfully == artifact.hasPriorityProp());
    ASSERT_TRUE(p.loadsSuccessfully == artifact.hasStateProp());
    if (p.loadsSuccessfully) {
        ASSERT_TRUE(filesystem::exists(forcedResourceIdPath));
        ASSERT_TRUE(filesystem::exists(forcedResourceIdPath + "/" + RESOURCE_METADATA_JSON));
        ASSERT_EQ(forcedResourceIdPath + "/" + RESOURCE_NAME, artifact.getPathProp());
        ASSERT_EQ(RESOURCE_SIZE, filesystem::sizeOf(artifact.getPathProp()));

        Document document;
        ifstream ifs(forcedResourceIdPath + "/" + RESOURCE_METADATA_JSON);
        IStreamWrapper is(ifs);
        ASSERT_FALSE(document.ParseStream(is).HasParseError());

        ASSERT_EQ(document["id"].GetString(), RESOURCE_ID);
        ASSERT_EQ(document["name"].GetString(), RESOURCE_NAME);
        ASSERT_EQ(document["size"].GetUint64(), RESOURCE_SIZE);
    }
}

// clang-format off
INSTANTIATE_TEST_CASE_P(RequestersTestCases, InitTest, ValuesIn<vector<MetadataFileState>>(
        // Requester file to be loaded   | Resource file || Will succeed?  | Description
        {{REQUESTER_VALID                , RESOURCE_VALID , WILL_LOAD      , "Loading a valid requester will succeed"},
         {REQUESTER_EMPTY_TYPE           , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with empty type will fail"},
         {REQUESTER_MISSING_TYPE         , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with missing type will fail"},
         {REQUESTER_EMPTY_KEY            , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with empty key will fail"},
         {REQUESTER_MISSING_KEY          , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with missing key will fail"},
         {REQUESTER_EMPTY_FILTER_KEY     , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with empty filter_key will fail"},
         {REQUESTER_INVALID_FILTER_VALUE , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with invalid filter_value will fail"},
         {REQUESTER_MISSING_FILTER_VALUE , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with missing filter_value will fail"},
         {REQUESTER_INVALID_ENDPOINT     , RESOURCE_VALID , WILL_LOAD      , "Loading requester with invalid endpoint will succeed"},
         {REQUESTER_EMPTY_ENDPOINT       , RESOURCE_VALID , WILL_LOAD      , "Loading requester with empty endpoint will succeed"},
         {REQUESTER_MISSING_ENDPOINT     , RESOURCE_VALID , WILL_LOAD      , "Loading requester with missing endpoint will succeed"},
         {REQUESTER_INVALID_UNPACK       , RESOURCE_VALID , WILL_LOAD      , "Loading requester with invalid unpack will succeed"},
         {REQUESTER_MISSING_UNPACK       , RESOURCE_VALID , WILL_LOAD      , "Loading requester with missing unpack will succeed"},
         {REQUESTER_EMPTY_RESOURCE_ID    , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with empty resource id will fail"},
         {REQUESTER_MISSING_RESOURCE_ID  , RESOURCE_VALID , WILL_BE_ERASED , "Loading requester with missing resource id will fail"},
         {REQUESTER_INVALID_PRIORITY     , RESOURCE_VALID , WILL_LOAD      , "Loading requester with invalid priority will succeed"},
         {REQUESTER_EMPTY_PRIORITY       , RESOURCE_VALID , WILL_LOAD      , "Loading requester with empty priority will succeed"},
         {REQUESTER_MISSING_PRIORITY     , RESOURCE_VALID , WILL_LOAD      , "Loading requester with missing priority will succeed"},
         {REQUESTER_EMPTY_TIMESTAMP      , RESOURCE_VALID , WILL_LOAD      , "Loading requester with empty timestamp will succeed"},
         {REQUESTER_MISSING_TIMESTAMP    , RESOURCE_VALID , WILL_LOAD      , "Loading requester with missing timestamp will succeed"}
        }), PrintDescription());

INSTANTIATE_TEST_CASE_P(ResourceTestCases, InitTest, ValuesIn<vector<MetadataFileState>>(
        // Requester file | Resource file to be loaded || Will succeed? | Description
        {{REQUESTER_VALID , RESOURCE_VALID              , WILL_LOAD     , "Loading a valid resource will succeed"},
         {REQUESTER_VALID , RESOURCE_EMPTY_ID           , WILL_LOAD     , "Loading a resource with empty id will succeed"},
         {REQUESTER_VALID , RESOURCE_MISSING_ID         , WILL_LOAD     , "Loading a resource with missing id will succeed"},
         {REQUESTER_VALID , RESOURCE_EMPTY_SIZE         , WILL_LOAD     , "Loading a resource with empty size will succeed"},
         {REQUESTER_VALID , RESOURCE_MISSING_SIZE       , WILL_LOAD     , "Loading a resource with missing size will succeed"},
         {REQUESTER_VALID , RESOURCE_EMPTY_NAME         , WILL_LOAD     , "Loading a resource with empty name will succeed"},
         {REQUESTER_VALID , RESOURCE_MISSING_NAME       , WILL_LOAD     , "Loading a resource with missing name will succeed"}
        }), PrintDescription());
// clang-format on