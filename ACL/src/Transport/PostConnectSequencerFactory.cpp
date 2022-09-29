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

#include <set>

#include "ACL/Transport/PostConnectSequencer.h"
#include "ACL/Transport/PostConnectSequencerFactory.h"

#include "AVSCommon/Utils/Logger/Logger.h"

namespace alexaClientSDK {
namespace acl {

using namespace avsCommon::sdkInterfaces;
using namespace acsdkPostConnectOperationProviderRegistrarInterfaces;

/// String to identify log entries originating from this file.
#define TAG "PostConnectSequencerFactory"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * An implementation of PostConnectOperationProviderRegistrarInterface that adapts the existing interface
 * of PostConnectSequencerFactory that takes a vector of providers to the new PostConnectSequencerFactory
 * that takes an instance of PostConnectOperationProviderRegistrarInterface.
 */
class LegacyProviderRegistrar : public PostConnectOperationProviderRegistrarInterface {
public:
    /**
     * Constructor.
     *
     * @param postConnectOperationProviders The providers to (pre) register.
     */
    explicit LegacyProviderRegistrar(
        const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& postConnectOperationProviders);

    /// @name PostConnectOperationProviderRegistrarInterface methods.
    /// @{
    bool registerProvider(
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>& provider) override;
    avsCommon::utils::Optional<
        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>>>
    getProviders() override;
    /// @}

private:
    /// The registered providers.
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> m_providers;
};

LegacyProviderRegistrar::LegacyProviderRegistrar(
    const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& providers) :
        m_providers{providers} {
}

bool LegacyProviderRegistrar::registerProvider(const std::shared_ptr<PostConnectOperationProviderInterface>& provider) {
    return false;
}

avsCommon::utils::Optional<
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>>>
LegacyProviderRegistrar::getProviders() {
    return m_providers;
}

std::shared_ptr<PostConnectFactoryInterface> PostConnectSequencerFactory::createPostConnectFactoryInterface(
    const std::shared_ptr<PostConnectOperationProviderRegistrarInterface>& registrar) {
    if (!registrar) {
        ACSDK_ERROR(LX("createPostConectFactoryInterfaceFailed").d("reason", "nullRegistrar"));
        return nullptr;
    }
    return std::shared_ptr<PostConnectSequencerFactory>(new PostConnectSequencerFactory(registrar));
}

std::shared_ptr<PostConnectSequencerFactory> PostConnectSequencerFactory::create(
    const std::vector<std::shared_ptr<PostConnectOperationProviderInterface>>& providers) {
    for (auto& provider : providers) {
        if (!provider) {
            ACSDK_ERROR(LX("createFailed").d("reason", "invalidProviderFound"));
            return nullptr;
        }
    }
    auto registrar = std::make_shared<LegacyProviderRegistrar>(providers);
    return std::shared_ptr<PostConnectSequencerFactory>(new PostConnectSequencerFactory(registrar));
}

PostConnectSequencerFactory::PostConnectSequencerFactory(
    const std::shared_ptr<PostConnectOperationProviderRegistrarInterface>& registrar) :
        m_registrar{registrar} {
}

std::shared_ptr<PostConnectInterface> PostConnectSequencerFactory::createPostConnect() {
    auto providers = m_registrar->getProviders();
    if (!providers.hasValue()) {
        ACSDK_ERROR(LX("createPostConnectFailed").d("reason", "nullProviders"));
        return nullptr;
    }
    PostConnectSequencer::PostConnectOperationsSet postConnectOperationsSet;
    for (auto& provider : providers.value()) {
        auto postConnectOperation = provider->createPostConnectOperation();
        if (postConnectOperation) {
            postConnectOperationsSet.insert(postConnectOperation);
        }
    }
    return PostConnectSequencer::create(postConnectOperationsSet);
}

}  // namespace acl
}  // namespace alexaClientSDK
