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

using P = Priority;
static constexpr bool ERASED = true;
static constexpr bool KEPT = false;
const auto MB = 1024 * 4;  // not really, but for our test we'll treat KB as MB
const auto ARTIFACT_SIZE = 10 * MB;

struct EvictionData {
    vector<Priority> priorities;
    vector<int> orderUsageIndices;
    size_t spaceNeeded;
    bool spaceFreed;
    vector<bool> deletedIndices;
    string description;
};

class EvictionTest
        : public AssetManagerTest
        , public WithParamInterface<EvictionData> {
public:
    void SetUp() override {
        AssetManagerTest::SetUp();
        auto& p = GetParam();

        for (size_t i = 0; i < p.priorities.size(); i++) {
            ArtifactUnderTest artifact{
                    commsHandler,
                    DavsRequest::create(to_string(i), "k", {{"k", {"v"}}}, Region::NA, ArtifactRequest::UNPACK)};
            uploadArtifactFromRequest(artifact.request, static_cast<size_t>(ARTIFACT_SIZE));
            ASSERT_TRUE(assetManager->downloadArtifact(artifact.request)) << "Failed setup: " << p.description;
            ASSERT_TRUE(artifact.waitUntilStateEquals(State::LOADED)) << "Failed setup: " << p.description;
            ASSERT_TRUE(artifact.hasAllProps());
            artifact.setPriorityProp(p.priorities[i]);
            artifacts.emplace_back(move(artifact));
        }
    }

    void waitForDeletion(const vector<string>& paths) {
        auto& p = GetParam();
        // wait until all the artifacts that were meant to be deleted to get erased
        auto waited = false;
        for (size_t i = 0; i < p.deletedIndices.size(); i++) {
            if (p.deletedIndices[i]) {
                auto path = paths[i];
                ASSERT_TRUE(waitUntil([path] { return !filesystem::exists(path); }));
                waited = true;
            }
        }
        if (!waited) {
            this_thread::sleep_for(milliseconds(100));
        }
    }

    vector<ArtifactUnderTest> artifacts;
};

TEST_P(EvictionTest, LastUsedScenario) {  // NOLINT
    auto& p = GetParam();

    vector<string> paths(artifacts.size());
    for (auto index : p.orderUsageIndices) {
        paths[index] = artifacts[index].getPathProp();
        ASSERT_TRUE(filesystem::exists(paths[index])) << "Expected path does not exists: " << p.description;
        this_thread::sleep_for(milliseconds(1));
    }

    ASSERT_EQ(assetManager->freeUpSpace(p.spaceNeeded), p.spaceFreed)
            << "Failed freeUpSpace result check: " << p.description;

    for (size_t i = 0; i < p.deletedIndices.size(); i++) {
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasStateProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction state check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasPriorityProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction priority check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasPathProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction path check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !filesystem::exists(paths[i]) == p.deletedIndices[i]; }))
                << "Failed artifact eviction file deletion check: " << p.description;
    }
}

TEST_P(EvictionTest, RestartingAssetManagerPreservesLastUsed) {  // NOLINT
    auto& p = GetParam();

    vector<string> paths(artifacts.size());
    for (auto index : p.orderUsageIndices) {
        paths[index] = artifacts[index].getPathProp();
        ASSERT_TRUE(filesystem::exists(artifacts[index].getPathProp()))
                << "Expected path does not exists: " << p.description;
        this_thread::sleep_for(milliseconds(1));
    }

    shutdownAssetManager();
    startAssetManager();
    for (size_t i = 0; i < p.priorities.size(); i++) {
        artifacts[i].setPriorityProp(p.priorities[i]);
    }

    ASSERT_TRUE(waitUntil([&] { return assetManager->freeUpSpace(p.spaceNeeded) == p.spaceFreed; }))
            << "Failed freeUpSpace result check: " << p.description;

    for (size_t i = 0; i < p.deletedIndices.size(); i++) {
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasStateProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction state check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasPriorityProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction priority check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasPathProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction path check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !filesystem::exists(paths[i]) == p.deletedIndices[i]; }))
                << "Failed artifact eviction file deletion check: " << p.description;
    }
}

TEST_P(EvictionTest, LoweringBudgetScenario) {  // NOLINT
    auto& p = GetParam();

    vector<string> paths(artifacts.size());
    for (auto index : p.orderUsageIndices) {
        paths[index] = artifacts[index].getPathProp();
        ASSERT_TRUE(filesystem::exists(artifacts[index].getPathProp()))
                << "Expected path does not exists: " << p.description;
        this_thread::sleep_for(milliseconds(1));
    }

    // budget is expected in MB
    int newBudget = max(0, static_cast<int>(artifacts.size()) * ARTIFACT_SIZE - static_cast<int>(p.spaceNeeded)) / MB;
    assetManager->setBudget(newBudget);
    waitForDeletion(paths);

    for (size_t i = 0; i < p.deletedIndices.size(); i++) {
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasStateProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction state check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasPriorityProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction priority check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !artifacts[i].hasPathProp() == p.deletedIndices[i]; }))
                << "Failed artifact eviction path check: " << p.description;
        ASSERT_TRUE(waitUntil([&] { return !filesystem::exists(paths[i]) == p.deletedIndices[i]; }))
                << "Failed artifact eviction file deletion check: " << p.description;
    }
}

// clang-format off
INSTANTIATE_TEST_CASE_P(EvictionTestCases, EvictionTest, ValuesIn<vector<EvictionData>>(
        // List of artifacts w/ Priorities               | Usage Order | Space Needed || Space Freed? | What got erased          | Description
        {{{P::UNUSED, P::UNUSED, P::UNUSED}              , {0, 1, 2}   , 0 * MB        , true         , {KEPT,   KEPT,   KEPT}   , "Requesting 0 bytes preserves all the artifacts"},
         {{P::UNUSED, P::UNUSED, P::UNUSED}              , {0, 1, 2}   , 9 * MB        , true         , {ERASED, KEPT,   KEPT}   , "Remove only as many artifacts that are needed to free up the requested space"},
         {{P::UNUSED, P::UNUSED, P::UNUSED}              , {0, 1, 2}   , 25 * MB       , true         , {ERASED, ERASED, ERASED} , "Remove all unused artifacts if necessary to clear up space"},
         {{P::UNUSED, P::UNUSED, P::UNUSED}              , {0, 1, 2}   , 31 * MB       , false        , {ERASED, ERASED, ERASED} , "Inform caller that we failed to clear sufficient space even after clearing all unused artifacts"},
         {{P::ACTIVE, P::UNUSED, P::PENDING_ACTIVATION}  , {0, 1, 2}   , 15 * MB       , false        , {KEPT,   ERASED, KEPT}   , "Never clear active or pending activation priorities even if more space is requested"},
         {{P::LIKELY_TO_BE_ACTIVE, P::UNUSED, P::UNUSED} , {2, 1, 0}   , 20 * MB       , true         , {KEPT,   ERASED, ERASED} , "Start erasing artifacts with lowest priority even if they were more recently used"},
         {{P::UNUSED, P::UNUSED, P::UNUSED}              , {1, 2, 0}   , 10 * MB       , true         , {KEPT,   ERASED, KEPT}   , "If priority is the same then erase the oldest used artifact"},
        }), PrintDescription());
// clang-format on