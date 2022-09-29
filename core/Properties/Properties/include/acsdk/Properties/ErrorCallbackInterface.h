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

#ifndef ACSDK_PROPERTIES_ERRORCALLBACKINTERFACE_H_
#define ACSDK_PROPERTIES_ERRORCALLBACKINTERFACE_H_

#include <memory>
#include <string>

namespace alexaClientSDK {
namespace properties {

/**
 * @brief Possible error causes.
 *
 * This enumeration defines supported error reasons for properties open operation.
 *
 * @sa ErrorCallbackInterface
 * @ingroup Lib_acsdkProperties
 */
enum class StatusCode {
    SUCCESS = 1,                 ///< Status code indicating no error. For internal use only.
    UNKNOWN_ERROR = 2,           ///< Any error, that doesn't fit into other categories.
    HSM_ERROR = 3,               ///< HSM API Error.
    CRYPTO_ERROR = 4,            ///< Crypto API Error.
    DIGEST_ERROR = 5,            ///< Data corruption error.
    INNER_PROPERTIES_ERROR = 6,  ///< Underlying properties error.
};

/**
 * @brief Error action.
 *
 * This enumeration defines possible actions when properties framework encounters an error.
 *
 * @sa ErrorCallbackInterface
 * @ingroup Lib_acsdkProperties
 */
enum class Action {
    CONTINUE = 1,    ///< Continue with default behaviour.
    FAIL = 2,        ///< Fail operation. Do not delete data.
    CLEAR_DATA = 3,  ///< Continue operation, delete data.
    RETRY = 4,       ///< Retry operation.
};

/**
 * @brief Callback interface to handle errors.
 *
 * When framework has a callback handler installed, the handler may override default framework actions on error
 * situations.
 *
 * @sa setErrorCallback()
 * @ingroup Lib_acsdkProperties
 */
class ErrorCallbackInterface {
public:
    //// Default constructor.
    virtual ~ErrorCallbackInterface() noexcept = default;

    /**
     * @brief Handler of open properties error.
     *
     * This handler is invoked when open properties call encounters an error.
     *
     * @param[in] status    Status code. Handler must be able to handle unknown error codes.
     * @param[in] configUri Configuration URI for the properties container.
     *
     * @return Preferred action to continue.
     *
     * @retval Action::CONTINUE     Execute default action. The framework decides what to do.
     * @retval Action::FAIL         Fails the call. The framework aborts the operation and returns an error code to
     *                              caller.
     * @retval Action::CLEAR_DATA   Signals to framework to clear all container's data and continue normally.
     * @retval Action::RETRY        Signals to framework to retry failed operation.
     */
    virtual Action onOpenPropertiesError(StatusCode status, const std::string& configUri) noexcept = 0;

    /**
     * @brief Handler of get property errors.
     *
     * This handler is invoked when getting string or binary property call encounters an error.
     *
     * @param[in] status    Status code. Handler must be able to handle unknown error codes.
     * @param[in] configUri Configuration URI for the properties container.
     *
     * @return Preferred action to continue.
     * @retval Action::DEFAULT      Execute default action. The framework decides what to do.
     * @retval Action::FAIL         Fails the call. The framework aborts the operation and returns an error code to
     *                              caller.
     * @retval Action::CLEAR_DATA   Signals to framework to clear the property value and continue normally. The caller
     *                              will get an error as a result.
     * @retval Action::RETRY Signals to framework to retry failed operation.
     */
    virtual Action onGetPropertyError(StatusCode status, const std::string& configUri) noexcept = 0;

    /**
     * @brief Handler of put property errors.
     *
     * This handler is invoked when setting string or binary property call encounters an error.
     *
     * @param[in] status    Status code. Handler must be able to handle unknown error codes.
     * @param[in] configUri Configuration URI for the properties container.
     *
     * @return Preferred action to continue.
     * @retval Action::CONTINUE     Execute default action. The framework decides what to do.
     * @retval Action::FAIL         Fails the call. The framework aborts the operation and returns an error code to
     *                              caller.
     * @retval Action::CLEAR_DATA   Signals to framework to clear the property value and continue normally.
     * @retval Action::RETRY        Signals to framework to retry failed operation.
     */
    virtual Action onPutPropertyError(StatusCode status, const std::string& configUri) noexcept = 0;
};

}  // namespace properties
}  // namespace alexaClientSDK

#endif  // ACSDK_PROPERTIES_ERRORCALLBACKINTERFACE_H_
