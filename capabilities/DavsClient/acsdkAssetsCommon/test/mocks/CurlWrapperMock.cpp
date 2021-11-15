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

#include "CurlWrapperMock.h"

#include <rapidjson/document.h>

#include <cstdarg>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "DavsServiceMock.h"
#include "acsdkAssetsCommon/Base64Url.h"
#include "acsdkAssetsCommon/CurlWrapper.h"

using namespace std;
using namespace rapidjson;

using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::acsdkAssets::commonInterfaces;

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

bool CurlWrapperMock::getResult = false;
string CurlWrapperMock::root = "";
string CurlWrapperMock::capturedRequest = "";
string CurlWrapperMock::mockResponse = "";
bool CurlWrapperMock::useDavsService = false;
bool CurlWrapperMock::downloadShallFail = false;
string CurlWrapperMock::header = "Content-Type: application/json\n Content-Length: 160000";

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

// Mock CURL by using stub implementations; this enables much better coverage than mocking the entire CurlWrapper.

extern "C" {

typedef size_t (*WRITE_CALLBACK)(char* ptr, size_t size, size_t nmemb, void* userdata);

struct MyCurlContext {
    WRITE_CALLBACK callback;
    void* callbackData;
    string preparedResponse;
    bool returnNotFound;
    bool headerRequest = false;
    bool headAndData = false;
    WRITE_CALLBACK headerCallback;
    void* headerCallbackData;
};

CURL* curl_easy_init(void) {
    MyCurlContext* c = new MyCurlContext();
    c->returnNotFound = false;
    return (CURL*)c;
}

static void prepareResponseBasedOnFile(MyCurlContext* c, const string& fileName) {
    fstream response(fileName, ios::in);
    stringstream ss;
    if (response.good()) {
        ss << response.rdbuf();
        c->preparedResponse = ss.str();
    }
    c->returnNotFound = c->preparedResponse.empty();
}

CURLcode curl_easy_setopt(CURL* curl, CURLoption option, ...) {
    auto c = static_cast<MyCurlContext*>(curl);
    va_list args;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvarargs"
#endif
    va_start(args, option);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    if (option == CURLOPT_WRITEFUNCTION) {
        c->callback = va_arg(args, WRITE_CALLBACK);
    } else if (option == CURLOPT_WRITEDATA) {
        c->callbackData = va_arg(args, void*);
    } else if (option == CURLOPT_NOBODY) {
        c->headerRequest = va_arg(args, int) == 1;
    } else if (option == CURLOPT_HEADERDATA) {
        c->headerCallbackData = va_arg(args, void*);
    } else if (option == CURLOPT_HEADERFUNCTION) {
        c->headerCallback = va_arg(args, WRITE_CALLBACK);
        c->headAndData = true;
    } else if (option == CURLOPT_URL) {
        string url = va_arg(args, const char*);
        string artifactStart = "artifacts/";
        auto typeStart = url.find(artifactStart) + artifactStart.size();
        auto keyStart = url.find('/', typeStart) + 1;
        auto queryStart = url.find('?', keyStart) + 1;
        auto type = url.substr(typeStart, keyStart - typeStart - 1);
        auto key = url.substr(keyStart, queryStart - keyStart - 1);

        const char* filterPart = "encodedFilters=";
        const char* substr = strstr(url.c_str(), filterPart);
        if (strstr(url.c_str(), "test://") != nullptr) {
            prepareResponseBasedOnFile(c, &url[7]);
            if (c->returnNotFound) {
                va_end(args);
                return CURLE_HTTP_RETURNED_ERROR;
            }
        } else if (substr != nullptr) {
            string capturedFilter;
            Base64Url::decode(substr + strlen(filterPart), capturedFilter);
            CurlWrapperMock::capturedRequest = R"({"artifactType":")" + type + R"(","artifactKey":")" + key + R"(",)" +
                                               R"("filters":)" + capturedFilter + R"(})";

            if (CurlWrapperMock::useDavsService) {
                Document document(kObjectType);
                document.Parse(capturedFilter.c_str());
                DavsServiceMock::FilterMap filterMap;
                for (auto elem = document.MemberBegin(); elem != document.MemberEnd(); elem++) {
                    for (auto& subFilter : elem->value.GetArray()) {
                        filterMap[elem->name.GetString()].insert(subFilter.GetString());
                    }
                }
                auto id = string() + type + "_" + key + "_" + DavsServiceMock::getId(filterMap);

                prepareResponseBasedOnFile(c, CurlWrapperMock::root + "/" + id + ".response");
            } else {
                c->preparedResponse = CurlWrapperMock::mockResponse;
                c->returnNotFound = false;
            }
        } else {
            CurlWrapperMock::capturedRequest.clear();
            const char* idPart = strrchr(url.c_str(), '/');
            const char* tgzPart = strstr(url.c_str(), ".tar.gz");
            if (idPart == nullptr || tgzPart == nullptr) {
                c->returnNotFound = true;
            } else {
                string id(idPart + 1, tgzPart);
                prepareResponseBasedOnFile(c, CurlWrapperMock::root + "/" + id + ".artifact");
            }
        }
    }

    va_end(args);

    return CURLE_OK;
}

CURLcode curl_easy_perform(CURL* curl) {
    auto c = static_cast<MyCurlContext*>(curl);

    if (c->returnNotFound) {
        return CURLE_HTTP_NOT_FOUND;
    }
    size_t written;
    size_t expectedSize;
    if (c->headerRequest) {
        written =
                c->callback((char*)CurlWrapperMock::header.c_str(), 1, CurlWrapperMock::header.size(), c->callbackData);
        expectedSize = CurlWrapperMock::header.size();
    } else {
        written = c->callback((char*)c->preparedResponse.c_str(), 1, c->preparedResponse.size(), c->callbackData);
        expectedSize = c->preparedResponse.size();
        if (c->headAndData) {
            c->headerCallback(
                    (char*)CurlWrapperMock::header.c_str(), 1, CurlWrapperMock::header.size(), c->headerCallbackData);
        }
    }
    if (written == expectedSize) {
        if (CurlWrapperMock::useDavsService || CurlWrapperMock::getResult) {
            return CURLE_OK;
        } else {
            return CURLE_HTTP_RETURNED_ERROR;
        }
    } else {
        return CURLE_WRITE_ERROR;
    }
}

void curl_easy_cleanup(CURL* curl) {
    auto c = static_cast<MyCurlContext*>(curl);
    delete c;
}

CURLcode curl_easy_getinfo(CURL* curl, CURLINFO info, ...) {
    auto c = static_cast<MyCurlContext*>(curl);
    va_list args;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvarargs"
#endif
    va_start(args, info);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    if (info == CURLINFO_RESPONSE_CODE) {
        if (CurlWrapperMock::downloadShallFail) {
            long* codePtr = va_arg(args, long*);
            *codePtr = 500;
        } else {
            long* codePtr = va_arg(args, long*);
            *codePtr = c->returnNotFound ? 404 : 200;
        }
    }
    va_end(args);
    return CURLE_OK;
}

struct curl_slist* curl_slist_append(struct curl_slist* existing, const char* data) {
    curl_slist* newHead = (curl_slist*)malloc(sizeof(curl_slist));
    newHead->next = existing;
    newHead->data = (char*)data;
    return newHead;
}

void free_recursively(struct curl_slist* head) {
    if (head == nullptr) {
        return;
    }

    free_recursively(head->next);
    free(head);
}

void curl_slist_free_all(struct curl_slist* head) {
    free_recursively(head);
}
}