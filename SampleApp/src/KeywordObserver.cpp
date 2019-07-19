/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "SampleApp/KeywordObserver.h"

namespace alexaClientSDK {
namespace sampleApp {

KeywordObserver::KeywordObserver(
    std::shared_ptr<defaultClient::DefaultClient> client,
    capabilityAgents::aip::AudioProvider audioProvider,
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider) :
        m_client{client},
        m_audioProvider{audioProvider},
        m_espProvider{espProvider} {
}

void KeywordObserver::onKeyWordDetected(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    std::string keyword,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    if (endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex == avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        if (m_client) {
            m_client->notifyOfTapToTalk(m_audioProvider, endIndex);
        }
    } else if (
        endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        auto espData = capabilityAgents::aip::ESPData::getEmptyESPData();
        if (m_espProvider) {
            espData = m_espProvider->getESPData();
        }

        if (m_client) {
            // TODO(ACSDK-1976): We need to take into consideration the keyword duration.
            auto startOfSpeechTimestamp = std::chrono::steady_clock::now();
            m_client->notifyOfWakeWord(
                m_audioProvider, beginIndex, endIndex, keyword, startOfSpeechTimestamp, espData, KWDMetadata);
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
