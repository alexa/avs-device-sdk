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
#include <string>

#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include "IPCServerSampleApp/IPC/IPCVersionManager.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

using namespace avsCommon::utils::json;

/// The version string
static const std::string VERSION = "version";

/// The entries string
static const std::string ENTRIES = "entries";

/// The namespace string
static const std::string NAMESPACE = "namespace";

/// String to identify log entries originating from this file.
static const std::string TAG("IPCVersionManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

IPCVersionManager::IPCVersionManager() : m_namespaceVersionMap{} {
}

bool IPCVersionManager::validateVersionForNamespace(const std::string& ns, int clientNamespaceVersion) {
    if (m_namespaceVersionMap.count(ns) == 0) {
        ACSDK_ERROR(LX(__func__).d("reason", "Namespace not registered by server").d("namespace", ns));
        return false;
    }
    ACSDK_DEBUG9(LX(__func__)
                     .d("Namespace", ns)
                     .d("Server Version", m_namespaceVersionMap[ns])
                     .d("Client version", clientNamespaceVersion));

    if (m_namespaceVersionMap[ns] > clientNamespaceVersion) {
        ACSDK_ERROR(LX(__func__).m("Namespace version mismatch. Update Client"));
    } else if (m_namespaceVersionMap[ns] < clientNamespaceVersion) {
        ACSDK_ERROR(LX(__func__).m("Namespace version mismatch. Update Server"));
    }
    return m_namespaceVersionMap[ns] == clientNamespaceVersion;
}

bool IPCVersionManager::handleAssertNamespaceVersions(const rapidjson::Document& payload) {
    ACSDK_DEBUG9(LX(__func__));
    if (!payload.HasMember(ENTRIES)) {
        ACSDK_ERROR(LX(__func__).d("reason", "entries not present in the message"));
        return false;
    }

    std::unordered_map<std::string, int> ipcMessageMap = buildMapFromEntries(payload);

    for (auto const& mapEntry : ipcMessageMap) {
        std::string ns = mapEntry.first;
        int version = mapEntry.second;
        if (!validateVersionForNamespace(ns, version)) {
            return false;
        }
    }
    return true;
}

bool IPCVersionManager::handleAssertNamespaceVersionsFromString(const std::string& payload) {
    ACSDK_DEBUG9(LX(__func__));
    rapidjson::Document document;
    if (!jsonUtils::parseJSON(payload, &document)) {
        ACSDK_ERROR(LX(__func__).d("reason", "invalid payload string"));
        return false;
    }

    return handleAssertNamespaceVersions(document);
}

void IPCVersionManager::registerNamespaceVersionEntry(const std::string& ns, int version) {
    m_namespaceVersionMap.insert({ns, version});
}

void IPCVersionManager::deregisterNamespaceVersionEntry(const std::string& ns) {
    m_namespaceVersionMap.erase(ns);
}

std::unordered_map<std::string, int> IPCVersionManager::buildMapFromEntries(const rapidjson::Document& message) {
    std::unordered_map<std::string, int> ipcMessageMap = {};
    auto& entries = message[ENTRIES];
    for (rapidjson::Value::ConstValueIterator itr = entries.Begin(); itr != entries.End(); ++itr) {
        const rapidjson::Value& attribute = *itr;
        std::string ns;
        if (!avsCommon::utils::json::jsonUtils::retrieveValue(attribute, NAMESPACE, &ns)) {
            ACSDK_WARN(LX("buildMapFromEntriesError").d("reason", "namespace not found"));
            return {};
        }
        ipcMessageMap[ns] = attribute[VERSION].GetInt();
    }
    return ipcMessageMap;
}

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
