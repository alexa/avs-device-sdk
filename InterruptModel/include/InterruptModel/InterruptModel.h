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

#ifndef ALEXA_CLIENT_SDK_INTERRUPTMODEL_INCLUDE_INTERRUPTMODEL_INTERRUPTMODEL_H_
#define ALEXA_CLIENT_SDK_INTERRUPTMODEL_INCLUDE_INTERRUPTMODEL_INTERRUPTMODEL_H_

#include <map>
#include <memory>
#include <AVSCommon/AVS/ContentType.h>
#include <AVSCommon/AVS/MixingBehavior.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

namespace alexaClientSDK {
namespace afml {
namespace interruptModel {
/**
 * This class contains the interrupt model implementation
 * for the device. This class uses the @c InterruptModelConfiguration
 * passed in during creation to determine the MixingBehavior
 * During Focus State transitions, the AFML shall invoke this class to
 * determine the MixingBehavior to be taken by the ChannelObservers
 * corresponding to the lower priority channel being backgrounded when
 * a higher priority channel barges-in.
 */
class InterruptModel {
public:
    /**
     * Creates an instance of @c InterruptModel
     *
     * @param config Root configuration node.
     * @return instance of @c InterruptModel.
     */
    static std::shared_ptr<InterruptModel> createInterruptModel(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& config);

    /**
     * Creates an instance of @c InterruptModel
     * @param interactionConfiguration interrupt model configuration for device.
     * @deprecated Use createInterruptModel
     * @return instance of @c InterruptModel.
     */
    static std::shared_ptr<InterruptModel> create(
        avsCommon::utils::configuration::ConfigurationNode interactionConfiguration);

    /**
     * get Mixing Behavior for the lower priority channel
     * @param lowPriorityChannel the lower priority channel
     * @param lowPriorityContentType the current content type
     * @param highPriorityChannel the channel barging in
     * @param highPriorityContentType the content type barging in
     * @return MixingBehavior which must be taken by the lower priority channel
     */
    avsCommon::avs::MixingBehavior getMixingBehavior(
        const std::string& lowPriorityChannel,
        avsCommon::avs::ContentType lowPriorityContentType,
        const std::string& highPriorityChannel,
        avsCommon::avs::ContentType highPriorityContentType) const;

private:
    /**
     * Constructor
     * @param interactionConfiguration interrupt model configuration for device.
     */
    InterruptModel(avsCommon::utils::configuration::ConfigurationNode interactionConfiguration);

    /// @c ConfigurationNode that contains the @c InterruptModelConfiguration
    avsCommon::utils::configuration::ConfigurationNode m_interactionConfiguration;
};
}  // namespace interruptModel
}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTERRUPTMODEL_INCLUDE_INTERRUPTMODEL_INTERRUPTMODEL_H_
