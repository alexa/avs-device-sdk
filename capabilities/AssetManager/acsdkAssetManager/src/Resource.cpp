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

#include "Resource.h"

#include <AVSCommon/Utils/FileSystem/FileSystemUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <fstream>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace manager {

using namespace std;
using namespace rapidjson;
using namespace alexaClientSDK::avsCommon::utils;

static const char* METADATA_FILE_NAME = "metadata.json";
static const char* RESOURCE_NAME = "name";
static const char* RESOURCE_ID = "id";
static const char* RESOURCE_SIZE = "size";

/// String to identify log entries originating from this file.
static const std::string TAG{"Resource"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<Resource> Resource::createFromConfigFile(const std::string& resourceDirectory) {
    Document document;
    auto metaDataFilePath = resourceDirectory + "/" + METADATA_FILE_NAME;
    ifstream ifs(metaDataFilePath);
    IStreamWrapper is(ifs);

    if (document.ParseStream(is).HasParseError()) {
        ACSDK_ERROR(
                LX("createFromConfigFile").m("Error parsing the metadata file").d("file", metaDataFilePath.c_str()));
        return nullptr;
    }

    if (!document.HasMember(RESOURCE_NAME)) {
        ACSDK_ERROR(LX("createFromConfigFile").d("Missing member", RESOURCE_NAME));
        return nullptr;
    }
    if (!document.HasMember(RESOURCE_ID)) {
        ACSDK_ERROR(LX("createFromConfigFile").d("Missing member", RESOURCE_ID));
        return nullptr;
    }
    if (!document.HasMember(RESOURCE_SIZE)) {
        ACSDK_ERROR(LX("createFromConfigFile").d("Missing member", RESOURCE_SIZE));
        return nullptr;
    }

    string name = document[RESOURCE_NAME].GetString();
    if (name.empty()) {
        ACSDK_ERROR(LX("createFromConfigFile").d("Empty member", RESOURCE_NAME));
        return nullptr;
    }
    string id = document[RESOURCE_ID].GetString();
    if (id.empty()) {
        ACSDK_ERROR(LX("createFromConfigFile").d("Empty member", RESOURCE_ID));
        return nullptr;
    }
    auto& sizeMember = document[RESOURCE_SIZE];
    if (!sizeMember.IsUint64()) {
        ACSDK_ERROR(LX("createFromConfigFile").d("Invalid member %s", RESOURCE_SIZE));
        return nullptr;
    }

    return shared_ptr<Resource>(
            new Resource(resourceDirectory, move(name), move(id), static_cast<size_t>(sizeMember.GetUint64())));
}

std::shared_ptr<Resource> Resource::createFromStorage(const std::string& resourceDirectory) {
    auto resource = createFromConfigFile(resourceDirectory);
    if (resource != nullptr) {
        ACSDK_DEBUG(LX("createFromStorage")
                            .d("Loaded resource", resource->m_resourceName)
                            .d("metadataFile", resourceDirectory.c_str()));
        return resource;
    }

    ACSDK_WARN(LX("createFromStorage")
                       .m("Could not load artifact metadata, will try to generate metadata from ")
                       .d("file", resourceDirectory));
    auto id = filesystem::basenameOf(resourceDirectory);
    if (id.empty()) {
        ACSDK_ERROR(LX("createFromStorage")
                            .m("Failed to get the resource id from directory")
                            .d("directory", resourceDirectory));
        return nullptr;
    }

    auto fileList = filesystem::list(resourceDirectory);
    fileList.erase(remove(fileList.begin(), fileList.end(), METADATA_FILE_NAME), fileList.end());
    if (fileList.size() != 1) {
        ACSDK_ERROR(LX("createFromStorage")
                            .m("Failed to get resource name from directory")
                            .d("directory", resourceDirectory));
        ACSDK_ERROR(LX("createFromStorage")
                            .m("Found wrong number of files, expecting only 1")
                            .d("files found", fileList.size()));
        return nullptr;
    }

    auto name = fileList[0];
    auto size = filesystem::sizeOf(resourceDirectory + "/" + name);
    if (size == 0) {
        ACSDK_ERROR(LX("createFromStorage")
                            .m("Failed to get the resource size from directory")
                            .d("directory", resourceDirectory));
        return nullptr;
    }

    ACSDK_DEBUG(LX("createFromStorage")
                        .m("Loaded and generated resource info, caching in metadata file")
                        .d("loadedFile", resourceDirectory));
    resource = shared_ptr<Resource>(new Resource(resourceDirectory, name, id, size));
    resource->saveMetadata();
    return resource;
}

std::shared_ptr<Resource> Resource::create(
        const std::string& parentDirectory,
        const std::string& id,
        const std::string& source) {
    if (!filesystem::makeDirectory(parentDirectory)) {
        ACSDK_ERROR(LX("create").m("Could not create parent directory").d("directory", parentDirectory));
        return nullptr;
    }
    if (id.empty()) {
        ACSDK_ERROR(LX("create").m("Empty id for artifact").d("artifact", source));
        return nullptr;
    }
    if (!filesystem::exists(source)) {
        ACSDK_ERROR(LX("create").m("Source file does not exists").d("artifact", source));
        return nullptr;
    }

    auto resourceHome = parentDirectory + "/" + id;
    auto filename = filesystem::basenameOf(source);

    if (!filesystem::makeDirectory(resourceHome)) {
        ACSDK_ERROR(LX("create").m("Failed to create resource directory").d("directory", resourceHome));
        return nullptr;
    }

    if (!filesystem::move(source, resourceHome + "/" + filename)) {
        ACSDK_ERROR(LX("create").m("Failed to move file").d("file", source).d("directory", resourceHome));
        return nullptr;
    }

    auto size = filesystem::sizeOf(resourceHome);
    auto resource = shared_ptr<Resource>(new Resource(resourceHome, filename, id, size));
    if (!resource->saveMetadata()) {
        ACSDK_ERROR(
                LX("create").m("Could not save metadata information, will try to generate this dynamically on next "
                               "restart"));
    }

    ACSDK_INFO(LX("create").d("id", resource->getId().c_str()).d("path", resource->getPath().c_str()));
    return resource;
}

Resource::Resource(const string& resourceDirectory, const string& resourceName, const string& id, size_t sizeBytes) :
        m_resourceDirectory(resourceDirectory),
        m_resourceName(resourceName),
        m_id(id),
        m_sizeBytes(sizeBytes),
        m_fullResourcePath(this->m_resourceDirectory + "/" + this->m_resourceName),
        m_refCount(0) {
}

bool Resource::saveMetadata() {
    const auto outputPath = m_resourceDirectory + "/" + METADATA_FILE_NAME;
    Document document(kObjectType);
    auto& allocator = document.GetAllocator();

    document.AddMember(StringRef(RESOURCE_ID), StringRef(m_id), allocator);
    document.AddMember(StringRef(RESOURCE_SIZE), static_cast<uint64_t>(m_sizeBytes), allocator);
    document.AddMember(StringRef(RESOURCE_NAME), StringRef(m_resourceName), allocator);

    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    document.Accept(writer);

    ofstream fileHandle;
    fileHandle.open(outputPath, ios::out);
    if (!fileHandle.is_open()) {
        ACSDK_ERROR(LX("saveMetadata").m("Failed to open metadata file"));
        return false;
    }
    fileHandle << buffer.GetString();
    fileHandle.close();
    if (!fileHandle.good()) {
        ACSDK_ERROR(LX("saveMetadata").m("Failed to close the file").d("file", outputPath.c_str()));
        return false;
    }
    return true;
}

void Resource::erase() {
    filesystem::removeAll(m_resourceDirectory);
    m_fullResourcePath.clear();
    m_sizeBytes = 0;
}

}  // namespace manager
}  // namespace acsdkAssets
}  // namespace alexaClientSDK