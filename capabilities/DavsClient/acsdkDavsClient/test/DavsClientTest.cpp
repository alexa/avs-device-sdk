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
#include <rapidjson/document.h>

#include <chrono>
#include <memory>
#include <thread>

#include "AuthDelegateMock.h"
#include "CurlWrapperMock.h"
#include "DavsServiceMock.h"
#include "TestUtil.h"
#include "acsdkAssetsCommon/Base64Url.h"
#include "acsdkDavsClient/DavsEndpointHandlerV3.h"
#include "acsdkDavsClient/DavsHandler.h"

using namespace std;
using namespace chrono;
using namespace testing;
using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::acsdkAssets::commonInterfaces;
using namespace alexaClientSDK::acsdkAssets::davsInterfaces;
using namespace alexaClientSDK::acsdkAssets::davs;
using namespace alexaClientSDK::avsCommon::utils;

// through trials, initial time is the minimum time we need to wait for downloadCheck to finish
static milliseconds INTIIAL_BACKOFF_VALUE = milliseconds(100);
static milliseconds BASE_BACKOFF_VALUE = milliseconds(10);
static milliseconds MAX_BACKOFF_VALUE = milliseconds(50);

// Same as: cat source | sed -e 's/what/replacement/' except that it crashes if it can't find what in source
static string replace_string(const string& source, const string& what, const string& replacement);

class MyDownloader : public DavsDownloadCallbackInterface {
public:
    void onStart() override {
        started = true;
    }
    void onArtifactDownloaded(std::shared_ptr<VendableArtifact>, const std::string& artifactPath) override {
        path = artifactPath;
        downloaded = true;
        failure = false;
    }
    void onDownloadFailure(ResultCode) override {
        failure = true;
    }
    void onProgressUpdate(int prog) override {
        this->progress = prog;
    }

    bool started;
    bool downloaded;
    bool failure;
    int progress;
    string path;
};

class MyChecker : public DavsCheckCallbackInterface {
public:
    bool checkIfOkToDownload(std::shared_ptr<VendableArtifact>, size_t) override {
        return okToDownload;
    }
    void onCheckFailure(ResultCode) override {
        checkFailure = true;
    }

    bool okToDownload;
    bool checkFailure;
};

struct TestDataForCheckArtifact {
    string description;
    string request;
    bool getResult;
    string response;
    bool checkFailure;
    bool downloadAttempted;
    string contentType;
};

class CheckArtifactTest
        : public Test
        , public WithParamInterface<TestDataForCheckArtifact> {
public:
    void SetUp() override {
        DAVS_TEST_DIR = createTmpDir("Artifact");
        testData = GetParam();

        checker = make_shared<MyChecker>();
        downloader = make_shared<MyDownloader>();

        checker->okToDownload = true;
        checker->checkFailure = false;
        downloader->started = false;
        downloader->downloaded = false;
        downloader->failure = false;

        CurlWrapperMock::mockResponse = testData.response;
        CurlWrapperMock::getResult = testData.getResult;
        CurlWrapperMock::header = testData.contentType;
    }

    void TearDown() override {
        filesystem::removeAll(DAVS_TEST_DIR);
    }

    bool jsonEquals(const string& json1, const string& json2) {
        using namespace rapidjson;

        Document d1(kObjectType);
        Document d2(kObjectType);

        d1.Parse(json1.c_str());
        d2.Parse(json2.c_str());

        return (d1 == d2);
    }

    string DAVS_TEST_DIR;

    TestDataForCheckArtifact testData;
    shared_ptr<MyChecker> checker;
    shared_ptr<MyDownloader> downloader;
    shared_ptr<DavsRequest> request;
    shared_ptr<DavsHandler> handler;
};

TEST_P(CheckArtifactTest, parametrizedTest) {  // NOLINT
    DavsRequest::FilterMap filterMap;
    filterMap["locale"] = {"en-US"};
    filterMap["modelClass"] = {"B"};

    CurlWrapperMock::useDavsService = false;

    request = DavsRequest::create("wakeword", "alexa", filterMap);
    handler = DavsHandler::create(
            request,
            downloader,
            checker,
            DAVS_TEST_DIR,
            BASE_BACKOFF_VALUE,
            MAX_BACKOFF_VALUE,
            AuthDelegateMock::create(),
            DavsEndpointHandlerV3::create("123"));
    handler->requestAndDownload(true);

    ASSERT_TRUE(waitUntil([this] { return checker->checkFailure == testData.checkFailure; }));
    ASSERT_TRUE(waitUntil([this] { return downloader->started == testData.downloadAttempted; }));
    ASSERT_TRUE(jsonEquals(CurlWrapperMock::capturedRequest, testData.request));
}

string valid_request =
        R"({"artifactType":"wakeword","artifactKey":"alexa","filters":{"locale":["en-US"],"modelClass":["B"]}})";
string valid_response = R"(
{
   "urlExpiryEpoch": 1537400172798,
   "artifactType": "wakeword",
   "artifactSize": 4485147,
   "artifactKey": "alexa",
   "artifactTimeToLive": 1537400172798,
   "downloadUrl": "https://device-artifacts-v2.s3.amazonaws.com/wakeword-alexa-8aac547c6d1c48cc16dc317900d3ba8e.tar.gz?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Date=20180919T223612Z&X-Amz-SignedHeaders=host&X-Amz-Expires=3600&X-Amz-Credential=AKIAJTPKJI7A3WTMPCQQ%2F20180919%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Signature=87160eda6c8325e9ce61120974cdf2f81c4b8a3d47c1192f6cf4b7500ed17165",
   "artifactIdentifier": "8aac547c6d1c48cc16dc317900d3ba8e"
})";
auto insecure_response = replace_string(valid_response, "https://", "http://");
auto negative_expiry = replace_string(valid_response, "\"urlExpiryEpoch\": 1537400172798,", "\"urlExpiryEpoch\": -12,");
auto int_as_string = replace_string(valid_response, "4485147", "\"4485147\"");

// clang-format off
string valid_multipart_full_response = R"(--90378d5d-5961-4c14-9105-d968ad9f2ba8\r
Content-Type: application/json

{"artifactType": "wakeword","artifactKey": "alexa","artifactTimeToLive": 1570483487469,"artifactIdentifier": "56e662bbcaafd80f6eadcbe1bb6d837a","artifactSize": 55, "checksum": {"md5": "56e662bbcaafd80f6eadcbe1bb6d837a"}}
--90378d5d-5961-4c14-9105-d968ad9f2ba8\r
Content-Type: application/octet-stream

000 blobbyblobbyblobbyblobby
001 blobbyblobbyblobbyblobby
002 blobbyblobbyblobbyblobby
003 blobbyblobbyblobbyblobby
004 blobbyblobbyblobbyblobby
005 blobbyblobbyblobbyblobby
006 blobbyblobbyblobbyblobby
007 blobbyblobbyblobbyblobby
008 blobbyblobbyblobbyblobby
009 blobbyblobbyblobbyblobby
010 blobbyblobbyblobbyblobby
011 blobbyblobbyblobbyblobby
012 blobbyblobbyblobbyblobby
)";
INSTANTIATE_TEST_CASE_P(ArtifactCheckTest, CheckArtifactTest, ValuesIn<vector<TestDataForCheckArtifact>>(
    // description             request         getResponse  response                        checkFailure   downloadAttempted  contentType
    {{"Happy case"            , valid_request , true       , valid_response                , false        , true             , "Content-Type: application/json"},
     {"HTTP GET failed"       , valid_request , false      , ""                            , true         , false            , "Content-Type: application/json"},
     {"GET failed w/response" , valid_request , false      , valid_response                , true         , false            , "Content-Type: application/json"},
     {"Empty response"        , valid_request , true       , ""                            , true         , false            , "Content-Type: application/json"},
     {"Invalid JSON"          , valid_request , true       , "Golden Fleece"               , true         , false            , "Content-Type: application/json"},
     {"Valid JSON, no data"   , valid_request , true       , "{\"AMZN\":1917}"             , true         , false            , "Content-Type: application/json"},
     {"Valid JSON, bad data"  , valid_request , true       , negative_expiry               , true         , false            , "Content-Type: application/json"},
     {"String instead of int" , valid_request , true       , int_as_string                 , true         , false            , "Content-Type: application/json"},
     {"Insecure response OK"  , valid_request , true       , insecure_response             , false        , true             , "Content-Type: application/json"},
     {"Multi-part happy case" , valid_request , true       , valid_multipart_full_response , false        , false            , "Content-Type: multipart/mixed; boundary=--90378d5d-5961-4c14-9105-d968ad9f2ba8\r\n"}
}), PrintDescription());
// clang-format on

static string replace_string(const string& source, const string& what, const string& replacement) {
    string result = source;
    size_t pos = result.find(what, 0);
    result.replace(pos, what.length(), replacement);
    return result;
}

class DownloadTest : public Test {
public:
    DavsServiceMock service;

    void SetUp() override {
        DAVS_TEST_DIR = createTmpDir("Artifact");

        DavsRequest::FilterMap filterMap;
        filterMap["locale"] = {"en-US"};

        checker = make_shared<MyChecker>();
        downloader = make_shared<MyDownloader>();
        request = DavsRequest::create("wakeword", "alexa", filterMap);
        handler = DavsHandler::create(
                move(request),
                downloader,
                checker,
                DAVS_TEST_DIR,
                BASE_BACKOFF_VALUE,
                MAX_BACKOFF_VALUE,
                AuthDelegateMock::create(),
                DavsEndpointHandlerV3::create("123"));
        CurlWrapperMock::useDavsService = true;
    }

    void TearDown() override {
        filesystem::removeAll(DAVS_TEST_DIR);
        CurlWrapperMock::downloadShallFail = false;
        CurlWrapperMock::useDavsService = false;
    }

    string DAVS_TEST_DIR;

    shared_ptr<MyChecker> checker;
    shared_ptr<MyDownloader> downloader;
    shared_ptr<DavsRequest> request;
    shared_ptr<DavsHandler> handler;
};

TEST_F(DownloadTest, fromPublishToDownloadTest) {  // NOLINT
    DavsServiceMock::FilterMap metadata;
    metadata["locale"] = {"en-US"};

    string artifactToDownload;
    Base64Url::encode("TestContent", artifactToDownload);
    service.uploadBase64Artifact("wakeword", "alexa", metadata, artifactToDownload, seconds(10));

    checker->okToDownload = true;
    handler->requestAndDownload(true);

    ASSERT_TRUE(waitUntil([this] { return downloader->started; }));
    ASSERT_TRUE(waitUntil([this] { return downloader->downloaded; }));
    ASSERT_EQ(
            filesystem::basenameOf(downloader->path), "wakeword_alexa_" + DavsServiceMock::getId(metadata) + ".tar.gz");
    ASSERT_FALSE(downloader->failure);
}

TEST_F(DownloadTest, downloadWithThrottlingTest) {  // NOLINT
    DavsServiceMock::FilterMap metadata;
    metadata["locale"] = {"en-US"};

    string artifactToDownload;
    Base64Url::encode("TestContent", artifactToDownload);
    service.uploadBase64Artifact("wakeword", "alexa", metadata, artifactToDownload, seconds(10));

    checker->okToDownload = true;
    handler->setThrottled(true);
    handler->requestAndDownload(true);

    ASSERT_TRUE(waitUntil([this] { return downloader->started; }));
    ASSERT_TRUE(waitUntil([this] { return downloader->downloaded; }));
    ASSERT_EQ(
            filesystem::basenameOf(downloader->path), "wakeword_alexa_" + DavsServiceMock::getId(metadata) + ".tar.gz");
    ASSERT_FALSE(downloader->failure);
}

TEST_F(DownloadTest, downloadFailureDoesntRetryForeverTest) {  // NOLINT
    DavsServiceMock::FilterMap metadata;
    metadata["locale"] = {"en-US"};

    string artifactToDownload;
    Base64Url::encode("TestContent", artifactToDownload);
    service.uploadBase64Artifact("wakeword", "alexa", metadata, artifactToDownload, seconds(10));

    checker->okToDownload = true;

    // After download attempts start, the first download will wait
    handler->setFirstBackOff(INTIIAL_BACKOFF_VALUE);
    handler->requestAndDownload(true);

    ASSERT_TRUE(waitUntil([this] { return downloader->started; }));

    // After download starts, it shall end in failure
    CurlWrapperMock::downloadShallFail = true;
    ASSERT_TRUE(waitUntil([this] { return downloader->failure; }, seconds(1)));
    ASSERT_FALSE(downloader->downloaded);
}

TEST_F(DownloadTest, downloadCanRecoverTest) {  // NOLINT
    DavsServiceMock::FilterMap metadata;
    metadata["locale"] = {"en-US"};

    string artifactToDownload;
    Base64Url::encode("TestContent", artifactToDownload);
    service.uploadBase64Artifact("wakeword", "alexa", metadata, artifactToDownload, seconds(10));

    checker->okToDownload = true;

    // After download attempts start, the first download will wait
    handler->setFirstBackOff(INTIIAL_BACKOFF_VALUE);
    handler->requestAndDownload(true);

    ASSERT_TRUE(waitUntil([this] { return downloader->started; }, milliseconds(500)));

    // After download starts, it shall end in failure
    CurlWrapperMock::downloadShallFail = true;
    ASSERT_FALSE(downloader->downloaded);
    this_thread::sleep_for(INTIIAL_BACKOFF_VALUE);

    // now the download shall recover and successful
    CurlWrapperMock::downloadShallFail = false;
    ASSERT_TRUE(waitUntil([this] { return downloader->downloaded; }, seconds(1)));
    ASSERT_FALSE(downloader->failure);
}

TEST_F(DownloadTest, downloadRetriesWithThrottlingTest) {  // NOLINT
    DavsServiceMock::FilterMap metadata;
    metadata["locale"] = {"en-US"};

    string artifactToDownload;
    Base64Url::encode("TestContent", artifactToDownload);
    service.uploadBase64Artifact("wakeword", "alexa", metadata, artifactToDownload, seconds(10));

    checker->okToDownload = true;

    // After download attempts start, the first download will wait
    handler->setFirstBackOff(INTIIAL_BACKOFF_VALUE);
    handler->requestAndDownload(true);

    ASSERT_TRUE(waitUntil([this] { return downloader->started; }, milliseconds(500)));

    // After download starts, it shall end in failure
    CurlWrapperMock::downloadShallFail = true;
    handler->setThrottled(false);
    ASSERT_FALSE(downloader->downloaded);
    this_thread::sleep_for(INTIIAL_BACKOFF_VALUE);

    // now the download shall recover and successful
    handler->setThrottled(true);
    CurlWrapperMock::downloadShallFail = false;
    ASSERT_TRUE(waitUntil([this] { return downloader->downloaded; }, seconds(1)));
    ASSERT_FALSE(downloader->failure);
}

// Tests that bad values do not extend beyond the base or max
TEST_F(DownloadTest, BackOffTimeLimitsTest) {  // NOLINT
    milliseconds baseTimeMs = handler->getBackOffTime(milliseconds(0));
    ASSERT_EQ(baseTimeMs.count(), BASE_BACKOFF_VALUE.count());

    baseTimeMs = handler->getBackOffTime(milliseconds(-100));
    ASSERT_EQ(baseTimeMs.count(), BASE_BACKOFF_VALUE.count());

    milliseconds maxTimeMs = handler->getBackOffTime(seconds(1000000));
    ASSERT_EQ(maxTimeMs.count(), MAX_BACKOFF_VALUE.count());
}

TEST_F(DownloadTest, BackOffIncrements) {  // NOLINT
    auto backoffTime = handler->getBackOffTime(milliseconds(0));
    ASSERT_EQ(backoffTime.count(), BASE_BACKOFF_VALUE.count());

    milliseconds prevBackoffTime;
    while (backoffTime < MAX_BACKOFF_VALUE) {
        prevBackoffTime = backoffTime;
        backoffTime = handler->getBackOffTime(backoffTime);
        ASSERT_GT(backoffTime, prevBackoffTime);
    }

    backoffTime = handler->getBackOffTime(backoffTime);
    ASSERT_EQ(backoffTime.count(), MAX_BACKOFF_VALUE.count());
}

string default_value = "default_value";
struct UrlData {
    string url;
    string result;
};
class UrlParserTest
        : public Test
        , public WithParamInterface<UrlData> {};

TEST_P(UrlParserTest, urlParser) {  // NOLINT
    auto p = GetParam();
    ASSERT_EQ(DavsHandler::parseFileFromLink(p.url, default_value), p.result);
}

// clang-format off
INSTANTIATE_TEST_CASE_P(PossibleTests, UrlParserTest, ValuesIn<vector<UrlData>>(
        {{"https://device-artifacts-v2.s3.amazonaws.com/file.tar.gz?X-Amz-Algorithm=AW", "file.tar.gz"},
         {"http://device-artifacts-v2.s3.amazonaws.com/file.tar.gz", "file.tar.gz"},
         {"https://device-artifacts-v2.s3.amazonaws.com/file.tar.gz?X-Amz-Algorithm=AW", "file.tar.gz"},
         {"https://s3.amazonaws.com/f", "f"},
         {"https://s3.amazonaws.com/f?", "f"},
         {"https://s3.amazonaws.com/?hi/file.tar.gz?bye", "file.tar.gz"},
         {"https://amazonaws.com/file.tar.gz?X-Amz-Algorithm=AW", "file.tar.gz"},
         {"https://azamonaws.com/file.tar.gz?X-Amz-Algorithm=AW", default_value},
         {"https://s3.amazon.com/file.tar.gz?X-Amz-Algorithm=AW", default_value},
         {"https://s3.amazonaws.com/?X-Amz", default_value},
        }));
// clang-format on