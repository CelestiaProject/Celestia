// resmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _RESMANAGER_H_
#define _RESMANAGER_H_

#include <string>
#include <vector>
#include <map>
#include "reshandle.h"


enum ResourceState {
    ResourceNotLoaded     = 0,
    ResourceLoaded        = 1,
    ResourceLoadingFailed = 2,
};


template<class T> class ResourceInfo
{
 public:
    ResourceInfo() : state(ResourceNotLoaded), resource(NULL) {};

    virtual T* load(const std::string&) = 0;

    typedef typename T ResourceType;
    ResourceState state;
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

    ResourceTable resources;
    ResourceHandleMap handles;

 public:
    ResourceHandle getHandle(const T& info)
    {
        ResourceHandleMap::iterator iter = handles.find(info);
        if (iter != handles.end())
        {
            return iter->second;
        }
        else
        {
            ResourceHandle h = handles.size();
            resources.insert(resources.end(), info);
            handles.insert(ResourceHandleMap::value_type(info, h));
            return h;
        }
    }

    ResourceType* find(ResourceHandle h)
    {
        if (h >= handles.size() || h < 0)
        {
            return NULL;
        }
        else
        {
            if (resources[h].state == ResourceNotLoaded)
            {
                resources[h].resource = resources[h].load(baseDir);
                if (resources[h].resource == NULL)
                    resources[h].state = ResourceLoadingFailed;
                else
                    resources[h].state = ResourceLoaded;
            }

            if (resources[h].state == ResourceLoaded)
                return resources[h].resource;
            else
                return NULL;
        }
    }

};

#endif // _RESMANAGER_H_

