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
#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCVERSIONMANAGER_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCVERSIONMANAGER_H_

#include <rapidjson/document.h>
#include <list>
#include <unordered_map>

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace ipc {

class IPCVersionManager {
public:
    IPCVersionManager();

    virtual ~IPCVersionManager() = default;

    /**
     * Validates the Namespace Versions
     *
     * @param payload The rapidjson payload containing the namespace-version entries.
     * @return true if versions are the same, false if there is a version mismatch.
     */
    bool handleAssertNamespaceVersions(const rapidjson::Document& payload);

    /**
     * Validates the Namespace Version using a serialized payload
     *
     * @param payload The serialized payload containing the namespace-version entries.
     * @return true if versions are the same, false if there is a version mismatch.
     */
    bool handleAssertNamespaceVersionsFromString(const std::string& payload);

    /**
     * Validates an individual namespace-version entry
     *
     * @param ns The namespace to validate the version of
     * @param clientNamespaceVersion The IPC client version to validate against
     * @return true if versions are the same, false if there is a version mismatch
     */
    bool validateVersionForNamespace(const std::string& ns, int clientNamespaceVersion);

    /**
     * Adds a namespace-version entry to the VersionManager
     *
     * @param ns The namespace to add
     * @param version The version to add
     */
    void registerNamespaceVersionEntry(const std::string& ns, int version);

    /**
     * Removes a namespace-version entry from the VersionManager
     *
     * @param ns Namespace of the namespace-version entry to remove
     */
    void deregisterNamespaceVersionEntry(const std::string& ns);

private:
    /**
     * Extracts namespace-version entries from rapidjson::Document
     *
     * @param message The rapidjson document containing the namespace-version entries.
     * @return namespace-version entries
     */
    static std::unordered_map<std::string, int> buildMapFromEntries(const rapidjson::Document& message);

    /// The namespace-version map
    std::unordered_map<std::string, int> m_namespaceVersionMap;
};

}  // namespace ipc
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_IPC_IPCVERSIONMANAGER_H_
