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

#ifndef AVS_CAPABILITIES_ASSETMANAGER_ACSDKASSETMANAGER_TEST_ASSETMANAGERTEST_H_
#define AVS_CAPABILITIES_ASSETMANAGER_ACSDKASSETMANAGER_TEST_ASSETMANAGERTEST_H_

#include "ArtifactUnderTest.h"
#include "AuthDelegateMock.h"
#include "InternetConnectionMonitorMock.h"
#include "RequesterMetadata.h"
#include "TestUtil.h"
#include "acsdkAssetsInterfaces/Communication/InMemoryAmdCommunicationHandler.h"
#include "archive.h"
#include "archive_entry.h"

using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::acsdkAssets::commonInterfaces;
using namespace alexaClientSDK::acsdkAssets::davsInterfaces;
using namespace alexaClientSDK::acsdkAssets::davs;
using namespace alexaClientSDK::acsdkAssets::client;
using namespace alexaClientSDK::acsdkAssets::manager;
using namespace alexaClientSDK::avsCommon::utils;

class AssetManagerTest : public Test {
public:
    void SetUp() override {
        TMP_DIR = createTmpDir("AssetManager");
        TESTING_DIRECTORY = TMP_DIR + "/davs_testing";
        BASE_DIR = TMP_DIR + "/davs";
        DAVS_TMP = TMP_DIR + "/davstmp";
        DAVS_RESOURCES_DIR = BASE_DIR + "/resources";
        DAVS_REQUESTS_DIR = BASE_DIR + "/requests";
        URL_RESOURCES_DIR = "/tmp/urlResources";
        filesystem::makeDirectory(URL_RESOURCES_DIR);

        commsHandler = InMemoryAmdCommunicationHandler::create();

        tarArtifact->commsHandler = commsHandler;
        unavailableArtifact->commsHandler = commsHandler;
        tarUrlArtifact->commsHandler = commsHandler;
        unavailableUrlArtifact->commsHandler = commsHandler;
        httpUrlArtifact->commsHandler = commsHandler;
        nonApprovedUrlArtifact->commsHandler = commsHandler;

        authDelegateMock = AuthDelegateMock::create();
        // clang-format off
        allowUrlList = UrlAllowListWrapper::create(
                {"https://s3.amazonaws.com/alexareminderservice.prod.usamazon.reminder.earcons/echo_system_alerts_reminder_start_v",
                 "https://tinytts.amazon.com/",
                 "https://tinytts-eu-west-1.amazon.com/",
                 "https://tinytts-us-west-2.amazon.com/",
                 "test://"});
        // clang-format on

        CurlWrapperMock::useDavsService = true;
        CurlWrapperMock::downloadShallFail = false;

        uploadArtifactFromRequest(tarArtifact->request);
        uploadArtifactFromRequest(tarUrlArtifact->request, 100);

        wifiMonitorMock = InternetConnectionMonitorMock::create();
        davsEndpointHandler = DavsEndpointHandlerV3::create("123");
        startAssetManager();
    }

    void TearDown() override {
        shutdownAssetManager();
        filesystem::removeAll(URL_RESOURCES_DIR);
        filesystem::removeAll(TMP_DIR);
    }

    void startAssetManager() {
        if (assetManager != nullptr) {
            return;
        }
        davsClient = DavsClient::create(DAVS_TMP, authDelegateMock, wifiMonitorMock, davsEndpointHandler);
        assetManager = AssetManager::create(commsHandler, davsClient, BASE_DIR, authDelegateMock, allowUrlList);
        ASSERT_NE(assetManager, nullptr);
        assetManager->onIdleChanged(1);
    }

    void shutdownAssetManager() {
        if (assetManager == nullptr) {
            return;
        }
        assetManager.reset();
    }

    void uploadArtifactFromRequest(
            const shared_ptr<ArtifactRequest>& request,
            size_t size = 1,
            const string& id = "",
            milliseconds ttlDelta = minutes(60)) {
        filesystem::makeDirectory(TESTING_DIRECTORY);
        auto metadata = RequesterMetadata::create(request);
        auto type = metadata->getRequest()->getRequestType();
        if (type == Type::DAVS) {
            auto davsRequest = static_pointer_cast<DavsRequest>(request);
            if (davsRequest != nullptr) {
                service.uploadBinaryArtifact(
                        davsRequest->getType(),
                        davsRequest->getKey(),
                        davsRequest->getFilters(),
                        createTarFile(TESTING_DIRECTORY, "target", size),
                        ttlDelta,
                        id);
            }
        } else if (type == Type::URL) {
            auto urlRequest = static_pointer_cast<UrlRequest>(request);
            if (urlRequest != nullptr) {
                createTarFile(URL_RESOURCES_DIR, "urlTarget", 1);
            }
        }
    }

    static string createTarFile(const string& dir, const string& filename, size_t size = 1) {
        auto tarPath = dir + "/" + filename + ".tar.gz";
        auto data = string(size, 'a');

        auto a = archive_write_new();
        archive_write_add_filter_gzip(a);
        archive_write_set_format_pax_restricted(a);
        archive_write_open_filename(a, tarPath.c_str());

        auto entry = archive_entry_new();
        archive_entry_set_pathname(entry, filename.c_str());
        archive_entry_set_size(entry, static_cast<la_int64_t>(data.size()));
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(a, entry);
        archive_write_data(a, data.c_str(), data.size());
        archive_entry_free(entry);

        archive_write_close(a);
        archive_write_free(a);
        return tarPath;
    }

    string TMP_DIR;
    string TESTING_DIRECTORY;
    string BASE_DIR;
    string DAVS_TMP;
    string DAVS_RESOURCES_DIR;
    string DAVS_REQUESTS_DIR;
    string URL_RESOURCES_DIR;
    string URL_WORKING_DIR;

    DavsServiceMock service;
    shared_ptr<DavsClient> davsClient;
    shared_ptr<AssetManager> assetManager;
    shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegateMock;
    shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> wifiMonitorMock;
    shared_ptr<DavsEndpointHandlerV3> davsEndpointHandler;
    shared_ptr<UrlAllowListWrapper> allowUrlList;
    shared_ptr<AmdCommunicationInterface> commsHandler;

    // clang-format off
    shared_ptr<ArtifactUnderTest> tarArtifact = make_shared<ArtifactUnderTest>( nullptr, DavsRequest::create("test", "tar", {{"filter1", {"value1"}}, {"filter2", {"value2"}}}, Region::NA, ArtifactRequest::UNPACK));
    shared_ptr<ArtifactUnderTest> unavailableArtifact = make_shared<ArtifactUnderTest>( nullptr, DavsRequest::create("test", "not_found", {{"filter1", {"value1"}}}, Region::NA, ArtifactRequest::UNPACK));
    shared_ptr<ArtifactUnderTest> tarUrlArtifact = make_shared<ArtifactUnderTest>(nullptr, UrlRequest::create("test:///tmp/urlResources/urlTarget.tar.gz", "urlArtifact", ArtifactRequest::UNPACK));
    shared_ptr<ArtifactUnderTest> unavailableUrlArtifact = make_shared<ArtifactUnderTest>( nullptr, UrlRequest::create("test:///unavailableUrlArtifact", "unavailableUrlArtifact"));
    shared_ptr<ArtifactUnderTest> httpUrlArtifact = make_shared<ArtifactUnderTest>(nullptr, UrlRequest::create("http://tinytts.amazon.com/", "httpUrlArtifact"));
    shared_ptr<ArtifactUnderTest> nonApprovedUrlArtifact = make_shared<ArtifactUnderTest>(nullptr, UrlRequest::create("https://evil.com/", "nonApprovedUrlArtifact"));
    // clang-format on
};

#endif  // AVS_CAPABILITIES_ASSETMANAGER_ACSDKASSETMANAGER_TEST_ASSETMANAGERTEST_H_
