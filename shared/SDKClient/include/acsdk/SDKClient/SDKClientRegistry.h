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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_SDKCLIENTREGISTRY_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_SDKCLIENTREGISTRY_H_

#include <memory>
#include <unordered_map>

#include <acsdkShutdownManagerInterfaces/ShutdownManagerInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/Utils/PlatformDefinitions.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/TypeIndex.h>

#include "Annotated.h"
#include "FeatureClientInterface.h"
#include "internal/Utils.h"

namespace alexaClientSDK {
namespace sdkClient {

/**
 * The SDKClientRegistry combines a number of feature clients and provides a registry of components and clients.
 * Once built by the @c SDKClientBuilder, the SDKClientRegistry provides methods to allow the application to access
 * components or clients.
 *
 * Once built, the SDKClientRegistry can be used as follows:
 * @code
 * std::shared_ptr<SDKClientRegistry> sdkClientRegistry = ... // From SDKClientBuilder
 * auto client1 = sdkClientRegistry->get<FeatureClient1>() // Retrieve FeatureClient1 client
 * auto component1 = sdkClientRegistry->getComponent<Component1>() // Retrieve component exposed by a FeatureClient
 */
class SDKClientRegistry
        : public std::enable_shared_from_this<SDKClientRegistry>
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Retrieve a feature client from this SDKClientRegistry
     * @tparam ClientType The type of the client to retrieve
     * @return shared_ptr to the client, or nullptr if no such feature client exists
     */
    template <
        typename ClientType,
        typename std::enable_if<std::is_base_of<FeatureClientInterface, ClientType>::value>::type* = nullptr>
    ACSDK_INLINE_VISIBILITY inline std::shared_ptr<ClientType> get();

    /**
     * Retrieve a component provided by a feature client from this SDKClientRegistry
     * @tparam ComponentType The type of the component to retrieve, may also be an annotated type
     * @return shared_ptr to the component of type ComponentType (or the underlying type if an annotated type is used)
     * or nullptr if no such component exists
     */
    template <typename ComponentType>
    ACSDK_INLINE_VISIBILITY inline std::shared_ptr<typename internal::UnderlyingComponentType<ComponentType>::type>
    getComponent();

    /**
     * Register a component with this SDKClientRegistry
     * @tparam ComponentType The components type, or annotated type
     * @param component Shared pointer to the component which is being registered with this SDKClientRegistry
     * @return true if the client was registered successfully, false if the component could not be registered
     */
    template <typename ComponentType>
    ACSDK_INLINE_VISIBILITY inline bool registerComponent(std::shared_ptr<ComponentType> component);

    /**
     * Register an annotated component with this SDKClientRegistry
     * @tparam AnnotatedName The name of this component
     * @tparam ComponentType The components type
     * @param component Shared pointer to the component which is being registered with this SDKClientRegistry
     * @return true if the client was registered successfully, false if the component could not be registered
     */
    template <typename AnnotatedName, typename ComponentType>
    ACSDK_INLINE_VISIBILITY inline bool registerComponent(Annotated<AnnotatedName, ComponentType> component);

    /**
     * Adds a feature client to an existing SDK Client
     * @tparam ClientBuilderType The type of the feature client builder
     * @param feature The feature client to add, all dependencies which are requested by this feature client must
     * already be registered by existing feature clients
     * @return true if the feature was added successfully, false if a failure occurred.
     */
    template <typename FeatureClientBuilderType>
    ACSDK_INLINE_VISIBILITY inline bool addFeature(std::unique_ptr<FeatureClientBuilderType> featureBuilder);

    /// @c RequiresShutdown functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /// Private Constructor
    SDKClientRegistry();

    /// Initializes the SDKClientRegistry following construction, expected to be called by SDKClientBuilder
    bool initialize();

    /**
     * Registers a new client with the @c SDKClientRegistry
     * @param clientTypeId The TypeIndex for the client
     * @param client The feature client object
     * @return true if successful
     */
    bool registerClient(
        const avsCommon::utils::TypeIndex& clientTypeId,
        std::shared_ptr<FeatureClientInterface> client);

    /**
     * Registers a component with the @c SDKClientRegistry
     * @param clientTypeId The TypeIndex for the client
     * @param component The component to register
     * @return true if successful
     */
    bool registerComponent(const avsCommon::utils::TypeIndex& clientTypeId, std::shared_ptr<void> component);

    /**
     * Adds a feature to the @c SDKClientRegistry
     * @param clientTypeId The TypeIndex for the client
     * @param featureBuilder The feature client builder object
     * @param constructorFn The construct function for this builder
     * @return true if successful
     */
    bool addFeature(
        const avsCommon::utils::TypeIndex& clientTypeId,
        std::unique_ptr<FeatureClientBuilderInterface> featureBuilder,
        std::function<std::shared_ptr<FeatureClientInterface>(const std::shared_ptr<SDKClientRegistry>&)>
            constructorFn);

    /// Contains the type to client interface mapping
    std::unordered_map<avsCommon::utils::TypeIndex, std::shared_ptr<FeatureClientInterface>> m_clientMapping;

    /// Contains the type to component mapping
    std::unordered_map<avsCommon::utils::TypeIndex, std::shared_ptr<void>> m_componentMapping;

    /// The shutdown manager
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface> m_shutdownManager;

    /// The shutdown notifier
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> m_shutdownNotifier;

    /// SDKClientBuilder is a friend to allow it to construct the SDKClientRegistry
    friend class SDKClientBuilder;
};
}  // namespace sdkClient
}  // namespace alexaClientSDK

#include "acsdk/SDKClient/internal/SDKClientRegistry_impl.h"
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_SDKCLIENTREGISTRY_H_
