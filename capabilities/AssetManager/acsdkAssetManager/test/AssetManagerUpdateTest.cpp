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

// Two valid artifact json will be tested by Integ Test
constexpr auto VALID_ONE_ARTIFACT_JSON = R"delim({"artifactList":[{"type":"test","key":"tar"}]})delim";
constexpr auto INVALID_ONE_ARTIFACT_JSON =
        R"delim({"artifactList":[{"type":"test-invalid","key":"tar-invalid"}]})delim";

class UpdateTest
        : public AssetManagerTest
        , public WithParamInterface<bool> {
public:
    void SetUp() override {
        AssetManagerTest::SetUp();

        artifact->commsHandler = commsHandler;

        uploadArtifactFromRequest(artifact->request, artifactSize, origId, ttl);
        ASSERT_TRUE(assetManager->downloadArtifact(artifact->request));
        ASSERT_TRUE(artifact->waitUntilStateEquals(State::LOADED));
        ASSERT_TRUE(artifact->hasAllProps());
        auto path = artifact->getPathProp();
        ASSERT_TRUE(filesystem::exists(path));

        updateAccepted = GetParam();
        updateRejected = !updateAccepted;
        oldPath = artifact->getPathProp();
        newPath = replaceAll(oldPath, origId, updatedId);
    }

    void uploadArtifactAndSubscribeToChange(
            const shared_ptr<ArtifactUnderTest>& artifactUnderTest,
            const Priority priority,
            string& updatedIdProp) {
        uploadArtifactFromRequest(artifactUnderTest->request, artifactSize, updatedIdProp);
        artifactUnderTest->setPriorityProp(priority);
        // only after we've set the priority accordingly and have gone through the update request
        artifactUnderTest->subscribeToChangeEvents();
    }

    void checkArtifactUpdatedOnce(
            const shared_ptr<ArtifactUnderTest>& artifactUnderTest,
            bool& updateAcceptedFlag,
            bool& updateRejectedFlag,
            string& oldPathProp,
            string& newPathProp) {
        ASSERT_TRUE(waitUntil([&] { return filesystem::exists(newPathProp); }, milliseconds(ttl * 10)));
        assetManager->handleUpdate(artifactUnderTest->request->getSummary(), updateAcceptedFlag);
        ASSERT_EQ(updateAccepted ? newPathProp : oldPathProp, artifactUnderTest->getPathProp());
        ASSERT_EQ(filesystem::exists(oldPathProp), updateRejectedFlag);
        ASSERT_EQ(filesystem::exists(newPathProp), updateAcceptedFlag);
        ASSERT_EQ(artifactUnderTest->updateEventCount, 1);
    }

    string replaceAll(string str, const string& from, const string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    milliseconds ttl = milliseconds(500);
    size_t artifactSize = 10;
    string origId = "original";
    string updatedId = "updated_id";

    bool updateAccepted{};
    bool updateRejected{};
    string oldPath;
    string newPath;
    shared_ptr<ArtifactUnderTest> artifact =
            make_shared<ArtifactUnderTest>(nullptr, DavsRequest::create("test", "tar", {{"filter", {"first"}}}));
};

TEST_P(UpdateTest, UpdatingArtifactsDeletesTheOldResourceAndAcquiresTheNew) {  // NOLINT
    uploadArtifactFromRequest(artifact->request, artifactSize, updatedId);

    // nothing should happen when requesting update for invalid states or when artifact isn't ready
    assetManager->handleUpdate("", updateAccepted);
    assetManager->handleUpdate("{validRequest:false}", updateAccepted);
    assetManager->handleUpdate(artifact->request->getSummary(), updateAccepted);

    artifact->subscribeToChangeEvents();
    // only after we've set the priority accordingly and have gone through the update request
    artifact->setPriorityProp(Priority::ACTIVE);

    ASSERT_TRUE(waitUntil([&] { return filesystem::exists(newPath); }, ttl * 10));
    ASSERT_TRUE(filesystem::exists(oldPath));
    ASSERT_EQ(oldPath, artifact->getPathProp());

    assetManager->handleUpdate(artifact->request->getSummary(), updateAccepted);
    ASSERT_EQ(updateAccepted ? newPath : oldPath, artifact->getPathProp());
    ASSERT_EQ(filesystem::exists(oldPath), updateRejected);
    ASSERT_EQ(filesystem::exists(newPath), updateAccepted);
    ASSERT_EQ(artifact->updateEventCount, 1);
    artifact->resetCounts();
}

TEST_P(UpdateTest, UpdatingArtifactsWillKeepRetryingUntilItTimesOutAndDeletesTheNew) {  // NOLINT
    uploadArtifactFromRequest(artifact->request, artifactSize, updatedId);

    // nothing should happen when requesting update for invalid states or when artifact isn't ready
    assetManager->handleUpdate("", updateAccepted);
    assetManager->handleUpdate("{validRequest:false}", updateAccepted);
    assetManager->handleUpdate(artifact->request->getSummary(), updateAccepted);

    artifact->subscribeToChangeEvents();

    // only after we've set the priority accordingly and have gone through the update request
    artifact->setPriorityProp(Priority::ACTIVE);

    // expect to send 2 update events when we are not getting a handle update response
    ASSERT_TRUE(waitUntil([&] { return filesystem::exists(newPath); }, ttl * 10));
    ASSERT_TRUE(filesystem::exists(oldPath));
    ASSERT_EQ(oldPath, artifact->getPathProp());

    // after some time, we will delete the new artifact and keep the old
    ASSERT_TRUE(waitUntil([&] { return !filesystem::exists(newPath); }, ttl * 10));
    ASSERT_TRUE(filesystem::exists(oldPath));
    ASSERT_EQ(oldPath, artifact->getPathProp());
    ASSERT_EQ(artifact->updateEventCount, 2);
    artifact->resetCounts();
}

TEST_P(UpdateTest, HandlingSharedArtifactsWhereOneGetsUpdatedDoesNotDeleteOldResource) {  // NOLINT
    shared_ptr<ArtifactUnderTest> otherArtifact =
            make_shared<ArtifactUnderTest>(commsHandler, DavsRequest::create("test", "tar", {{"filter", {"second"}}}));
    uploadArtifactFromRequest(otherArtifact->request, artifactSize, origId);

    ASSERT_TRUE(assetManager->downloadArtifact(otherArtifact->request));
    ASSERT_TRUE(otherArtifact->waitUntilStateEquals(State::LOADED));
    ASSERT_TRUE(otherArtifact->hasAllProps());

    uploadArtifactFromRequest(artifact->request, artifactSize, updatedId);
    artifact->subscribeToChangeEvents();
    artifact->setPriorityProp(Priority::ACTIVE);

    ASSERT_TRUE(waitUntil([&] { return filesystem::exists(newPath); }, ttl * 10));
    ASSERT_TRUE(filesystem::exists(oldPath));
    ASSERT_EQ(oldPath, artifact->getPathProp());

    assetManager->handleUpdate(artifact->request->getSummary(), updateAccepted);
    assetManager->handleUpdate(otherArtifact->request->getSummary(), updateAccepted);  // nothing should happen here
    ASSERT_EQ(updateAccepted ? newPath : oldPath, artifact->getPathProp());
    ASSERT_EQ(filesystem::exists(oldPath), true);  // never get rid of the old path since it's being shared
    ASSERT_EQ(filesystem::exists(newPath), updateAccepted);
    ASSERT_EQ(otherArtifact->getPathProp(), oldPath);
    ASSERT_EQ(artifact->updateEventCount, 1);
    artifact->resetCounts();
}

TEST_P(UpdateTest, CheckingForUpdateAtStartupAfterArtifactBecomesActive) {  // NOLINT
    int updateCount = 0;
    uploadArtifactFromRequest(artifact->request, artifactSize, updatedId);
    artifact->setPriorityProp(Priority::ACTIVE);
    artifact->subscribeToChangeEvents();
    //    artifact->expectUpdateEvent(newPath);
    ASSERT_TRUE(waitUntil([&] { return filesystem::exists(newPath); }, ttl * 10));

    assetManager->handleUpdate(artifact->request->getSummary(), updateAccepted);
    ASSERT_EQ(updateAccepted ? newPath : oldPath, artifact->getPathProp());
    ASSERT_EQ(filesystem::exists(oldPath), updateRejected);
    ASSERT_EQ(filesystem::exists(newPath), updateAccepted);

    // make sure things are still reflected after reboot
    if (updateRejected) {
        // note that if we failed to update, DavsClient will recheck with DAVS at bootup
        updateCount = 1;
    }
    ASSERT_EQ(artifact->updateEventCount, 1);
    artifact->resetCounts();
    shutdownAssetManager();
    startAssetManager();
    artifact->subscribeToChangeEvents();
    // the new artifact will always be checked and downloaded when changing to active
    artifact->setPriorityProp(Priority::ACTIVE);
    ASSERT_TRUE(waitUntil([&] { return filesystem::exists(newPath); }, ttl * 10));
    ASSERT_TRUE(waitUntil([&] { return (updateAccepted ? newPath : oldPath) == artifact->getPathProp(); }));
    ASSERT_EQ(filesystem::exists(oldPath), updateRejected);
    ASSERT_EQ(filesystem::exists(newPath), true);
    ASSERT_EQ(artifact->updateEventCount, updateCount);
    artifact->resetCounts();
}
TEST_P(UpdateTest, UpdatingOneActiveArtifactViaDeviceArtifactNotification) {
    uploadArtifactAndSubscribeToChange(artifact, Priority::ACTIVE, updatedId);

    // Trigger update from JSON
    davsClient->checkAndUpdateArtifactGroupFromJson(VALID_ONE_ARTIFACT_JSON);

    checkArtifactUpdatedOnce(artifact, updateAccepted, updateRejected, oldPath, newPath);
}
TEST_P(UpdateTest, UpdatingOneInactiveArtifactViaDeviceArtifactNotification) {
    uploadArtifactAndSubscribeToChange(artifact, Priority::UNUSED, updatedId);

    // Trigger update from JSON
    davsClient->checkAndUpdateArtifactGroupFromJson(VALID_ONE_ARTIFACT_JSON);

    ASSERT_FALSE(waitUntil([&] { return filesystem::exists(newPath); }, milliseconds(300)));
}
TEST_P(UpdateTest, UpdatingUnregisteredArtifactViaDeviceArtifactNotification) {
    uploadArtifactAndSubscribeToChange(artifact, Priority::UNUSED, updatedId);

    // Trigger update from JSON
    davsClient->checkAndUpdateArtifactGroupFromJson(INVALID_ONE_ARTIFACT_JSON);

    ASSERT_FALSE(waitUntil([&] { return filesystem::exists(newPath); }, milliseconds(300)));
}
INSTANTIATE_TEST_CASE_P(UpdatesAcceptedAndRejected, UpdateTest, Values(true, false), PrintToStringParamName());