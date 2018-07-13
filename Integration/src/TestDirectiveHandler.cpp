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

#include "Integration/TestDirectiveHandler.h"

namespace alexaClientSDK {
namespace integration {
namespace test {

TestDirectiveHandler::TestDirectiveHandler(avsCommon::avs::DirectiveHandlerConfiguration config) :
        m_configuration{config} {
}

void TestDirectiveHandler::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = DirectiveParams::Type::HANDLE_IMMEDIATELY;
    dp.directive = directive;
    m_queue.push_back(dp);
    m_wakeTrigger.notify_all();
}

void TestDirectiveHandler::preHandleDirective(
    std::shared_ptr<avsCommon::avs::AVSDirective> directive,
    std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = TestDirectiveHandler::DirectiveParams::Type::PREHANDLE;
    dp.directive = directive;
    dp.result = std::move(result);
    m_results[directive->getMessageId()] = dp.result;
    m_directives[directive->getMessageId()] = directive;
    m_queue.push_back(dp);
    m_wakeTrigger.notify_all();
}

bool TestDirectiveHandler::handleDirective(const std::string& messageId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = DirectiveParams::Type::HANDLE;
    auto result = m_results.find(messageId);
    if (m_results.end() == result) {
        return false;
    }
    dp.result = result->second;
    auto directive = m_directives.find(messageId);
    if (m_directives.end() == directive) {
        return false;
    }
    dp.directive = directive->second;
    m_queue.push_back(dp);
    m_wakeTrigger.notify_all();
    return true;
}

void TestDirectiveHandler::cancelDirective(const std::string& messageId) {
    std::unique_lock<std::mutex> lock(m_mutex);
    TestDirectiveHandler::DirectiveParams dp;
    dp.type = DirectiveParams::Type::CANCEL;
    auto result = m_results.find(messageId);
    dp.result = result->second;
    m_results.erase(result);
    auto directive = m_directives.find(messageId);
    dp.directive = directive->second;
    m_directives.erase(directive);
    m_queue.push_back(dp);
    m_wakeTrigger.notify_all();
}

avsCommon::avs::DirectiveHandlerConfiguration TestDirectiveHandler::getConfiguration() const {
    return m_configuration;
}

void TestDirectiveHandler::onDeregistered() {
}

TestDirectiveHandler::DirectiveParams TestDirectiveHandler::waitForNext(const std::chrono::seconds duration) {
    DirectiveParams ret;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_wakeTrigger.wait_for(lock, duration, [this]() { return !m_queue.empty(); })) {
        ret.type = DirectiveParams::Type::TIMEOUT;
        return ret;
    }
    ret = m_queue.front();
    m_queue.pop_front();
    return ret;
}

TestDirectiveHandler::DirectiveParams::DirectiveParams() : type{Type::UNSET} {
}
}  // namespace test
}  // namespace integration
}  // namespace alexaClientSDK
