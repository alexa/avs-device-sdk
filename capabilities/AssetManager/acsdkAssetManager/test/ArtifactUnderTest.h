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

#ifndef AVS_CAPABILITIES_ASSETMANAGER_ACSDKASSETMANAGER_TEST_ARTIFACTUNDERTEST_H_
#define AVS_CAPABILITIES_ASSETMANAGER_ACSDKASSETMANAGER_TEST_ARTIFACTUNDERTEST_H_

#include <AVSCommon/Utils/functional/hash.h>
#include <CurlWrapperMock.h>
#include <DavsServiceMock.h>
#include <TestUtil.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <functional>
#include <memory>
#include <ostream>

#include "RequestFactory.h"
#include "acsdkAssetManager/AssetManager.h"
#include "acsdkAssetManagerClient/AMD.h"
#include "acsdkAssetsInterfaces/Communication/AmdCommunicationInterface.h"
#include "acsdkAssetsInterfaces/Priority.h"
#include "acsdkAssetsInterfaces/State.h"
#include "acsdkDavsClient/DavsEndpointHandlerV3.h"

using namespace std;
using namespace chrono;
using namespace ::testing;
using namespace alexaClientSDK::acsdkAssets::common;
using namespace alexaClientSDK::acsdkAssets::commonInterfaces;
using namespace alexaClientSDK::acsdkAssets::davsInterfaces;
using namespace alexaClientSDK::acsdkAssets::davs;
using namespace alexaClientSDK::acsdkAssets::client;
using namespace alexaClientSDK::acsdkCommunicationInterfaces;

class ArtifactUnderTest
        : public CommunicationPropertyChangeSubscriber<int>
        , public CommunicationPropertyChangeSubscriber<string>
        , public enable_shared_from_this<ArtifactUnderTest> {
public:
    bool hasAllProps() {
        return hasStateProp() && hasPriorityProp() && hasPathProp();
    }

    bool hasStateProp() {
        int value;
        return commsHandler->readProperty(request->getSummary() + AMD::STATE_SUFFIX, value);
    }

    bool hasPriorityProp() {
        int value;
        return commsHandler->readProperty(request->getSummary() + AMD::PRIORITY_SUFFIX, value);
    }

    bool hasPathProp() {
        return commsHandler->invoke(request->getSummary() + AMD::PATH_SUFFIX).isSucceeded();
    }

    State getStateProp() {
        int value;
        commsHandler->readProperty(request->getSummary() + AMD::STATE_SUFFIX, value);
        return static_cast<State>(value);
    }

    Priority getPriorityProp() {
        int temp;
        commsHandler->readProperty(request->getSummary() + AMD::PRIORITY_SUFFIX, temp);
        return static_cast<Priority>(temp);
    }

    string getPathProp() {
        return commsHandler->invoke(request->getSummary() + AMD::PATH_SUFFIX).value();
    }

    Priority setPriorityProp(Priority p) {
        commsHandler->writeProperty(request->getSummary() + AMD::PRIORITY_SUFFIX, static_cast<int>(p));
        return p;
    }

    bool waitUntilStateEquals(State expectedState, milliseconds timeout = milliseconds(500)) {
        return waitUntil([this, expectedState] { return getStateProp() == expectedState; }, timeout);
    }

    void onCommunicationPropertyChange(const std::string& PropertyName, int newValue) override {
        if (PropertyName == request->getSummary() + AMD::STATE_SUFFIX) {
            stateMap[static_cast<State>(newValue)] += 1;
        }
    }
    void onCommunicationPropertyChange(const std::string& PropertyName, string) override {
        if (PropertyName == request->getSummary() + AMD::UPDATE_SUFFIX) {
            updateEventCount++;
        }
    }
    void resetCounts() {
        updateEventCount = 0;
        stateMap.clear();
        commsHandler->unsubscribeToPropertyChangeEvent(
                request->getSummary() + AMD::STATE_SUFFIX,
                static_pointer_cast<CommunicationPropertyChangeSubscriber<int>>(shared_from_this()));
        commsHandler->unsubscribeToPropertyChangeEvent(
                request->getSummary() + AMD::UPDATE_SUFFIX,
                static_pointer_cast<CommunicationPropertyChangeSubscriber<string>>(shared_from_this()));
    }
    void subscribeToChangeEvents() {
        commsHandler->subscribeToPropertyChangeEvent(
                request->getSummary() + AMD::STATE_SUFFIX,
                static_pointer_cast<CommunicationPropertyChangeSubscriber<int>>(shared_from_this()));
        commsHandler->subscribeToPropertyChangeEvent(
                request->getSummary() + AMD::UPDATE_SUFFIX,
                static_pointer_cast<CommunicationPropertyChangeSubscriber<string>>(shared_from_this()));
    }
    ArtifactUnderTest(std::shared_ptr<AmdCommunicationInterface> comm, shared_ptr<ArtifactRequest> request) :
            commsHandler(comm),
            request(request) {
    }
    std::shared_ptr<AmdCommunicationInterface> commsHandler;
    shared_ptr<ArtifactRequest> request;
    unordered_map<State, int, alexaClientSDK::avsCommon::utils::functional::EnumClassHash> stateMap;
    int updateEventCount = 0;
};

#endif  // AVS_CAPABILITIES_ASSETMANAGER_ACSDKASSETMANAGER_TEST_ARTIFACTUNDERTEST_H_
