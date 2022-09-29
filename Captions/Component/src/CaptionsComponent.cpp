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

#ifdef ENABLE_CAPTIONS
#include <Captions/CaptionManager.h>
#include <Captions/LibwebvttParserAdapter.h>
#include <Captions/TimingAdapterFactory.h>
#endif

#include "Captions/CaptionsComponent.h"

namespace alexaClientSDK {
namespace captions {

using namespace acsdkManufactory;
using namespace acsdkShutdownManagerInterfaces;

/// String to identify log entries originating from this file.
#define TAG "CaptionsComponent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

static std::shared_ptr<CaptionManagerInterface> createCaptionManagerInterface(
    const std::shared_ptr<ShutdownNotifierInterface>& shutdownNotifier) {
    if (!shutdownNotifier) {
        ACSDK_ERROR(LX("createCaptionManagerInterfaceFailed").m("nullShutdownNotifier"));
        return nullptr;
    }

#ifdef ENABLE_CAPTIONS
    ACSDK_DEBUG5(LX(__func__).m("captions enabled"));
    auto webvttParser = LibwebvttParserAdapter::getInstance();
    if (!webvttParser) {
        ACSDK_ERROR(LX("createCaptionManagerInterfaceFailed").m("nullWebvttParser"));
        return nullptr;
    }

    auto captionManager = CaptionManager::create(webvttParser);
    shutdownNotifier->addObserver(captionManager);

    if (!captionManager) {
        ACSDK_ERROR(LX("createCaptionManagerInterfaceFailed"));
        return nullptr;
    }

    return captionManager;
#else
    ACSDK_DEBUG5(LX(__func__).m("captions disabled"));
    return nullptr;
#endif
}

CaptionsComponent getComponent() {
    return ComponentAccumulator<>().addRetainedFactory(createCaptionManagerInterface);
}

}  // namespace captions
}  // namespace alexaClientSDK