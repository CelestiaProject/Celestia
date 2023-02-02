// resmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <celcompat/filesystem.h>
#include <celutil/reshandle.h>


enum class ResourceState {
    NotLoaded     = 0,
    Loaded        = 1,
    LoadingFailed = 2,
};


template<class T> class ResourceManager
{
 private:
    fs::path baseDir;

 public:
    explicit ResourceManager(const fs::path& _baseDir) : baseDir(_baseDir) {};
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    using ResourceType = typename T::ResourceType;

 private:
    using KeyType = typename T::ResourceKey;

    struct InfoType
    {
        T info;
        ResourceState state{ ResourceState::NotLoaded };
        std::shared_ptr<ResourceType> resource{ nullptr };

        explicit InfoType(T _info) : info(std::move(_info)) {}
        InfoType(const InfoType&) = delete;
        InfoType& operator=(const InfoType&) = delete;
        InfoType(InfoType&&) noexcept = default;
        InfoType& operator=(InfoType&&) noexcept = default;

        KeyType resolve(const fs::path& _baseDir) { return info.resolve(_baseDir); }
        bool load(const KeyType& resolvedKey)
        {
            resource = info.load(resolvedKey);
            return resource != nullptr;
        }
    };

    using ResourceTable = std::vector<InfoType>;
    using ResourceHandleMap = std::map<T, ResourceHandle>;
    using NameMap = std::map<KeyType, std::weak_ptr<ResourceType>>;

    ResourceTable resources;
    ResourceHandleMap handles;
    NameMap loadedResources;

    void loadResource(InfoType& info)
    {
        KeyType resolvedKey = info.resolve(baseDir);
        std::shared_ptr<ResourceType> resource = nullptr;
        if (auto iter = loadedResources.find(resolvedKey); iter != loadedResources.end())
            resource = iter->second.lock();

        if (resource != nullptr)
        {
            info.resource = std::move(resource);
            info.state = ResourceState::Loaded;
        }
        else if (info.load(resolvedKey))
        {
            info.state = ResourceState::Loaded;
            if (auto [iter, inserted] = loadedResources.try_emplace(std::move(resolvedKey), info.resource); !inserted)
                iter->second = info.resource;
        }
        else
        {
            info.state = ResourceState::LoadingFailed;
        }
    }

 public:
    ResourceHandle getHandle(const T& info)
    {
        auto h = static_cast<ResourceHandle>(handles.size());
        if (auto [iter, inserted] = handles.try_emplace(info, h); inserted)
        {
            resources.emplace_back(info);
            return h;
        }
        else
        {
            return iter->second;
        }
    }

    ResourceType* find(ResourceHandle h)
    {
        if (h < 0 || h >= static_cast<ResourceHandle>(handles.size()))
        {
            return nullptr;
        }

        if (resources[h].state == ResourceState::NotLoaded)
        {
            loadResource(resources[h]);
        }

        return resources[h].state == ResourceState::Loaded
            ? resources[h].resource.get()
            : nullptr;
    }
};
