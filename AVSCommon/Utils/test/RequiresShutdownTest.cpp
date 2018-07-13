/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

/// @file RequiresShutdownTest.cpp

#include <gtest/gtest.h>

#include "AVSCommon/Utils/RequiresShutdown.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace test {

/// Test class which implements RequiresShutdown
class Object : public alexaClientSDK::avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor.
     *
     * @param name of object to log in any messages.
     */
    Object(const std::string& name) : alexaClientSDK::avsCommon::utils::RequiresShutdown(name), properShutdown{true} {
    }

    /// Dummy shutdown function which doesn't do anything.
    void doShutdown() override;

    /// A shared_ptr which can be used to create loops.
    std::shared_ptr<Object> object;

    /// A flag indicating whether doShutdown should actually clean up object
    bool properShutdown;
};

void Object::doShutdown() {
    if (properShutdown) {
        object.reset();
    }
}

/**
 * This test covers all cases for RequiresShutdown.  It is written this way because the errors are detected when the
 * program exits, so there is no benefit to breaking these out into separate test functions.  Also note that there are
 * no EXPECT/ASSERT calls here because there are no outputs from RequiresShutdown.  Running this as a unit test
 * verifies that we don't crash, but functional verification currently requires a manual examination of the console
 * output from this test.
 */
TEST(RequiresShutdownTest, allTestCases) {
    // shared_ptr loop that implements and calls proper shutdown functions
    auto loopCallGoodShutdownMemberA = std::make_shared<Object>("loopCallGoodShutdownMemberA");
    auto loopCallGoodShutdownMemberB = std::make_shared<Object>("loopCallGoodShutdownMemberB");
    loopCallGoodShutdownMemberA->object = loopCallGoodShutdownMemberB;
    loopCallGoodShutdownMemberB->object = loopCallGoodShutdownMemberA;
    loopCallGoodShutdownMemberA->shutdown();
    loopCallGoodShutdownMemberB->shutdown();
    loopCallGoodShutdownMemberA.reset();
    loopCallGoodShutdownMemberB.reset();

    // shared_ptr loop that implements proper shutdown functions, but doesn't call them (and thus leaks)
    auto loopNocallGoodShutdownMemberA = std::make_shared<Object>("loopNocallGoodShutdownMemberA");
    auto loopNocallGoodShutdownMemberB = std::make_shared<Object>("loopNocallGoodShutdownMemberB");
    loopNocallGoodShutdownMemberA->object = loopNocallGoodShutdownMemberB;
    loopNocallGoodShutdownMemberB->object = loopNocallGoodShutdownMemberA;
    loopNocallGoodShutdownMemberA.reset();
    loopNocallGoodShutdownMemberB.reset();

    // shared_ptr loop that implements and calls shutdown functions, but they don't break the loop (and thus leak)
    auto loopCallBadShutdownMemberA = std::make_shared<Object>("loopCallBadShutdownMemberA");
    auto loopCallBadShutdownMemberB = std::make_shared<Object>("loopCallBadShutdownMemberB");
    loopCallBadShutdownMemberA->object = loopCallBadShutdownMemberB;
    loopCallBadShutdownMemberB->object = loopCallBadShutdownMemberA;
    loopCallBadShutdownMemberA->properShutdown = false;
    loopCallBadShutdownMemberB->properShutdown = false;
    loopCallBadShutdownMemberA->shutdown();
    loopCallBadShutdownMemberB->shutdown();
    loopCallBadShutdownMemberA.reset();
    loopCallBadShutdownMemberB.reset();

    // raw pointer leak that implements and calls proper shutdown function
    auto rawPointerLeakCallShutdown = new Object("rawPointerLeakCallShutdown");
    rawPointerLeakCallShutdown->shutdown();

    // raw pointer leak that implements and calls proper shutdown function, but doesn't call it
    auto rawPointerLeakNoCallShutdown = new Object("rawPointerLeakNoCallShutdown");
    (void)rawPointerLeakNoCallShutdown;
}

}  // namespace test
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
