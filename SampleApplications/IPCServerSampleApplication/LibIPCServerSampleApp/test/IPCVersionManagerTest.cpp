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

#include <gtest/gtest.h>
#include "rapidjson/document.h"

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include "IPCServerSampleApp/IPC/IPCVersionManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace test {

using namespace ::testing;
using namespace rapidjson;
using namespace avsCommon::utils::json;
using namespace ipc;

/// The SessionSetup string
static const std::string SESSION_SETUP("SessionSetup");

/// The FocusManager string
static const std::string FOCUS_MANAGER("FocusManager");

/// The Controller string
static const std::string CONTROLLER("Controller");

class IPCVersionManagerTest : public ::testing::Test {
public:
    void SetUp() override;

protected:
    std::shared_ptr<IPCVersionManager> m_ipcVersionManager;
};

void IPCVersionManagerTest::SetUp() {
    m_ipcVersionManager = std::make_shared<IPCVersionManager>();
}

/**
 * Test version equality
 */
TEST_F(IPCVersionManagerTest, test_serverClientVersionEqual) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 1);

    ASSERT_TRUE(m_ipcVersionManager->validateVersionForNamespace(SESSION_SETUP, 1));
    ASSERT_TRUE(m_ipcVersionManager->validateVersionForNamespace(FOCUS_MANAGER, 1));
}

/**
 * Test server version higher than client version
 */
TEST_F(IPCVersionManagerTest, test_highServerVersion) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 2);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 2);

    ASSERT_FALSE(m_ipcVersionManager->validateVersionForNamespace(SESSION_SETUP, 1));
    ASSERT_FALSE(m_ipcVersionManager->validateVersionForNamespace(FOCUS_MANAGER, 1));
}

/**
 * Test client version higher than server version
 */
TEST_F(IPCVersionManagerTest, test_highClientVersion) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 1);

    ASSERT_FALSE(m_ipcVersionManager->validateVersionForNamespace(SESSION_SETUP, 2));
    ASSERT_FALSE(m_ipcVersionManager->validateVersionForNamespace(FOCUS_MANAGER, 2));
}

/**
 * Test mixed server client mismatch
 */
TEST_F(IPCVersionManagerTest, test_mixedMismatch) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 2);

    ASSERT_FALSE(m_ipcVersionManager->validateVersionForNamespace(SESSION_SETUP, 2));
    ASSERT_FALSE(m_ipcVersionManager->validateVersionForNamespace(FOCUS_MANAGER, 1));
}

/**
 * Test server client mismatch document
 */
TEST_F(IPCVersionManagerTest, test_mixedMismatchDocument) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 2);
    m_ipcVersionManager->registerNamespaceVersionEntry(CONTROLLER, 2);
    std::string json = R"(
        {"entries":[{"namespace":"SessionSetup","version":2},{"namespace":"FocusManager","version":0},
        {"namespace":"Controller","version":1}]})";

    Document document;
    jsonUtils::parseJSON(json, &document);
    ASSERT_FALSE(m_ipcVersionManager->handleAssertNamespaceVersions(document));
    ASSERT_FALSE(m_ipcVersionManager->handleAssertNamespaceVersionsFromString(json));
}

/**
 * Test server client mismatch document when a namespace version entry in the middle is mismatched
 */
TEST_F(IPCVersionManagerTest, test_mixedMiddleEntryMismatchDocument) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 2);
    m_ipcVersionManager->registerNamespaceVersionEntry(CONTROLLER, 2);
    std::string json = R"(
        {"entries":[{"namespace":"SessionSetup","version":1},{"namespace":"FocusManager","version":2},
        {"namespace":"Controller","version":1}]})";

    Document document;
    jsonUtils::parseJSON(json, &document);
    ASSERT_FALSE(m_ipcVersionManager->handleAssertNamespaceVersions(document));
    ASSERT_FALSE(m_ipcVersionManager->handleAssertNamespaceVersionsFromString(json));
}

/**
 * Test server has namespace registered but client does not
 */
TEST_F(IPCVersionManagerTest, test_registeredNamespaceOnServerNotClient) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(FOCUS_MANAGER, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(CONTROLLER, 1);
    std::string json = R"(
        {"entries":[{"namespace":"SessionSetup","version":1},
        {"namespace":"Controller","version":1}]})";

    Document document;
    jsonUtils::parseJSON(json, &document);
    ASSERT_TRUE(m_ipcVersionManager->handleAssertNamespaceVersions(document));
    ASSERT_TRUE(m_ipcVersionManager->handleAssertNamespaceVersionsFromString(json));
}

/**
 * Test client has namespace registered but server does not
 */
TEST_F(IPCVersionManagerTest, test_registeredNamespaceOnClientNotServer) {
    m_ipcVersionManager->registerNamespaceVersionEntry(SESSION_SETUP, 1);
    m_ipcVersionManager->registerNamespaceVersionEntry(CONTROLLER, 1);
    std::string json = R"(
        {"entries":[{"namespace":"SessionSetup","version":1},{"namespace":"FocusManager","version":1},
        {"namespace":"Controller","version":1}]})";

    Document document;
    jsonUtils::parseJSON(json, &document);
    ASSERT_FALSE(m_ipcVersionManager->handleAssertNamespaceVersions(document));
    ASSERT_FALSE(m_ipcVersionManager->handleAssertNamespaceVersionsFromString(json));
}

}  // namespace test
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
