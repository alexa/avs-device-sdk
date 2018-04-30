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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SOFTWAREINFOSENDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SOFTWAREINFOSENDER_H_

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <ACL/AVSConnectionManager.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SoftwareInfoSenderObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

class SoftwareInfoSendRequest;

/**
 * @c SoftwareInfoSender is a @c CapabilityAgent that handles the @c System.ReportSoftwareInfo directive and
 * the sending of @c System.SoftwareInfo events to AVS.
 *
 * @see https://developer.amazon.com/docs/alexa-voice-service/system.html#reportsoftwareinfo-directive
 * @see https://developer.amazon.com/docs/alexa-voice-service/system.html#softwareinfo-event
 */
class SoftwareInfoSender
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<SoftwareInfoSender>
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface
        , public avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface {
public:
    /**
     * Creates a new @c SoftwareInfoSender instance.
     *
     * @param firmwareVersion The firmware version to include in SoftwareInfo events.
     * @param sendSoftwareInfoUponConnect Whether the SoftwareInfo event should be sent upon connecting.
     * @param observer An object to receive notifications from this SoftwareInfoSender.
     * @param connection Our connection to AVS.
     * @param messageSender Object for sending messages to AVS.
     * @param exceptionEncounteredSender The object to use for sending ExceptionEncountered messages.
     * @return An instance of SoftwareInfoSender if successful else nullptr.
     */
    static std::shared_ptr<SoftwareInfoSender> create(
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
        bool sendSoftwareInfoUponConnect,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> observer,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connection,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Specify the firmwareVersion to send to @c AVS via @c SoftwareInfo events.
     *
     * If the firmware version changes while the client is already connected to @c AVS, it should call
     * @c setFirmwareVersion() immediately with the new version.  If the version is new it will trigger
     * sending a @c SoftwareInfo event to AVS.
     *
     * @param firmwareVersion The firmware version to report to AVS.
     * @return Whether the new firmware version was accepted.
     */
    bool setFirmwareVersion(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion);

    /// @name DirectiveHandlerInterface and CapabilityAgent Functions
    /// @{
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /// @name ConnectionStatusObserverInterface Functions
    /// @{
    void onConnectionStatusChanged(
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status status,
        avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason) override;
    /// @}

    /// @name SoftwareInfoSenderObserverInterface Functions
    /// @{
    void onFirmwareVersionAccepted(avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param firmwareVersion The firmware version to include in SoftwareInfo events.
     * @param sendSoftwareInfoUponConnect Whether the SoftwareInfo event should be sent upon connecting.
     * @param observer An object to receive notifications from this SoftwareInfoSender.
     * @param connection Our connection to AVS.
     * @param messageSender Object for sending messages to AVS.
     * @param exceptionEncounteredSender The object to use for sending ExceptionEncountered messages.
     */
    SoftwareInfoSender(
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
        bool sendSoftwareInfoUponConnect,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> observer,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connection,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender);

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /// The firmware Version to send by SoftwareInfo event.
    avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion m_firmwareVersion;

    /// Whether to send the SoftwareInfo event upon @c CONNECTED.  Access to this member is serialized by @c m_mutex.
    bool m_sendSoftwareInfoUponConnect;

    /// Object to receive notifications from this SoftwareInfoSender.
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> m_observer;

    /// Our connection to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> m_connection;

    /// Object for sending messages to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// Object to send ExceptionEncountered to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> m_exceptionEncounteredSender;

    /// Mutex for serializing access to data members.
    std::mutex m_mutex;

    /// The last reported connection status. Access to this member is serialized by @c m_mutex.
    avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::Status m_connectionStatus;

    /// @c SoftwareInfoSendRequest created in response to reaching the @c CONNECTED state or in
    /// response to a call to @c setFirmwareVersion().  Access to this member is serialized by @c m_mutex.
    std::shared_ptr<capabilityAgents::system::SoftwareInfoSendRequest> m_clientInitiatedSendRequest;

    /// @c SoftwareInfoSendRequest created in response to receiving a System.ReportSoftwareInfo directive. Access to
    /// this member is serialized by @c m_mutex.
    std::shared_ptr<capabilityAgents::system::SoftwareInfoSendRequest> m_handleDirectiveSendRequest;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SOFTWAREINFOSENDER_H_
