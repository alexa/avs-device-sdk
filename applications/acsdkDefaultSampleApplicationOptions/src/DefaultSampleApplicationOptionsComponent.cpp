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

#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkCore/CoreComponent.h>
#include <acsdkShared/SharedComponent.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>

#include "acsdkDefaultSampleApplicationOptions/DefaultSampleApplicationOptionsComponent.h"
#include "acsdkDefaultSampleApplicationOptions/NullMetricRecorder.h"

namespace alexaClientSDK {
namespace acsdkDefaultSampleApplicationOptions {

using namespace acsdkManufactory;
using namespace authorization::cblAuthDelegate;

Component<
    acsdkManufactory::Import<std::shared_ptr<authorization::cblAuthDelegate::CBLAuthRequesterInterface>>,
    acsdkManufactory::Import<std::unique_ptr<avsCommon::utils::libcurlUtils::HttpPostInterface>>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::utils::logger::Logger>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>
getComponent() {
    return ComponentAccumulator<>()
        .addComponent(acsdkShared::getComponent())
        .addComponent(acsdkCore::getComponent())
        .addRetainedFactory(CBLAuthDelegate::createAuthDelegateInterface)
        .addRetainedFactory(NullMetricRecorder::createMetricRecorderInterface)
        .addRetainedFactory(SQLiteCBLAuthDelegateStorage::createCBLAuthDelegateStorageInterface)
#ifdef ANDROID_LOGGER
        .addPrimaryFactory(applicationUtilities::androidUtilities::AndroidLogger::getAndroidLogger)
#else
        .addPrimaryFactory(avsCommon::utils::logger::getConsoleLogger)
#endif
        ;
}

}  // namespace acsdkDefaultSampleApplicationOptions
}  // namespace alexaClientSDK
