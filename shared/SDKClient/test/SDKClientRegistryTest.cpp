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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "acsdk/SDKClient/SDKClientBuilder.h"
#include "acsdk/SDKClient/SDKClientRegistry.h"

namespace alexaClientSDK {
namespace sdkClient {
namespace test {

using namespace ::testing;

struct MockComponent {};
struct MockComponent2 {};
struct Annotation1 {};
struct Annotation2 {};

/// Mock feature
class MockFeatureNoReqs : public FeatureClientInterface {
public:
    MockFeatureNoReqs() : FeatureClientInterface("MockFeatureNoReqs") {
    }
    MOCK_METHOD1(configure, bool(const std::shared_ptr<sdkClient::SDKClientRegistry>&));
    MOCK_METHOD0(doShutdown, void());
};

class MockFeatureBuilderNoReqs : public FeatureClientBuilderInterface {
public:
    MockFeatureBuilderNoReqs() {
    }
    MOCK_METHOD1(construct, std::shared_ptr<MockFeatureNoReqs>(const std::shared_ptr<SDKClientRegistry>& client));
    MOCK_METHOD0(name, std::string());
};

// Mock feature with two required types
class MockFeatureTwoReqs : public FeatureClientInterface {
public:
    MockFeatureTwoReqs() : FeatureClientInterface("MockFeatureTwoReqs") {
    }
    MOCK_METHOD1(configure, bool(const std::shared_ptr<sdkClient::SDKClientRegistry>&));
    MOCK_METHOD0(doShutdown, void());
};

class MockFeatureBuilderTwoReqs : public FeatureClientBuilderInterface {
public:
    MockFeatureBuilderTwoReqs() {
        addRequiredType<MockComponent>();
        addRequiredType<MockComponent2>();
    }
    MOCK_METHOD1(construct, std::shared_ptr<MockFeatureTwoReqs>(const std::shared_ptr<SDKClientRegistry>& client));
    MOCK_METHOD0(name, std::string());
};

/// Test harness for @c TemplateRuntime class.
class SDKClientRegistryTest : public ::testing::Test {
public:
    template <typename T, typename Y>
    std::unique_ptr<T> generateFeatureBuilder(std::shared_ptr<Y> constructedClient) {
        auto underlyingBuilder = new T();
        EXPECT_CALL(*underlyingBuilder, construct(_)).Times(1).WillOnce(Return(constructedClient));
        ON_CALL(*underlyingBuilder, name()).WillByDefault(Return("BuilderName"));
        return std::unique_ptr<T>(underlyingBuilder);
    }

    template <typename T>
    std::unique_ptr<T> generateFeatureClientWithExpectations(bool configureSuccess) {
        auto underlyingFeature = new T();

        EXPECT_CALL(*underlyingFeature, configure(_)).Times(1).WillOnce(Return(configureSuccess));

        return std::unique_ptr<T>(underlyingFeature);
    }
};

/**
 * Tests that an SDKClientRegistry can be constructed using the SDKClientBuilder
 */
TEST_F(SDKClientRegistryTest, test_buildSimpleClient) {
    SDKClientBuilder builder;
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderNoReqs, MockFeatureNoReqs>(
        generateFeatureClientWithExpectations<MockFeatureNoReqs>(true)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
}

/**
 * Tests the SDKClientBuilder does not add a null feature
 */
TEST_F(SDKClientRegistryTest, test_addNullFeature) {
    SDKClientBuilder builder;
    std::unique_ptr<MockFeatureBuilderNoReqs> nullFeature;
    builder.withFeature(std::move(nullFeature));
    auto client = builder.build();
    EXPECT_EQ(client, nullptr);
}

/**
 * Tests that the builder fails if construction of a dependency fails
 */
TEST_F(SDKClientRegistryTest, test_buildSimpleClientConstructionFailure) {
    SDKClientBuilder builder;
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderNoReqs, MockFeatureNoReqs>(nullptr));
    auto client = builder.build();
    EXPECT_EQ(client, nullptr);
}

/**
 * Tests that the builder fails if configuration of a dependency fails
 */
TEST_F(SDKClientRegistryTest, test_buildSimpleClientConfigurationFailure) {
    SDKClientBuilder builder;
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderNoReqs, MockFeatureNoReqs>(
        generateFeatureClientWithExpectations<MockFeatureNoReqs>(false)));
    auto client = builder.build();
    EXPECT_EQ(client, nullptr);
}

/**
 * Tests that an SDKClientRegistry successfully resolves dependencies
 */
TEST_F(SDKClientRegistryTest, test_buildDependentClient) {
    SDKClientBuilder builder;
    auto component1 = std::make_shared<MockComponent>();
    auto component2 = std::make_shared<MockComponent2>();
    auto feature1 = std::make_shared<MockFeatureNoReqs>();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([component1, component2, feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            client->registerComponent(component1);
            client->registerComponent(component2);
            return feature1;
        }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(feature1Builder));
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderTwoReqs, MockFeatureTwoReqs>(
        generateFeatureClientWithExpectations<MockFeatureTwoReqs>(true)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
}

/**
 * Tests that an SDKClientRegistry successfully resolves dependencies when they are added in a different order
 */
TEST_F(SDKClientRegistryTest, test_buildDependentClientReversedOrder) {
    SDKClientBuilder builder;
    auto component1 = std::make_shared<MockComponent>();
    auto component2 = std::make_shared<MockComponent2>();
    auto feature1 = std::make_shared<MockFeatureNoReqs>();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([component1, component2, feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            client->registerComponent(component1);
            client->registerComponent(component2);
            return feature1;
        }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderTwoReqs, MockFeatureTwoReqs>(
        generateFeatureClientWithExpectations<MockFeatureTwoReqs>(true)));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(feature1Builder));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
}

/**
 * Tests that an SDKClientRegistry is not returned if a dependency is unsatisfied
 */
TEST_F(SDKClientRegistryTest, test_buildDependentClientUnsatisfied) {
    SDKClientBuilder builder;
    auto noReqFeatureBuilder =
        generateFeatureBuilder<MockFeatureBuilderNoReqs, MockFeatureNoReqs>(std::make_shared<MockFeatureNoReqs>());
    auto twoReqFeatureBuilder = new MockFeatureBuilderTwoReqs();
    EXPECT_CALL(*twoReqFeatureBuilder, construct(_)).Times(0);
    ON_CALL(*twoReqFeatureBuilder, name()).WillByDefault(Return("MockFeatureBuilderTwoReqs"));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(noReqFeatureBuilder)));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderTwoReqs>(twoReqFeatureBuilder));
    auto client = builder.build();
    EXPECT_EQ(client, nullptr);
}

/**
 * Test that a component can be retrieved
 */
TEST_F(SDKClientRegistryTest, test_getComponent) {
    SDKClientBuilder builder;
    auto component1 = std::make_shared<MockComponent>();
    auto feature1 = std::make_shared<MockFeatureNoReqs>();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([component1, feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            client->registerComponent(component1);
            return feature1;
        }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(feature1Builder)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
    EXPECT_EQ(client->getComponent<MockComponent>(), component1);
    EXPECT_EQ(client->getComponent<MockComponent2>(), nullptr);
}

/**
 * Test that a client can be retrieved
 */
TEST_F(SDKClientRegistryTest, test_getClient) {
    SDKClientBuilder builder;
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderNoReqs, MockFeatureNoReqs>(
        generateFeatureClientWithExpectations<MockFeatureNoReqs>(true)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
    EXPECT_NE(client->get<MockFeatureNoReqs>(), nullptr);
}

/**
 * Test that annotated components function correctly
 */
TEST_F(SDKClientRegistryTest, test_annotatedComponent) {
    SDKClientBuilder builder;
    auto component1 = std::make_shared<MockComponent>();
    auto component2 = std::make_shared<MockComponent>();
    auto component3 = std::make_shared<MockComponent>();
    auto feature1 = new MockFeatureNoReqs();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(
            Invoke([component1, component2, component3, feature1](const std::shared_ptr<SDKClientRegistry>& client) {
                client->registerComponent(Annotated<Annotation1, MockComponent>(component1));
                client->registerComponent(Annotated<Annotation2, MockComponent>(component2));
                client->registerComponent(component3);
                return std::shared_ptr<MockFeatureNoReqs>(feature1);
            }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(feature1Builder)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);

    auto retrievedComponent1 = client->getComponent<Annotated<Annotation1, MockComponent>>();
    auto retrievedComponent2 = client->getComponent<Annotated<Annotation2, MockComponent>>();
    EXPECT_EQ(retrievedComponent1, component1);
    EXPECT_EQ(retrievedComponent2, component2);
    EXPECT_EQ(client->getComponent<MockComponent>(), component3);
}

/**
 * Test that duplicate clients are not registered
 */
TEST_F(SDKClientRegistryTest, test_duplicateClient) {
    SDKClientBuilder builder;
    auto duplicateFeatureBuilder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*duplicateFeatureBuilder, construct(_)).Times(0);
    builder.withFeature(generateFeatureBuilder<MockFeatureBuilderNoReqs, MockFeatureNoReqs>(
        generateFeatureClientWithExpectations<MockFeatureNoReqs>(true)));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(duplicateFeatureBuilder)));
    auto client = builder.build();
}

/**
 * Test that duplicate components are not registered
 */
TEST_F(SDKClientRegistryTest, test_duplicateComponent) {
    SDKClientBuilder builder;
    auto component1 = std::make_shared<MockComponent>();
    auto feature1 = new MockFeatureNoReqs();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([component1, feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            EXPECT_TRUE(client->registerComponent(component1));
            EXPECT_FALSE(client->registerComponent(component1));
            return std::shared_ptr<MockFeatureNoReqs>(feature1);
        }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(feature1Builder)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
}

/**
 * Test that a client can be registered post build
 */
TEST_F(SDKClientRegistryTest, test_addFeature) {
    SDKClientBuilder builder;
    auto feature1 = new MockFeatureNoReqs();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            client->registerComponent(std::make_shared<MockComponent>());
            client->registerComponent(std::make_shared<MockComponent2>());
            return std::shared_ptr<MockFeatureNoReqs>(feature1);
        }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(feature1Builder)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);

    EXPECT_TRUE(client->addFeature(generateFeatureBuilder<MockFeatureBuilderTwoReqs, MockFeatureTwoReqs>(
        generateFeatureClientWithExpectations<MockFeatureTwoReqs>(true))));
    EXPECT_NE(client->get<MockFeatureTwoReqs>(), nullptr);
}

/**
 * Test that attempting to add a feature post build fails if the requirements are unsatisfied
 */
TEST_F(SDKClientRegistryTest, test_addFeaturesRequirementsUnsatisfied) {
    SDKClientBuilder builder;
    auto feature1 = new MockFeatureNoReqs();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            client->registerComponent(std::make_shared<MockComponent>());
            return std::shared_ptr<MockFeatureNoReqs>(feature1);
        }));
    EXPECT_CALL(*feature1, configure(_)).Times(1).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(std::move(feature1Builder)));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);

    auto feature2Builder = new MockFeatureBuilderTwoReqs();
    EXPECT_CALL(*feature2Builder, construct(_)).Times(0);
    EXPECT_FALSE(client->addFeature(std::unique_ptr<MockFeatureBuilderTwoReqs>(std::move(feature2Builder))));
    EXPECT_EQ(client->get<MockFeatureTwoReqs>(), nullptr);
}

/**
 * Tests that clients are shutdown in reverse order of construction
 */
TEST_F(SDKClientRegistryTest, test_shutdown) {
    SDKClientBuilder builder;
    auto component1 = std::make_shared<MockComponent>();
    auto component2 = std::make_shared<MockComponent2>();
    auto feature1 = std::make_shared<MockFeatureNoReqs>();
    auto feature1Builder = new MockFeatureBuilderNoReqs();
    EXPECT_CALL(*feature1Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([component1, component2, feature1](const std::shared_ptr<SDKClientRegistry>& client) {
            client->registerComponent(component1);
            client->registerComponent(component2);
            return feature1;
        }));
    EXPECT_CALL(*feature1, configure(_)).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderNoReqs>(feature1Builder));

    auto feature2 = std::make_shared<MockFeatureTwoReqs>();
    auto feature2Builder = new MockFeatureBuilderTwoReqs();
    EXPECT_CALL(*feature2Builder, construct(_))
        .Times(1)
        .WillOnce(Invoke([feature2](const std::shared_ptr<SDKClientRegistry>& client) { return feature2; }));
    EXPECT_CALL(*feature2, configure(_)).WillOnce(Return(true));
    builder.withFeature(std::unique_ptr<MockFeatureBuilderTwoReqs>(feature2Builder));
    auto client = builder.build();
    EXPECT_NE(client, nullptr);
    Sequence seq;
    EXPECT_CALL(*feature2, doShutdown()).InSequence(seq);
    EXPECT_CALL(*feature1, doShutdown()).InSequence(seq);
    client->shutdown();
}

}  // namespace test
}  // namespace sdkClient
}  // namespace alexaClientSDK
