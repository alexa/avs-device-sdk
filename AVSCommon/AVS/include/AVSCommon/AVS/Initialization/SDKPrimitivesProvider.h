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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_SDKPRIMITIVESPROVIDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_SDKPRIMITIVESPROVIDER_H_

#include <memory>
#include <mutex>

#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateFactoryInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/**
 * Provides primitives to components.
 * This class should only be used for objects which are impractical to pass as an explicit dependency.
 */
class SDKPrimitivesProvider {
public:
    /**
     * Returns the Singleton instance of the SDKPrimitivesProvider.
     *
     * @return The SDKPrimitivesProvider instance.
     */
    static std::shared_ptr<SDKPrimitivesProvider> getInstance();

    /**
     * Sets the @c TimerDelegateFactoryInterface.
     *
     * @return Whether this operation was successful.
     */
    bool withTimerDelegateFactory(
        std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> timerDelegateFactory);

    /**
     * Get the @c TimerDelegateFactoryInterface.
     *
     * @return The @c TimerDelegateFactoryInterface.
     */
    std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> getTimerDelegateFactory();

    /**
     * Initialize with the provided members.
     *
     * @return Whether this operation was successful.
     */
    bool initialize();

    /**
     * Initialize.
     *
     * @return Whether this is currently initialized.
     */
    bool isInitialized() const;

    /**
     * Reset all configured properties.
     */
    void reset();

    /**
     * Resets all configured properties. Resets the Singleton instance.
     */
    void terminate();

private:
    /// Constructor.
    SDKPrimitivesProvider();

    /// The Singleton instance.
    static std::shared_ptr<SDKPrimitivesProvider> m_provider;

    /// The mutex.
    static std::mutex m_mutex;

    /// Whether or not it is initialized.
    bool m_initialized;

    /// The @c TimerDelegateFactoryInterface.
    std::shared_ptr<sdkInterfaces::timing::TimerDelegateFactoryInterface> m_timerDelegateFactory;
};

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_SDKPRIMITIVESPROVIDER_H_
