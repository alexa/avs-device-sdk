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

class SharedResourceTest : public AssetManagerTest {
public:
    void SetUp() override {
        AssetManagerTest::SetUp();

        sameId_1.commsHandler = commsHandler;
        sameId_2.commsHandler = commsHandler;
        differentArtifact.commsHandler = commsHandler;

        uploadArtifactFromRequest(sameId_1.request, artifactSize, tarId);
        uploadArtifactFromRequest(sameId_2.request, artifactSize, tarId);
        uploadArtifactFromRequest(differentArtifact.request, artifactSize);
    }

    // clang-format off
    size_t artifactSize = 10;
    string tarId = "tarid";
    ArtifactUnderTest sameId_1{nullptr, DavsRequest::create("test", "tar", {{"filter", {"value1"}}}, Region::NA, ArtifactRequest::UNPACK)};
    ArtifactUnderTest sameId_2{nullptr, DavsRequest::create("test", "tar", {{"filter", {"value2"}}}, Region::NA, ArtifactRequest::UNPACK)};
    ArtifactUnderTest differentArtifact{nullptr, DavsRequest::create("different", "tar", {{"filterX", {"valueY"}}}, Region::NA, ArtifactRequest::UNPACK)};
    // clang-format on
};

TEST_F(SharedResourceTest, RequestingTheSameArtifactWithDifferentRequestDedups) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_1.request));
    ASSERT_TRUE(sameId_1.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(sameId_1.hasAllProps());

    auto path = sameId_1.getPathProp();
    ASSERT_TRUE(filesystem::exists(path));

    ASSERT_TRUE(assetManager->downloadArtifact(sameId_2.request));
    ASSERT_TRUE(sameId_2.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(sameId_2.hasAllProps());

    ASSERT_EQ(path, sameId_2.getPathProp());
    ASSERT_TRUE(filesystem::exists(path));
}

TEST_F(SharedResourceTest,
       DeletingRequestWithSharedResourceDoesNotDeleteResourceUntilAllRequestsAreDeleted) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_1.request));
    ASSERT_TRUE(sameId_1.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_2.request));
    ASSERT_TRUE(sameId_2.waitUntilStateEquals(State::LOADED));

    auto path = sameId_1.getPathProp();
    ASSERT_EQ(path, sameId_2.getPathProp());
    ASSERT_TRUE(filesystem::exists(path));

    assetManager->deleteArtifact(sameId_2.request->getSummary());
    ASSERT_TRUE(waitUntil([this] { return !sameId_2.hasStateProp(); }));
    ASSERT_FALSE(sameId_2.hasPathProp());
    ASSERT_TRUE(filesystem::exists(path));

    assetManager->deleteArtifact(sameId_1.request->getSummary());
    ASSERT_TRUE(waitUntil([this] { return !sameId_1.hasStateProp(); }));
    ASSERT_FALSE(sameId_1.hasPathProp());
    ASSERT_FALSE(filesystem::exists(path));
}

TEST_F(SharedResourceTest, ReloadingExistingArtifacts) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_1.request));
    ASSERT_TRUE(sameId_1.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_2.request));
    ASSERT_TRUE(sameId_2.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(assetManager->downloadArtifact(differentArtifact.request));
    ASSERT_TRUE(differentArtifact.waitUntilStateEquals(State::LOADED));

    auto samePath = sameId_1.getPathProp();
    ASSERT_EQ(samePath, sameId_2.getPathProp());
    ASSERT_TRUE(filesystem::exists(samePath));
    auto differentPath = differentArtifact.getPathProp();
    ASSERT_TRUE(filesystem::exists(differentPath));

    shutdownAssetManager();
    startAssetManager();

    ASSERT_EQ(samePath, sameId_1.getPathProp());
    ASSERT_EQ(sameId_1.getPathProp(), sameId_1.getPathProp());
    ASSERT_EQ(differentPath, differentArtifact.getPathProp());
    ASSERT_EQ(sameId_1.getStateProp(), State::LOADED);
    ASSERT_EQ(sameId_2.getStateProp(), State::LOADED);
    ASSERT_EQ(differentArtifact.getStateProp(), State::LOADED);
}

TEST_F(SharedResourceTest, ClearingSpaceAccountsForSharedResource) {  // NOLINT
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_1.request));
    ASSERT_TRUE(sameId_1.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(assetManager->downloadArtifact(sameId_2.request));
    ASSERT_TRUE(sameId_2.waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(assetManager->downloadArtifact(differentArtifact.request));
    ASSERT_TRUE(differentArtifact.waitUntilStateEquals(State::LOADED));

    // have the order from oldest to newest: sameId_2, different, sameId_1
    auto same2Path = sameId_2.getPathProp();
    auto differentPath = differentArtifact.getPathProp();
    auto same1Path = sameId_1.getPathProp();

    ASSERT_TRUE(assetManager->freeUpSpace(artifactSize));

    ASSERT_TRUE(filesystem::exists(same1Path));
    ASSERT_TRUE(filesystem::exists(same2Path));
    ASSERT_FALSE(filesystem::exists(differentPath));
}