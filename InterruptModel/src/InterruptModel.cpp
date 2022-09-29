/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "InterruptModel/InterruptModel.h"

namespace alexaClientSDK {
namespace afml {
namespace interruptModel {
using namespace avsCommon::avs;
using namespace avsCommon::utils::configuration;

#define TAG "InterruptModel"
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Key for the interrupt model configuration
static const std::string INTERRUPT_MODEL_CONFIG_KEY = "interruptModel";

static const std::string CURRENT_CHANNEL_CONTENT_TYPE_CONFIG_KEY = "contentType";
static const std::string HIGHPRIORITY_CHANNEL_CONFIG_ROOT_KEY = "incomingChannel";
static const std::string HIGHPRIORITY_CHANNEL_CONTENT_TYPE_CONFIG_KEY = "incomingContentType";

std::shared_ptr<InterruptModel> InterruptModel::createInterruptModel(const std::shared_ptr<ConfigurationNode>& config) {
    if (!config) {
        ACSDK_ERROR(LX("createInterruptModelFailed").m("invalid config"));
        return nullptr;
    }
    return InterruptModel::create((*config)[INTERRUPT_MODEL_CONFIG_KEY]);
}

std::shared_ptr<InterruptModel> InterruptModel::create(ConfigurationNode interactionConfiguration) {
    if (!interactionConfiguration) {
        ACSDK_ERROR(LX(__func__).m("Invalid interactionConfiguration"));
        return nullptr;
    }

    return std::shared_ptr<InterruptModel>(new InterruptModel(interactionConfiguration));
}

InterruptModel::InterruptModel(ConfigurationNode interactionConfiguration) :
        m_interactionConfiguration(interactionConfiguration) {
}

MixingBehavior InterruptModel::getMixingBehavior(
    const std::string& lowPrioChannel,
    ContentType lowPrioContentType,
    const std::string& highPrioChannel,
    ContentType highPrioContentType) const {
    ACSDK_INFO(LX(__func__)
                   .d("lowPriochannel", lowPrioChannel)
                   .d("lowPrioContentType", lowPrioContentType)
                   .d("highPrioChannel", highPrioChannel)
                   .d("highPrioContentType", highPrioContentType));

    auto lowPrioChannelConfig = m_interactionConfiguration[lowPrioChannel];
    if (!lowPrioChannelConfig) {
        ACSDK_WARN(LX(__func__).d("Channel Not found", lowPrioChannel));
        return MixingBehavior::UNDEFINED;
    }

    auto lowPrioChannelInteractionConfig = lowPrioChannelConfig[CURRENT_CHANNEL_CONTENT_TYPE_CONFIG_KEY];
    if (!lowPrioChannelInteractionConfig) {
        ACSDK_WARN(LX(__func__).d("No InteractionConfig found for ", lowPrioChannel));
        return MixingBehavior::UNDEFINED;
    }

    auto lowPrioChannelContentTypeConfig = lowPrioChannelInteractionConfig[contentTypeToString(lowPrioContentType)];
    if (!lowPrioChannelContentTypeConfig) {
        ACSDK_WARN(LX(__func__)
                       .m("No ContentType Config found")
                       .d("channel", lowPrioChannel)
                       .d("contentType", lowPrioContentType));
        return MixingBehavior::UNDEFINED;
    }

    auto highPrioChannelConfigRoot = lowPrioChannelContentTypeConfig[HIGHPRIORITY_CHANNEL_CONFIG_ROOT_KEY];
    if (!highPrioChannelConfigRoot) {
        ACSDK_WARN(LX(__func__)
                       .m("No Config found for")
                       .d("highPrioChannel", highPrioChannel)
                       .d("lowPrioChannel", lowPrioChannel));
        return MixingBehavior::UNDEFINED;
    }

    auto highPrioChannelConfig = highPrioChannelConfigRoot[highPrioChannel];
    if (!highPrioChannelConfig) {
        ACSDK_WARN(LX(__func__).m("No Config found for").d("key", highPrioChannel).d("lowPrioChannel", lowPrioChannel));
        return MixingBehavior::UNDEFINED;
    }

    auto highPrioChannelContentTypeRoot = highPrioChannelConfig[HIGHPRIORITY_CHANNEL_CONTENT_TYPE_CONFIG_KEY];
    if (!highPrioChannelContentTypeRoot) {
        ACSDK_WARN(
            LX(__func__).m("No Config found for").d("key", highPrioContentType).d("lowPrioChannel", lowPrioChannel));
        return MixingBehavior::UNDEFINED;
    }

    std::string retMixingBehaviorStr;
    if (!highPrioChannelContentTypeRoot.getString(contentTypeToString(highPrioContentType), &retMixingBehaviorStr)) {
        ACSDK_WARN(LX(__func__).d("Key Not Found", contentTypeToString(highPrioContentType)));
        return MixingBehavior::UNDEFINED;
    }

    MixingBehavior retMixingBehavior = avsCommon::avs::getMixingBehavior(retMixingBehaviorStr);
    if (MixingBehavior::UNDEFINED == retMixingBehavior) {
        ACSDK_ERROR(LX(__func__).d("Invalid MixingBehavior specified", retMixingBehaviorStr));
    }

    return retMixingBehavior;
}
}  // namespace interruptModel
}  // namespace afml
}  // namespace alexaClientSDK
