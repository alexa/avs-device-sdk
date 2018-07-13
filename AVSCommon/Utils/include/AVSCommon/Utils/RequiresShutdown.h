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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_REQUIRESSHUTDOWN_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_REQUIRESSHUTDOWN_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Abstract base class which requires the derived class to implement a shutdown function and tries to verify that
 * client code calls @c shutdown() correctly.
 */
class RequiresShutdown {
public:
    /**
     * Constructor.
     *
     * @param name The name of the class or object which requires shutdown calls.  Used in log messages when problems
     *    are detected in shutdown or destruction sequences.
     */
    RequiresShutdown(const std::string& name);

    /// Destructor.
    virtual ~RequiresShutdown();

    /**
     * Returns the name of this object.
     *
     * @return The name of the object.
     */
    const std::string& name() const;

    /**
     * Prepares/enables this object to be deleted.  This should be the last function called on this object prior to
     * deleting (or resetting) its shared_ptr.
     *
     * @warning
     * @li Attempting to call functions on this object after calling shutdown() can result in undefined behavior.
     * @li Neglecting to call shutdown() on this object can result in resource leaks or other undefined behavior.
     */
    void shutdown();

    /**
     * Checks whether this object has had @c shutdown() called on it.
     *
     * @return @c true if this object has been shut down, else @c false.
     */
    bool isShutdown() const;

protected:
    /// @copydoc shutdown()
    virtual void doShutdown() = 0;

private:
    /// The name of the derived class.
    const std::string m_name;

    /// Mutex to protect shutdown.
    mutable std::mutex m_mutex;

    /// Flag tracking whether @c shutdown() has been called on this instance.
    bool m_isShutdown;
};

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_REQUIRESSHUTDOWN_H_
