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

#include "AssetManagerTest.h"

static const auto TIMEOUT = milliseconds(100);

TEST_F(AssetManagerTest, InvalidParameters) {  // NOLINT
    // clang-format off
    ASSERT_TRUE(nullptr == AssetManager::create(nullptr, davsClient, BASE_DIR, authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == AssetManager::create(commsHandler, nullptr, BASE_DIR, authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == AssetManager::create(commsHandler, davsClient, "", authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == AssetManager::create(commsHandler, davsClient, "/", authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr == AssetManager::create(commsHandler,davsClient,"/non/existing/directory",authDelegateMock,allowUrlList));
    ASSERT_TRUE(nullptr ==AssetManager::create(commsHandler, davsClient, BASE_DIR, nullptr,allowUrlList));
    ASSERT_TRUE(nullptr ==AssetManager::create(commsHandler, davsClient, BASE_DIR, authDelegateMock,nullptr));

    // clang-format on
}

TEST_F(AssetManagerTest, DavsInvalidMetadataJsonFileOnLoad) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());

    auto path = tarArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    shutdownAssetManager();
    // Deleting all metadata files ensures invalid metadata on load
    filesystem::removeAll(DAVS_REQUESTS_DIR + "/" + filesystem::list(DAVS_REQUESTS_DIR)[0]);
    startAssetManager();

    ASSERT_FALSE(tarArtifact->hasPathProp());
    ASSERT_FALSE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, DavsRequestingValidDownloadUpdatesLipcProprtyAndDownloadsArtifactsToDisk) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());

    ASSERT_EQ(tarArtifact->getPriorityProp(), Priority::UNUSED);
    ASSERT_TRUE(filesystem::exists(tarArtifact->getPathProp() + "/target"));
}

TEST_F(AssetManagerTest,
       DavsRequestingValidButUnavailableArtifactSucceedsDownloadCallButLipcUpdatesAsInvalid) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(unavailableArtifact->request));
    ASSERT_TRUE(unavailableArtifact->hasAllProps());
    ASSERT_FALSE(unavailableArtifact->waitUntilStateEquals(State::LOADED, TIMEOUT));

    ASSERT_FALSE(unavailableArtifact->hasStateProp());
    ASSERT_FALSE(unavailableArtifact->hasPriorityProp());
}

TEST_F(AssetManagerTest, DavsDownloadingTheSameArtifactDedups) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());

    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_EQ(tarArtifact->getStateProp(), State::LOADED);
}

TEST_F(AssetManagerTest, DavsRestartingAssetManagerAfterDownloadingDavsArtifactReloadsItFromDisk) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());
    ASSERT_TRUE(filesystem::exists(tarArtifact->getPathProp() + "/target"));

    shutdownAssetManager();
    ASSERT_FALSE(tarArtifact->hasStateProp());
    ASSERT_FALSE(tarArtifact->hasPriorityProp());
    ASSERT_FALSE(tarArtifact->hasPathProp());

    startAssetManager();
    ASSERT_EQ(tarArtifact->getStateProp(), State::LOADED);
    ASSERT_EQ(tarArtifact->getPriorityProp(), Priority::UNUSED);
    ASSERT_TRUE(filesystem::exists(tarArtifact->getPathProp() + "/target"));
}
TEST_F(AssetManagerTest, DavsDeletingAnExistingArtifactRemovesItsPropertiesAndSendsAnInvalidStateEvent) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());
    auto path = tarArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    assetManager->deleteArtifact(tarArtifact->request->getSummary());

    ASSERT_TRUE(waitUntil([this] { return !tarArtifact->hasStateProp(); }));
    ASSERT_FALSE(tarArtifact->hasStateProp());
    ASSERT_FALSE(tarArtifact->hasPriorityProp());
    ASSERT_FALSE(tarArtifact->hasPathProp());
    ASSERT_FALSE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, DavsDeletingAnInvalidArtifactDoesNotImpactExistingArtifacts) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());

    auto path = tarArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    // deleting invalid request should have no impact on the results of this test and should be handled gracefully
    assetManager->deleteArtifact("");
    assetManager->deleteArtifact("{validRequest:false}");

    ASSERT_FALSE(waitUntil([this] { return !tarArtifact->hasStateProp(); }, TIMEOUT));
    ASSERT_TRUE(tarArtifact->hasStateProp());
    ASSERT_TRUE(tarArtifact->hasPriorityProp());
    ASSERT_TRUE(tarArtifact->hasPathProp());
    ASSERT_TRUE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, DavsRequestingDownloadOfDeletedArtifactSucceeds) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());
    auto path = tarArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    assetManager->deleteArtifact(tarArtifact->request->getSummary());
    ASSERT_TRUE(waitUntil([this] { return !tarArtifact->hasStateProp(); }));
    ASSERT_FALSE(filesystem::exists(path));

    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, PropogatesIdleState) {  // NOLINT
    assetManager->onIdleChanged(1);
    ASSERT_TRUE(waitUntil([&] { return davsClient->getIdleState(); }));
    assetManager->onIdleChanged(0);
    ASSERT_TRUE(waitUntil([&] { return !davsClient->getIdleState(); }));
}

TEST_F(AssetManagerTest, DavsDownloadWhileDeviceActive) {  // NOLINT
    assetManager->onIdleChanged(0);
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarArtifact->hasAllProps());

    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_EQ(tarArtifact->getStateProp(), State::LOADED);
}

TEST_F(AssetManagerTest, UrlInvalidMetadataJsonFileOnLoad) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());

    auto path = tarUrlArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    shutdownAssetManager();
    // Deleting all metadata files ensures invalid metadata on load
    filesystem::removeAll(DAVS_REQUESTS_DIR + "/" + filesystem::list(DAVS_REQUESTS_DIR)[0]);
    startAssetManager();

    ASSERT_FALSE(tarUrlArtifact->hasPathProp());
    ASSERT_FALSE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, UrlRequestingValidDownloadUpdatesLipcProprtyAndDownloadsArtifactsToDisk) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());

    ASSERT_EQ(tarUrlArtifact->getPriorityProp(), Priority::UNUSED);
    ASSERT_TRUE(filesystem::exists(tarUrlArtifact->getPathProp()));
}
TEST_F(AssetManagerTest, UrlRequestingUnavailableArtifactSucceedsDownloadCallButLipcUpdatesAsInvalid) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(unavailableUrlArtifact->request));
    ASSERT_FALSE(unavailableUrlArtifact->waitUntilStateEquals(State::LOADED, TIMEOUT));

    ASSERT_FALSE(unavailableUrlArtifact->hasStateProp());
    ASSERT_FALSE(unavailableUrlArtifact->hasPriorityProp());

    unavailableUrlArtifact->resetCounts();
}
TEST_F(AssetManagerTest, UrlRequestingDownloadOfHttpArtifactSucceedsDownloadCallButLipcUpdatesAsInvalid) {  // NOLINT
    ASSERT_FALSE(assetManager->downloadArtifact(httpUrlArtifact->request));
    ASSERT_FALSE(httpUrlArtifact->waitUntilStateEquals(State::LOADED, TIMEOUT));

    ASSERT_FALSE(httpUrlArtifact->hasStateProp());
    ASSERT_FALSE(httpUrlArtifact->hasPriorityProp());
}
TEST_F(AssetManagerTest, UrlRequestingDownloadOfNonApprovedArtifactFailsDownloadCall) {  // NOLINT
    ASSERT_FALSE(assetManager->downloadArtifact(nonApprovedUrlArtifact->request));
    ASSERT_TRUE(waitUntil([this] { return !nonApprovedUrlArtifact->hasStateProp(); }, TIMEOUT));

    ASSERT_FALSE(nonApprovedUrlArtifact->waitUntilStateEquals(State::LOADED, TIMEOUT));

    ASSERT_FALSE(nonApprovedUrlArtifact->hasStateProp());
    ASSERT_FALSE(nonApprovedUrlArtifact->hasPriorityProp());
}

TEST_F(AssetManagerTest, UrlDownloadingTheSameArtifactDedups) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_TRUE(tarArtifact->waitUntilStateEquals(State::LOADED));

    ASSERT_TRUE(assetManager->downloadArtifact(tarArtifact->request));
    ASSERT_EQ(tarArtifact->getStateProp(), State::LOADED);
}

TEST_F(AssetManagerTest, UrlRestartingAssetManagerAfterDownloadingUrlArtifactReloadsItFromDisk) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());
    ASSERT_TRUE(filesystem::exists(tarUrlArtifact->getPathProp()));

    shutdownAssetManager();
    ASSERT_FALSE(tarUrlArtifact->hasStateProp());
    ASSERT_FALSE(tarUrlArtifact->hasPriorityProp());
    ASSERT_FALSE(tarUrlArtifact->hasPathProp());

    startAssetManager();
    ASSERT_EQ(tarUrlArtifact->getStateProp(), State::LOADED);
    ASSERT_EQ(tarUrlArtifact->getPriorityProp(), Priority::UNUSED);
    ASSERT_TRUE(filesystem::exists(tarUrlArtifact->getPathProp()));
}

TEST_F(AssetManagerTest, UrlDeletingAnExistingArtifactRemovesItsPropertiesAndSendsAnInvalidStateEvent) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());
    auto path = tarUrlArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    tarUrlArtifact->subscribeToChangeEvents();
    assetManager->deleteArtifact(tarUrlArtifact->request->getSummary());

    ASSERT_TRUE(waitUntil([this] { return !tarUrlArtifact->hasStateProp(); }));
    ASSERT_FALSE(tarUrlArtifact->hasPriorityProp());
    ASSERT_FALSE(tarUrlArtifact->hasPathProp());
    ASSERT_EQ(tarUrlArtifact->stateMap[State::INVALID], 1);
    ASSERT_FALSE(filesystem::exists(path));
    tarUrlArtifact->resetCounts();
}

TEST_F(AssetManagerTest, UrlDeletingAnInvalidArtifactDoesNotImpactExistingArtifacts) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());
    auto path = tarUrlArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    // deleting invalid request should have no impact on the results of this test and should be handled gracefully
    assetManager->deleteArtifact("");
    assetManager->deleteArtifact("{validRequest:false}");

    ASSERT_FALSE(waitUntil([this] { return !tarUrlArtifact->hasStateProp(); }, TIMEOUT));
    ASSERT_TRUE(tarUrlArtifact->hasPriorityProp());
    ASSERT_TRUE(tarUrlArtifact->hasPathProp());
    ASSERT_TRUE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, UrlRequestingDownloadOfDeletedArtifactSucceeds) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());
    auto path = tarUrlArtifact->getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    assetManager->deleteArtifact(tarUrlArtifact->request->getSummary());
    ASSERT_TRUE(waitUntil([this] { return !tarUrlArtifact->hasStateProp(); }));
    ASSERT_FALSE(filesystem::exists(path));

    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(filesystem::exists(path));
}

TEST_F(AssetManagerTest, UrlDownloadWhileDeviceActive) {  // NOLINT
    assetManager->onIdleChanged(0);
    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_TRUE(tarUrlArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(tarUrlArtifact->hasAllProps());

    ASSERT_TRUE(assetManager->downloadArtifact(tarUrlArtifact->request));
    ASSERT_EQ(tarUrlArtifact->getStateProp(), State::LOADED);
}