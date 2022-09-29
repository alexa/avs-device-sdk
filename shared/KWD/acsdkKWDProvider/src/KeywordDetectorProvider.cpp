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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "KWDProvider/KeywordDetectorProvider.h"

using namespace alexaClientSDK;
using namespace alexaClientSDK::kwd;

/// String to identify log entries originating from this file.
#define TAG "KeywordDetectorProvider"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

KeywordDetectorProvider::KWDCreateMethod m_kwdCreateFunction;

KeywordDetectorProvider::KWDRegistration::KWDRegistration(KWDCreateMethod createFunction) {
    if (m_kwdCreateFunction) {
        ACSDK_ERROR(LX(__func__).m("KeywordDetector already registered"));
        return;
    }
    m_kwdCreateFunction = createFunction;
}

std::unique_ptr<acsdkKWDImplementations::AbstractKeywordDetector> KeywordDetectorProvider::create(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    avsCommon::utils::AudioFormat audioFormat,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
        keyWordDetectorStateObservers) {
    if (m_kwdCreateFunction) {
        return m_kwdCreateFunction(stream, audioFormat, keyWordObservers, keyWordDetectorStateObservers);
    } else {
        ACSDK_ERROR(LX(__func__).m("KeywordDetector create not found"));
        return nullptr;
    }
}
