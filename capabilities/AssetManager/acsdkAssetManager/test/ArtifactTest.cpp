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

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "AuthDelegateMock.h"
#include "InternetConnectionMonitorMock.h"
#include "RequestFactory.h"
#include "Requester.h"
#include "RequesterFactory.h"
#include "TestUtil.h"
#include "acsdkAssetManager/AssetManager.h"
#include "acsdkAssetsInterfaces/Communication/InMemoryAmdCommunicationHandler.h"
#include "acsdkDavsClient/DavsClient.h"
#include "acsdkDavsClient/DavsEndpointHandlerV3.h"

using namespace std;
using namespace testing;
using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::acsdkAssets::commonInterfaces;
using namespace alexaClientSDK::acsdkAssets::davsInterfaces;
using namespace alexaClientSDK::acsdkAssets::davs;
using namespace alexaClientSDK::acsdkAssets::client;
using namespace alexaClientSDK::acsdkAssets::manager;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

class ArtifactTest : public testing::Test {
public:
    void SetUp() override {
        TMP_DIR = createTmpDir("Artifact");
        DAVS_DIR = TMP_DIR + "/davs";
        DAVS_TMP = TMP_DIR + "/davstmp";

        authDelegateMock = AuthDelegateMock::create();
        wifiMonitorMock = InternetConnectionMonitorMock::create();
        davsEndpointHandler = DavsEndpointHandlerV3::create("123");
        allowUrlList = UrlAllowListWrapper::create({"ALL"});
        commsHandler = InMemoryAmdCommunicationHandler::create();

        davsClient = DavsClient::create(DAVS_TMP, authDelegateMock, wifiMonitorMock, davsEndpointHandler);

        auto assetManager = AssetManager::create(commsHandler, davsClient, DAVS_DIR, authDelegateMock, allowUrlList);
        assetManager->onIdleChanged(1);
        storageManager = StorageManager::create(DAVS_DIR, assetManager);
    }

    void TearDown() override {
        filesystem::removeAll(TMP_DIR);
    }

    string TMP_DIR;
    string DAVS_DIR;
    string DAVS_TMP;

    shared_ptr<ArtifactRequest> request =
            DavsRequest::create("test", "tar", {{"filter1", {"value1"}}, {"filter2", {"value2"}}});
    shared_ptr<ArtifactRequest> urlRequest = UrlRequest::create("urlLocation", "fileName", true, "certPath");
    shared_ptr<DavsClient> davsClient;
    shared_ptr<StorageManager> storageManager;
    shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegateMock;
    shared_ptr<InternetConnectionMonitorInterface> wifiMonitorMock;
    shared_ptr<DavsEndpointHandlerV3> davsEndpointHandler;
    shared_ptr<UrlAllowListWrapper> allowUrlList;
    shared_ptr<AmdCommunicationInterface> commsHandler;
};

TEST_F(ArtifactTest, CreateFromDavs) {  // NOLINT
    // clang-format off
    ASSERT_TRUE(nullptr == RequesterFactory::create(nullptr, commsHandler, davsClient, DAVS_TMP, authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == RequesterFactory::create(storageManager, nullptr, davsClient, DAVS_TMP, authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == RequesterFactory::create(storageManager, commsHandler, nullptr, DAVS_TMP, authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == RequesterFactory::create(storageManager, commsHandler, davsClient, "", authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == RequesterFactory::create(storageManager, commsHandler, davsClient, DAVS_TMP, nullptr,allowUrlList));
    // clang-format on

    auto factory = RequesterFactory::create(
            storageManager, commsHandler, davsClient, DAVS_TMP, authDelegateMock, allowUrlList);
    ASSERT_TRUE(nullptr == factory->createFromMetadata(nullptr, DAVS_DIR));
    ASSERT_TRUE(nullptr == factory->createFromMetadata(RequesterMetadata::create(nullptr), DAVS_DIR));
    ASSERT_TRUE(nullptr == factory->createFromMetadata(RequesterMetadata::create(request), ""));
}

TEST_F(ArtifactTest, CreateWithInvalidJsonFails) {  // NOLINT
    auto json = request->toJsonString();
    auto withoutType = string(json).replace(json.find("artifactType"), strlen("artifactType"), "artifactHype");
    auto withoutKey = string(json).replace(json.find("artifactKey"), strlen("artifactKey"), "artifactBey");
    auto withoutFilters = string(json).replace(json.find("filters"), strlen("filters"), "jitters");
    auto withoutEndpoint = string(json).replace(json.find("endpoint"), strlen("endpoint"), "endjoint");
    auto withoutUnpack = string(json).replace(json.find("unpack"), strlen("unpack"), "tupack");

    ASSERT_TRUE(nullptr == RequestFactory::create("{}"));
    ASSERT_TRUE(nullptr == RequestFactory::create(withoutType));
    ASSERT_TRUE(nullptr == RequestFactory::create(withoutKey));
    ASSERT_TRUE(nullptr == RequestFactory::create(withoutFilters));
    ASSERT_FALSE(nullptr == RequestFactory::create(withoutEndpoint));  // optional field
    ASSERT_FALSE(nullptr == RequestFactory::create(withoutUnpack));    // optional field
}

TEST_F(ArtifactTest, CreateUrlReqWithInvalidJsonFails) {  // NOLINT
    auto json = urlRequest->toJsonString();
    auto withoutUrl = string(json).replace(json.find("url"), strlen("url"), "urn");
    auto withoutFilename = string(json).replace(json.find("filename"), strlen("filename"), "tilebane");
    auto withoutUnpack = string(json).replace(json.find("unpack"), strlen("unpack"), "tupack");
    auto withoutCertPath = string(json).replace(json.find("certPath"), strlen("certPath"), "bertBath");

    ASSERT_TRUE(nullptr == RequestFactory::create(withoutUrl));
    ASSERT_TRUE(nullptr == RequestFactory::create(withoutFilename));
    ASSERT_FALSE(nullptr == RequestFactory::create(withoutUnpack));    // optional field
    ASSERT_FALSE(nullptr == RequestFactory::create(withoutCertPath));  // optional field
}

TEST_F(ArtifactTest, CreateWithEmptyFilter) {  // NOLINT
    ASSERT_TRUE(nullptr == DavsRequest::create("test", "tar", {{}}));

    auto emptyFiltersRequest = DavsRequest::create("test", "tar", {});
    ASSERT_FALSE(nullptr == emptyFiltersRequest);

    auto json = emptyFiltersRequest->toJsonString();
    ASSERT_TRUE(json.find("filters") != string::npos);

    auto recreatedRequest = RequestFactory::create(json);
    ASSERT_FALSE(nullptr == recreatedRequest);
    ASSERT_TRUE(recreatedRequest->toJsonString() == emptyFiltersRequest->toJsonString());
}
