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
#include <array>
#include <memory>
#include <utility>

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AuthDelegateMock.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace common {

using namespace std;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;

static const string TAG{"MAPLiteAuthDelegateMock"};
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

shared_ptr<AuthDelegateMock> AuthDelegateMock::create() {
    auto mapLiteAuthDelegate = shared_ptr<AuthDelegateMock>(new AuthDelegateMock());
    return mapLiteAuthDelegate;
}

string AuthDelegateMock::getAuthToken() {
    string token{
            "Atna|EwICIO_"
            "XWrM1cci3ZH9SSe0dudUhslX6PLgTfHHoTM1gHfERnr47HIVXjYMNzbxqb3lzP9tQ2SeTr77BGEAlYm0CTjtY3FN0s7TLXe93SruK68eel"
            "F"
            "WwB7Q0zw_gUE4fXcCZv_f8mYsTJnox_UoFNYFKqdBzE12g8TrNn4rkyPhKQRbrQDSdTQe_"
            "znY9tCt8EnDwxgJR09tPttEPfcsxFoSk8Qi36ptGGoLljVcCBM6Ef37XB9OseRkTQqfmtbTkBW8ikC--EfgLhVLnfceHs653mJ-"
            "oMpTNK2YBOTx-klj5iuWpHa3dq3xgjBpRI-1ocgqcYOk"};
    return token;
}

void AuthDelegateMock::onAuthFailure(const std::string& token) {
    ACSDK_ERROR(LX("onAuthFailure").sensitive("token", token));
}
void AuthDelegateMock::addAuthObserver(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface> observer) {
    ACSDK_INFO(LX(("addAuthObserver")));
}

void AuthDelegateMock::removeAuthObserver(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface> observer) {
    ACSDK_INFO(LX(("removeAuthObserver")));
}

}  // namespace common
}  // namespace acsdkAssets
}  // namespace alexaClientSDK