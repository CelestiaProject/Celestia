// resmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_RESMANAGER_H_
#define _CELUTIL_RESMANAGER_H_

#include <string>
#include <vector>
#include <map>
#include <celutil/reshandle.h>


enum ResourceState {
    ResourceNotLoaded     = 0,
    ResourceLoaded        = 1,
    ResourceLoadingFailed = 2,
};


template<class T> class ResourceInfo
{
 public:
    ResourceInfo() : state(ResourceNotLoaded), resource(NULL) {};
    virtual ~ResourceInfo() {};

    virtual std::string resolve(const std::string&) = 0;
    virtual T* load(const std::string&) = 0;

    typedef T ResourceType;
    ResourceState state;
    std::string resolvedName;
    T* resource;
};


template<class T> class ResourceManager
{
 private:
    std::string baseDir;

 public:
    ResourceManager();
    ResourceManager(std::string _baseDir) : baseDir(_baseDir) {};
    ~ResourceManager();

    typedef typename T::ResourceType ResourceType;

 private:
    typedef std::vector<T> ResourceTable;
    typedef std::map<T, ResourceHandle> ResourceHandleMap;
    typedef std::map<std::string, ResourceType*> NameMap;

    typedef typename ResourceHandleMap::value_type ResourceHandleMapValue;
    typedef typename NameMap::value_type NameMapValue;

    ResourceTable resources;
    ResourceHandleMap handles;
    NameMap loadedResources;

 public:
    ResourceHandle getHandle(const T& info)
    {
        typename ResourceHandleMap::iterator iter = handles.find(info);
        if (iter != handles.end())
        {
            return iter->second;
        }
        else
        {
            ResourceHandle h = handles.size();
            resources.insert(resources.end(), info);
            handles.insert(ResourceHandleMapValue(info, h));
            return h;
        }
    }

    ResourceType* find(ResourceHandle h)
    {
        if (h >= (int) handles.size() || h < 0)
        {
            return NULL;
        }
        else
        {
            if (resources[h].state == ResourceNotLoaded)
            {
                resources[h].resolvedName = resources[h].resolve(baseDir);
                typename NameMap::iterator iter =
                    loadedResources.find(resources[h].resolvedName);
                if (iter != loadedResources.end())
                {
                    resources[h].resource = iter->second;
                    resources[h].state = ResourceLoaded;
                }
                else
                {
                    resources[h].resource = resources[h].load(resources[h].resolvedName);
                    if (resources[h].resource == NULL)
                    {
                        resources[h].state = ResourceLoadingFailed;
                    }
                    else
                    {
                        resources[h].state = ResourceLoaded;
                        loadedResources.insert(NameMapValue(resources[h].resolvedName, resources[h].resource));
                    }
                }
            }

            if (resources[h].state == ResourceLoaded)
                return resources[h].resource;
            else
                return NULL;
        }
    }

    const T* getResourceInfo(ResourceHandle h)
    {
        if (h >= (int) handles.size() || h < 0)
            return NULL;
        else
            return &resources[h];
    }
};

#endif // _CELUTIL_RESMANAGER_H_

