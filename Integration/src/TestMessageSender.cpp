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

#include "ACL/AVSConnectionManager.h"
#include "Integration/TestMessageSender.h"

using namespace alexaClientSDK;
using namespace acl;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

namespace alexaClientSDK {
namespace integration {
namespace test {

TestMessageSender::TestMessageSender(
    std::shared_ptr<acl::MessageRouterInterface> messageRouter,
    bool isEnabled,
    std::shared_ptr<ConnectionStatusObserverInterface> connectionStatusObserver,
    std::shared_ptr<MessageObserverInterface> messageObserver) :
        RequiresShutdown{"TestMessageSender"} {
    m_connectionManager =
        acl::AVSConnectionManager::create(messageRouter, isEnabled, {connectionStatusObserver}, {messageObserver});
}

void TestMessageSender::sendMessage(std::shared_ptr<MessageRequest> request) {
    m_connectionManager->sendMessage(request);
    SendParams sendState;
    std::unique_lock<std::mutex> lock(m_mutex);
    sendState.type = SendParams::Type::SEND;
    sendState.request = request;
    m_queue.push_back(sendState);
    m_wakeTrigger.notify_all();
}

TestMessageSender::SendParams TestMessageSender::waitForNext(const std::chrono::seconds duration) {
    SendParams ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        ret.type = SendParams::Type::TIMEOUT;
        return ret;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

void TestMessageSender::enable() {
    m_connectionManager->enable();
}

void TestMessageSender::disable() {
    m_connectionManager->disable();
}

bool TestMessageSender::isEnabled() {
    return m_connectionManager->isEnabled();
}

void TestMessageSender::reconnect() {
    m_connectionManager->reconnect();
}

void TestMessageSender::setAVSEndpoint(const std::string& avsEndpoint) {
    m_connectionManager->setAVSEndpoint(avsEndpoint);
}

void TestMessageSender::addConnectionStatusObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    m_connectionManager->addConnectionStatusObserver(observer);
}

void TestMessageSender::removeConnectionStatusObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer) {
    m_connectionManager->removeConnectionStatusObserver(observer);
}

void TestMessageSender::addMessageObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    m_connectionManager->addMessageObserver(observer);
}

void TestMessageSender::doShutdown() {
    m_connectionManager->shutdown();
}

void TestMessageSender::removeMessageObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::MessageObserverInterface> observer) {
    m_connectionManager->removeMessageObserver(observer);
}

std::shared_ptr<acl::AVSConnectionManager> TestMessageSender::getConnectionManager() const {
    return m_connectionManager;
}

}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
