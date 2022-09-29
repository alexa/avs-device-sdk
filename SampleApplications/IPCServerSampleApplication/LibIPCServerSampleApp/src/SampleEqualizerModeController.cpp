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

#include <memory>
#include <sstream>

#include "IPCServerSampleApp/ConsolePrinter.h"

#include "IPCServerSampleApp/SampleEqualizerModeController.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace acsdkEqualizerInterfaces;

std::shared_ptr<SampleEqualizerModeController> SampleEqualizerModeController::create() {
    return std::shared_ptr<SampleEqualizerModeController>(new SampleEqualizerModeController());
}

bool SampleEqualizerModeController::setEqualizerMode(EqualizerMode mode) {
    ConsolePrinter::prettyPrint("Equalizer Mode changed to '" + equalizerModeToString(mode) + "'");
    return true;
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
