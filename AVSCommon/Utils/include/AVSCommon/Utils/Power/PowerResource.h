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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_POWERRESOURCE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_POWERRESOURCE_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

/**
 * An object representing a configuration of power level preferences.
 *
 * Behavior is undefined if direct calls are made to @c PowerResourceManagerInterface using
 * the same component identifier as one associated with a @c PowerResource object.
 */
class PowerResource {
public:
    /**
     * Destructor. This will release all acquired instances.
     */
    ~PowerResource();

    /**
     * Prefix that will be internally appended before calling @c PowerResourceManagerInterface.
     */
    static constexpr const char* PREFIX = "ACSDK_";

    /**
     * Creates an instance of the @c PowerResource.
     *
     * @param identifier The identifier. This identifier must be unique across all instances, as it will be used
     * to call the underlying @c PowerResourceManagerInterface. This will be prefixed internally with
     * ACSDK_ to maintain uniqueness within @c PowerResourceManagerInterface.
     * @param powerManager A pointer to the underlying @c PowerResourceManagerInterface.
     * @param level The level to create this resource with.
     * @param refCounted Whether refcounting is enabled.
     *
     * @return An instance.
     */
    static std::shared_ptr<PowerResource> create(
        const std::string& identifier,
        std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> powerManager,
        sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level =
            sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel::STANDBY_MED,
        bool refCounted = true);

    /**
     * Returns the id. This will equal the identifier passed into the constructor without the internal prefix.
     *
     * @return A string representing the id.
     */
    std::string getId() const;

    /**
     * Returns whether the current resource is refCounted.
     *
     * @return A bool indicating refCount state.
     */
    bool isRefCounted() const;

    /**
     * Returns whether the current resource is frozen.
     *
     * @return A bool indicating frozen state.
     */
    bool isFrozen() const;

    /**
     * Get the current level.
     *
     * @return The current level.
     */
    sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel getLevel() const;

    /**
     * Acquire a count of the resource.
     */
    void acquire();

    /**
     * Release a count of the resource.
     */
    void release();

    /**
     * Freezes the resource, and caches the current refcount. Any calls to acquire or release will no-op while
     * the @c PowerResource is frozen.
     */
    void freeze();

    /**
     * Thaws the resource, and re-acquires the amount of times the resource has been acquired.
     */
    void thaw();

private:
    /**
     * Constructor
     *
     * @param identifier The identifier.
     * @param powerManager A pointer to the underlying @c PowerResourceManagerInterface.
     * @param level The level to create this resource with.
     * @param refCounted Whether refcounting is enabled.
     * @param powerResourceId The identifier returned from the @c PowerResourceManagerInterface.
     */
    PowerResource(
        const std::string& identifier,
        std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> powerManager,
        sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel level,
        bool refCounted,
        std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface::PowerResourceId> powerResourceId);

    /// Identifier name.
    const std::string m_identifier;

    /// Whether this resource is refCounted.
    const bool m_isRefCounted;

    /// The PowerResourceId object used to call @c PowerResourceManagerInterface.
    std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface::PowerResourceId> m_powerResourceId;

    /// Thread safety.
    mutable std::mutex m_mutex;

    /// The current refCount.
    uint64_t m_refCount;

    /// Level of the resource. Can be modified as different components may wish to obtain different levels.
    sdkInterfaces::PowerResourceManagerInterface::PowerResourceLevel m_level;

    /// Whether whether the @c PowerResource is frozen.
    bool m_isFrozen;

    /// The underlying @c PowerResourceManagerInterface
    std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface> m_powerManager;
};

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_POWER_POWERRESOURCE_H_
