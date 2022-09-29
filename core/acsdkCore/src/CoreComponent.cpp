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
#include <acsdkPostConnectOperationProviderRegistrar/PostConnectOperationProviderRegistrar.h>
#include <acsdkShared/SharedComponent.h>
#include <acsdkSystemClockMonitor/SystemClockNotifier.h>
#include <acsdkSystemClockMonitor/SystemClockMonitor.h>
#include <ADSL/ADSLComponent.h>
#include <AFML/FocusManagementComponent.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <Alexa/AlexaEventProcessedNotifier.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <Alexa/AlexaInterfaceCapabilityAgent.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <ContextManager/ContextManager.h>
#include <Endpoints/DefaultEndpointBuilder.h>
#include <InterruptModel/InterruptModel.h>
#include <RegistrationManager/RegistrationManagerComponent.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include "acsdkCore/CoreComponent.h"

namespace alexaClientSDK {
namespace acsdkCore {

using namespace ::alexaClientSDK::storage;
using namespace ::alexaClientSDK::storage::sqliteStorage;
using namespace afml;
using namespace alexaClientSDK::endpoints;
using namespace acsdkAlexaEventProcessedNotifierInterfaces;
using namespace acsdkManufactory;
using namespace acsdkPostConnectOperationProviderRegistrar;
using namespace acsdkPostConnectOperationProviderRegistrarInterfaces;
using namespace acsdkSystemClockMonitor;
using namespace acsdkSystemClockMonitorInterfaces;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::endpoints;
using namespace avsCommon::utils;
using namespace avsGatewayManager;
using namespace avsGatewayManager::storage;
using namespace capabilityAgents::alexa;
using namespace capabilitiesDelegate;
using namespace capabilitiesDelegate::storage;
using namespace contextManager;
using namespace interruptModel;
using namespace registrationManager;
using namespace synchronizeStateSender;

/// String to identify log entries originating from this file.
#define TAG "CoreComponent"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

CoreComponent getComponent() {
    return ComponentAccumulator<>()
        .addComponent(acsdkShared::getComponent())
        .addComponent(adsl::getComponent())
        .addComponent(afml::getComponent())
        .addRetainedFactory(AlexaEventProcessedNotifier::createAlexaEventProcessedNotifierInterface)
        .addRetainedFactory(AlexaInterfaceMessageSender::createAlexaInterfaceMessageSender)
        .addRetainedFactory(AlexaInterfaceMessageSender::createAlexaInterfaceMessageSenderInternalInterface)
        .addRequiredFactory(AlexaInterfaceCapabilityAgent::createDefaultAlexaInterfaceCapabilityAgent)
        .addRetainedFactory(AttachmentManager::createInProcessAttachmentManagerInterface)
        .addRequiredFactory(AVSGatewayManager::createAVSGatewayManagerInterface)
        .addUniqueFactory(AVSGatewayManagerStorage::createAVSGatewayManagerStorageInterface)
        .addRetainedFactory(avsCommon::avs::ExceptionEncounteredSender::createExceptionEncounteredSenderInterface)
        .addRequiredFactory(CapabilitiesDelegate::createCapabilitiesDelegateInterface)
        .addRetainedFactory(DefaultEndpointBuilder::createDefaultEndpointBuilderInterface)
        .addRetainedFactory(DefaultEndpointBuilder::createDefaultEndpointCapabilitiesRegistrarInterface)
        .addRetainedFactory(ContextManager::createContextManagerInterface)
        .addRetainedFactory(DeviceInfo::createFromConfiguration)
        .addRetainedFactory(InterruptModel::createInterruptModel)
        .addRetainedFactory(PostConnectOperationProviderRegistrar::createPostConnectOperationProviderRegistrarInterface)
        .addComponent(registrationManager::getComponent())
        .addRetainedFactory(SQLiteMiscStorage::createMiscStorageInterface)
        .addUniqueFactory(SQLiteCapabilitiesDelegateStorage::createCapabilitiesDelegateStorageInterface)
        .addRequiredFactory(SynchronizeStateSenderFactory::createPostConnectOperationProviderInterface)
        .addRetainedFactory(SystemClockMonitor::createSystemClockMonitorInterface)
        .addRetainedFactory(SystemClockNotifier::createSystemClockNotifierInterface);
}

}  // namespace acsdkCore
}  // namespace alexaClientSDK
