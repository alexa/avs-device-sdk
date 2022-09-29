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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_REORDERABLEUNIQUESTACK_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_REORDERABLEUNIQUESTACK_H_

#include <list>
#include <unordered_map>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

/// String to identify log entries originating from this file.
#define REORDERABLE_UNIQUE_STACK_TAG "ReorderableUniqueStack"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(REORDERABLE_UNIQUE_STACK_TAG, event)

/**
 * Implementation of a stack which contains distinct elements and allows elements to be erased from any position
 * within the stack.
 * User of this stack implementation should ensure thread-safety
 * @tparam T The type of object managed by this stack
 */
template <class T>
class ReorderableUniqueStack {
public:
    /**
     * Default Constructor
     */
    ReorderableUniqueStack() = default;

    /**
     * Pushes an element on to the top of the stack
     * @param element The element to push
     */
    void push(const T& element);

    /**
     * Retrieve the element at the top of the stack
     * @return element at the top of the stack
     */
    alexaClientSDK::avsCommon::utils::Optional<T> top();

    /**
     * Pop the element at the top of the stack
     */
    void pop();

    /**
     * Check if the stack contains any elements
     * @return true if the stack contains no elements
     */
    bool empty();

    /**
     * Erases the given element from the stack
     * @param element The element to erase
     */
    void erase(const T& element);

    /**
     * Clears the entire stack
     */
    void clear();

    /**
     * Moves a given entry to the top of the stack
     */
    void moveToTop(const T& element);

    /**
     * Returns the number of elements in the stack
     * @return size of stack
     */
    size_t size();

    /**
     * Returns whether the element is present in the stack
     * @return boolean for presence of element
     */
    bool contains(const T& element);

    /**
     * Returns element above specified element in stack
     * @return element above
     */
    alexaClientSDK::avsCommon::utils::Optional<T> above(const T& element);

private:
    using StackType = std::list<T>;
    StackType m_stack;
    std::unordered_map<T, typename StackType::iterator> m_entries;
};

template <class T>
void ReorderableUniqueStack<T>::push(const T& element) {
    if (m_entries.count(element)) {
        // Element already exists, push it to the top
        moveToTop(element);
        return;
    }
    m_stack.push_back(element);
    m_entries[element] = std::prev(m_stack.end());
}

template <class T>
alexaClientSDK::avsCommon::utils::Optional<T> ReorderableUniqueStack<T>::top() {
    if (m_stack.empty()) {
        ACSDK_DEBUG9(LX("topFailed").d("reason", "Attempt to access empty stack"));
        return alexaClientSDK::avsCommon::utils::Optional<T>();
    }
    return m_stack.back();
}

template <class T>
void ReorderableUniqueStack<T>::pop() {
    if (m_stack.empty()) {
        ACSDK_ERROR(LX("popFailed").d("reason", "Attempt to access empty stack"));
        return;
    }
    m_entries.erase(m_stack.back());
    m_stack.pop_back();
}

template <class T>
bool ReorderableUniqueStack<T>::empty() {
    return m_stack.empty();
}

template <class T>
void ReorderableUniqueStack<T>::erase(const T& element) {
    auto findIt = m_entries.find(element);
    if (findIt == m_entries.end()) {
        ACSDK_WARN(LX("eraseFailed").d("reason", "Element does not exist in stack"));
        return;
    }
    m_stack.erase(findIt->second);
    m_entries.erase(findIt);
}

template <class T>
void ReorderableUniqueStack<T>::clear() {
    m_entries.clear();
    m_stack.clear();
}

template <class T>
void ReorderableUniqueStack<T>::moveToTop(const T& element) {
    auto findIt = m_entries.find(element);
    if (findIt == m_entries.end()) {
        ACSDK_WARN(LX("moveToTopFailed").d("reason", "Element does not exist in stack"));
        return;
    }

    // Remove this element from its current position and place it on top of the stack (iterators remain valid)
    m_stack.splice(m_stack.end(), m_stack, findIt->second);
}

template <class T>
size_t ReorderableUniqueStack<T>::size() {
    return m_stack.size();
};

template <class T>
bool ReorderableUniqueStack<T>::contains(const T& element) {
    auto findIt = m_entries.find(element);
    return findIt != m_entries.end();
};

template <class T>
alexaClientSDK::avsCommon::utils::Optional<T> ReorderableUniqueStack<T>::above(const T& element) {
    auto findIt = m_entries.find(element);
    if (m_stack.empty()) {
        ACSDK_WARN(LX("aboveFailed").d("reason", "Attempt to access empty stack"));
        return alexaClientSDK::avsCommon::utils::Optional<T>();
    }

    if (element == m_stack.back()) {
        ACSDK_WARN(LX("aboveFailed").d("reason", "Specified element is top of stack"));
        return alexaClientSDK::avsCommon::utils::Optional<T>();
    }

    if (findIt == m_entries.end()) {
        ACSDK_WARN(LX("aboveFailed").d("reason", "Element does not exist in stack"));
        return alexaClientSDK::avsCommon::utils::Optional<T>();
    }

    return *std::next(findIt->second);
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_REORDERABLEUNIQUESTACK_H_

#undef LX