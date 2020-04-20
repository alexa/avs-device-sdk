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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKLOCALEASSETSMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKLOCALEASSETSMANAGER_H_

#include <gmock/gmock.h>

#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace test {

/**
 * Mock implementation of LocaleAssetsManagerInterface.
 */
class MockLocaleAssetsManager : public LocaleAssetsManagerInterface {
public:
    MOCK_METHOD2(changeAssets, bool(const Locales& locale, const WakeWords& wakeWords));
    MOCK_METHOD0(cancelOngoingChange, void());
    MOCK_CONST_METHOD0(getDefaultSupportedWakeWords, WakeWordsSets());
    MOCK_CONST_METHOD0(getLanguageSpecificWakeWords, std::map<LanguageTag, WakeWordsSets>());
    MOCK_CONST_METHOD0(getLocaleSpecificWakeWords, std::map<Locale, WakeWordsSets>());
    MOCK_CONST_METHOD1(getSupportedWakeWords, WakeWordsSets(const Locale&));
    MOCK_CONST_METHOD0(getSupportedLocales, std::set<Locale>());
    MOCK_CONST_METHOD0(getSupportedLocaleCombinations, LocaleCombinations());
    MOCK_CONST_METHOD0(getDefaultLocale, Locale());
};

}  // namespace test
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_TEST_AVSCOMMON_SDKINTERFACES_MOCKLOCALEASSETSMANAGER_H_
