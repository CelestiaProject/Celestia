
#pragma once

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <set>

#define SHARED_TYPES(T) \
    typedef typename std::shared_ptr<T> SharedPtr; \
    typedef typename std::shared_ptr<const T> SharedConstPtr; \
    typedef typename std::unordered_map<T*, SharedPtr> SelfPtrMap; \
    typedef typename std::unordered_set<SharedPtr> UnorderedSet; \
    typedef typename std::set<SharedPtr> Set;
