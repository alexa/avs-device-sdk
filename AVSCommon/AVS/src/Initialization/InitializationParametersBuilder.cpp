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

#include "AVSCommon/AVS/Initialization/InitializationParametersBuilder.h"
#include "AVSCommon/Utils/Timing/TimerDelegateFactory.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

std::unique_ptr<InitializationParametersBuilder> InitializationParametersBuilder::create() {
    return std::unique_ptr<InitializationParametersBuilder>(new InitializationParametersBuilder());
}

InitializationParametersBuilder::InitializationParametersBuilder() {
    m_initParams.timerDelegateFactory = std::make_shared<utils::timing::TimerDelegateFactory>();
}

InitializationParametersBuilder& InitializationParametersBuilder::withJsonStreams(
    const std::shared_ptr<std::vector<std::shared_ptr<std::istream>>>& jsonStreams) {
    m_initParams.jsonStreams = jsonStreams;
    return *this;
}

InitializationParametersBuilder& InitializationParametersBuilder::withPowerResourceManager(
    const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>& powerResourceManager) {
    m_initParams.powerResourceManager = powerResourceManager;
    return *this;
}

InitializationParametersBuilder& InitializationParametersBuilder::withTimerDelegateFactory(
    const std::shared_ptr<avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface>& timerDelegateFactory) {
    m_initParams.timerDelegateFactory = timerDelegateFactory;
    return *this;
}

std::unique_ptr<InitializationParameters> InitializationParametersBuilder::build() {
    std::unique_ptr<InitializationParameters> initParams(new InitializationParameters(m_initParams));
    return initParams;
}

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
