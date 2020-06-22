/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <gtest/gtest.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include "InterruptModel/InterruptModel.h"
#include "InterruptModel/config/InterruptModelConfiguration.h"

namespace alexaClientSDK {
namespace afml {
namespace interruptModel {
namespace test {

using namespace alexaClientSDK::avsCommon::utils::configuration;
using namespace alexaClientSDK::afml::interruptModel;
using namespace alexaClientSDK::avsCommon::avs;
using namespace ::testing;
/// Alias for JSON stream type used in @c ConfigurationNode initialization
using JSONStream = std::vector<std::shared_ptr<std::istream>>;

static const std::string CONTENT_CHANNEL = "Content";
static const std::string DIALOG_CHANNEL = "Dialog";
static const std::string ALERT_CHANNEL = "Alert";
static const ContentType MIXABLE_CONTENT_TYPE = ContentType::MIXABLE;
static const ContentType NONMIXABLE_CONTENT_TYPE = ContentType::NONMIXABLE;
static const ContentType INVALID_CONTENT_TYPE = ContentType::NUM_CONTENT_TYPE;
static const std::string INTERRUPT_MODEL_KEY = "interruptModel";
static const std::string INVALID_CONFIG_KEY = "invalidkey";
static const std::string NONEXISTENT_CHANNEL = "mysteryChannel";
static const std::string VIRTUAL_CHANNEL1 = "VirtualChannel1";
static const std::string VIRTUAL_CHANNEL2 = "VirtualChannel2";
static const std::string CONTENT_TYPE_KEY = "contentType";

static const std::string CONFIG_JSON = R"({
            "interruptModel" : {
                "Dialog" : {
                },
                "Communications" : {
                    "contentType":
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                },
                "Alert" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        }
                    }
                },
                "Content" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Alert" : {
                                    "incomingChannelType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Alert" : {
                                    "incomingChannelType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                },
                "VirtualChannel1" : {
                    "MIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MAY_DUCK"
                                }
                            },
                            "Communications" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MAY_DUCK",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Alert" : {
                                "incomingChannelType" : {
                                    "MIXABLE" : "MAY_DUCK"
                                }
                            }
                        }
                    }
                },
                "VirtualChannel2" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                    }
                                },
                                "Alert" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "InvalidMixingBehavior"
                                    }
                                }
                            }
                        }
                    }
                }
            }
})";

class InterruptModelTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    ConfigurationNode generateConfigFromJson(std::string json) {
        auto stream = std::shared_ptr<std::stringstream>(new std::stringstream(json));
        JSONStream jsonStream({stream});
        ConfigurationNode::initialize(jsonStream);
        return ConfigurationNode::getRoot();
    }

    std::shared_ptr<InterruptModel> m_interruptModel;
    ConfigurationNode m_configNode;
};

void InterruptModelTest::SetUp() {
    m_configNode = generateConfigFromJson(CONFIG_JSON);
    m_interruptModel = InterruptModel::create(m_configNode[INTERRUPT_MODEL_KEY]);
}

void InterruptModelTest::TearDown() {
    ConfigurationNode::uninitialize();
}

TEST_F(InterruptModelTest, test_emptyConfiguration) {
    auto emptyConfig = m_configNode[INVALID_CONFIG_KEY];
    auto interruptModel = InterruptModel::create(emptyConfig);
    ASSERT_EQ(interruptModel, nullptr);
}

TEST_F(InterruptModelTest, test_NonExistentChannelConfigReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        CONTENT_CHANNEL, NONMIXABLE_CONTENT_TYPE, NONEXISTENT_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);

    retMixingBehavior = m_interruptModel->getMixingBehavior(
        NONEXISTENT_CHANNEL, NONMIXABLE_CONTENT_TYPE, DIALOG_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_MissingContentTypeKeyReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        VIRTUAL_CHANNEL1, NONMIXABLE_CONTENT_TYPE, CONTENT_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_MissingMixingBehaviorKeyReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        CONTENT_CHANNEL, INVALID_CONTENT_TYPE, DIALOG_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_MissingConfigReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        DIALOG_CHANNEL, MIXABLE_CONTENT_TYPE, VIRTUAL_CHANNEL1, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_MissingIncomingChannelKeyReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        VIRTUAL_CHANNEL2, MIXABLE_CONTENT_TYPE, DIALOG_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_UnspecifiedMixingBehaviorKeyReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        ALERT_CHANNEL, NONMIXABLE_CONTENT_TYPE, DIALOG_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_UnspecifiedIncomingMixingBehaviorReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        VIRTUAL_CHANNEL2, NONMIXABLE_CONTENT_TYPE, DIALOG_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

TEST_F(InterruptModelTest, test_InvalidIncomingMixingBehaviorReturnsUndefined) {
    auto retMixingBehavior = m_interruptModel->getMixingBehavior(
        VIRTUAL_CHANNEL2, NONMIXABLE_CONTENT_TYPE, ALERT_CHANNEL, MIXABLE_CONTENT_TYPE);
    ASSERT_EQ(MixingBehavior::UNDEFINED, retMixingBehavior);
}

}  // namespace test
}  // namespace interruptModel
}  // namespace afml
}  // namespace alexaClientSDK
