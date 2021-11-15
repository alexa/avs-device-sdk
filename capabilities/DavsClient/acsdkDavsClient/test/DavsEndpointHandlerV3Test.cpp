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

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "TestUtil.h"
#include "acsdkDavsClient/DavsEndpointHandlerV3.h"

using namespace std;
using namespace testing;
using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::acsdkAssets::commonInterfaces;
using namespace alexaClientSDK::acsdkAssets::davs;

struct TestDataEndpoints {
    string description;
    string segmentId;
    string locale;
    string type;
    string key;
    DavsRequest::FilterMap filters;
    Region region;
    string resultUrl;
};

class DavsEndpointHandlerV3Test
        : public Test
        , public WithParamInterface<TestDataEndpoints> {};

TEST_F(DavsEndpointHandlerV3Test, InvalidCreate) {  // NOLINT
    ASSERT_EQ(DavsEndpointHandlerV3::create(""), nullptr);
}

TEST_F(DavsEndpointHandlerV3Test, InvalidInputs) {  // NOLINT
    auto unit = DavsEndpointHandlerV3::create("123");
    ASSERT_NE(unit, nullptr);
    ASSERT_EQ(unit->getDavsUrl(nullptr), "");
}

TEST_P(DavsEndpointHandlerV3Test, TestWithVariousEndpointCombinations) {  // NOLINT
    auto& p = GetParam();
    auto unit = DavsEndpointHandlerV3::create(p.segmentId, p.locale);

    auto actualUrl = unit->getDavsUrl(DavsRequest::create(p.type, p.key, p.filters, p.region));
    ASSERT_EQ(actualUrl, p.resultUrl);
}

// clang-format off
INSTANTIATE_TEST_CASE_P(EndpointChecks, DavsEndpointHandlerV3Test, ValuesIn<vector<TestDataEndpoints>>(  // NOLINT
    // Description         Segment   Locale   Type     Key     Filters               Region    || Result URL
    {{"NA Endpoint"      , "123456", "en-US", "Type1", "Key1", {{"F", {"A", "B"}}}, Region::NA, "https://api.amazonalexa.com/v3/segments/123456/artifacts/Type1/Key1?locale=en-US&encodedFilters=eyJGIjpbIkEiLCJCIl19"},
     {"EU Endpoint"      , "ABCDEF", "en-US", "Type2", "Key2", {{"F", {"A", "B"}}}, Region::EU, "https://api.eu.amazonalexa.com/v3/segments/ABCDEF/artifacts/Type2/Key2?locale=en-US&encodedFilters=eyJGIjpbIkEiLCJCIl19"},
     {"FE Endpoint"      , "UVWXYZ", "en-US", "Type3", "Key3", {{"F", {"A", "B"}}}, Region::FE, "https://api.fe.amazonalexa.com/v3/segments/UVWXYZ/artifacts/Type3/Key3?locale=en-US&encodedFilters=eyJGIjpbIkEiLCJCIl19"},
     {"No locale"        , "123456", ""     , "Type4", "Key4", {{"F", {"A", "B"}}}, Region::NA, "https://api.amazonalexa.com/v3/segments/123456/artifacts/Type4/Key4?encodedFilters=eyJGIjpbIkEiLCJCIl19"},
     {"No filters"       , "123456", "en-GB", "Type5", "Key5", {}                 , Region::NA, "https://api.amazonalexa.com/v3/segments/123456/artifacts/Type5/Key5?locale=en-GB"},
     {"No filters/locale", "123456", ""     , "Type6", "Key6", {}                 , Region::NA, "https://api.amazonalexa.com/v3/segments/123456/artifacts/Type6/Key6"},
}), PrintDescription());
// clang-format on
