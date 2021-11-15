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

#ifndef ACSDKASSETMANAGERCLIENT_GENERICINVENTORY_H_
#define ACSDKASSETMANAGERCLIENT_GENERICINVENTORY_H_

#include <set>
#include <unordered_map>
#include <unordered_set>

#include "acsdkAssetManagerClient/ArtifactWrapper.h"
#include "acsdkAssetManagerClientInterfaces/ArtifactWrapperFactoryInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

using Setting = std::set<std::string>;
using SettingsMap = std::unordered_map<std::string, Setting>;

/**
 * A general inventory manager class that is responsible for managing a list of artifacts and maintaining an active
 * artifact. This manager will respond to setting changes by downloading an artifact if one does not exist for the
 * required settings or by preparing one that already exists on the system. This manager will determine the artifact
 * identity based on the request created.
 */
class GenericInventory
        : public std::enable_shared_from_this<GenericInventory>
        , public clientInterfaces::ArtifactUpdateValidator {
public:
    ~GenericInventory() override;

    inline const char* getName() const {
        return m_name.data();
    }

    /**
     * Informs the manager class before applying the settings of the new settings that are going to be active.
     * @note This should be called before commit to allow the manager to download the appropriate artifact if needed.
     *       The caller needs to check the returned artifact to ensure that it is available before committing.
     *
     * @param newSettings that will be used to identify the request for the new artifact.
     * @return NULLABLE, a pointer to the artifact that is represented by the new settings which can be used to query
     * its state.
     */
    std::shared_ptr<clientInterfaces::ArtifactWrapperInterface> prepareForSettingChange(const SettingsMap& newSettings);

    /**
     * Will apply the changes for the new settings after preparations have been completed.
     * @warning This must only be called after prepareForSettingChange has been called and the returned artifact has
     * been confirmed. Failing to do so will prevent the settings from being applied.
     *
     * @return weather the commit was successful.
     */
    bool commitChange();

    /**
     * Will cancel the changes for the new settings that were requested. This cancels any pending download if requested.
     */
    void cancelChange();

    /**
     * @return the path of the current active artifact on disk.
     */
    std::string getArtifactPath();

    /**
     * Overrides the current active artifact priority if one exists.
     */
    void setCurrentActivePriority(commonInterfaces::Priority priority);

    /**
     * Checks to see if an artifact for the provided setting is already available.
     */
    bool isSettingReady(const SettingsMap& setting);

protected:
    /**
     * Constructor
     *
     * @param name - name for the generic inventory
     * @param artifactWrapperFactory - factory for creating artifact
     */
    GenericInventory(
            std::string name,
            std::shared_ptr<clientInterfaces::ArtifactWrapperFactoryInterface> artifactWrapperFactory);

    /**
     * A method that will be responsible for creating an Artifact Request based on the provided settings.
     *
     * @param settings used to create the request.
     * @return NULLABLE, new request based on the provided settings.
     */
    virtual std::shared_ptr<commonInterfaces::ArtifactRequest> createRequest(const SettingsMap& settings) = 0;

    /**
     * A method that attempts to make use of the new artifact to ensure that it is usable.
     *
     * @param path to the new artifact that just got downloaded or updated.
     * @return true if the artifact is valid and should be used, false otherwise.
     */
    virtual bool applyChangesLocked(const std::string& path) = 0;

    /**
     * Internal call for cancelChange.
     */
    void cancelChangeLocked();

    /// @name ArtifactUpdateValidator method
    inline bool validateUpdate(
            const std::shared_ptr<commonInterfaces::ArtifactRequest>& request,
            const std::string& newPath) override {
        return m_activeArtifact != nullptr && m_activeArtifact->getRequest() == request && applyChangesLocked(newPath);
    }

private:
    const std::string m_name;
    const std::shared_ptr<clientInterfaces::ArtifactWrapperFactoryInterface> m_artifactWrapperFactory;

    std::shared_ptr<clientInterfaces::ArtifactWrapperInterface> m_activeArtifact;
    std::shared_ptr<clientInterfaces::ArtifactWrapperInterface> m_pendingArtifact;

    std::mutex m_eventMutex;
};

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKASSETMANAGERCLIENT_GENERICINVENTORY_H_
