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

#ifndef ACSDKAUTHORIZATIONINTERFACES_LWA_CBLAUTHORIZATIONOBSERVERINTERFACE_H_
#define ACSDKAUTHORIZATIONINTERFACES_LWA_CBLAUTHORIZATIONOBSERVERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace acsdkAuthorizationInterfaces {
namespace lwa {

/// An observer interface used for callbacks when authorizing using CBL in @c LWAAuthorizationAdapterInterface.
class CBLAuthorizationObserverInterface {
public:
    /// Destructor.
    virtual ~CBLAuthorizationObserverInterface() = default;

    /// An optional struct that may return additional information.
    struct CustomerProfile {
        /// The name associated with the account.
        std::string name;

        /// The email associated with the account.
        std::string email;

        /// Constructor.
        CustomerProfile() = default;

        /**
         * Constructor.
         *
         * @param pName The name.
         * @param pEmail The email.
         */
        CustomerProfile(const std::string& pName, const std::string& pEmail);
    };

    /**
     * A callback that requests the observer to display the following information
     * to the user as part of the CBL process.
     *
     * @param url The url to be displayed.
     * @param code The CBL code.
     */
    virtual void onRequestAuthorization(const std::string& url, const std::string& code) = 0;

    /**
     * Legacy API for notifying the application when token requests are being made.
     *
     * This notification can be noisy, a preferred alternative is for applications to observe
     * @c AuthObserverInterface::State::AUTHORIZING via @c AuthObserverInterface::onAuthStateChange.
     */
    virtual void onCheckingForAuthorization() = 0;

    /**
     * A callback if CustomerProfile is available.
     *
     * @param customerProfile The customerProfile that's returned.
     */
    virtual void onCustomerProfileAvailable(const CustomerProfile& customerProfile) = 0;
};

inline CBLAuthorizationObserverInterface::CustomerProfile::CustomerProfile(
    const std::string& pName,
    const std::string& pEmail) :
        name{pName},
        email{pEmail} {
}

/**
 * Operator overload to compare two CustomerProfile objects.
 *
 * @param lhs The object to compare.
 * @param rhs Another object to compare.
 *
 * @retun Whether the two objects are considered equivalent.
 */
inline bool operator==(
    const CBLAuthorizationObserverInterface::CustomerProfile& lhs,
    const CBLAuthorizationObserverInterface::CustomerProfile& rhs) {
    return lhs.name == rhs.name && lhs.email == rhs.email;
}

}  // namespace lwa
}  // namespace acsdkAuthorizationInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATIONINTERFACES_LWA_CBLAUTHORIZATIONOBSERVERINTERFACE_H_
