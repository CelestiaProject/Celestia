// model.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MODEL_H_
#define _CELENGINE_MODEL_H_

#include "mesh.h"

class Model
{
 public:
    Model();
    ~Model();

    const Mesh::Material* getMaterial(uint32) const;
    uint32 addMaterial(const Mesh::Material*);

    const Mesh* getMesh(uint32) const;
    uint32 addMesh(const Mesh*);

    bool pick(const Ray3d& r, double& distance) const;
    void render();

 private:
    std::vector<const Mesh::Material*> materials;
    std::vector<const Mesh*> meshes;
};

#endif // !_CELENGINE_MODEL_H_
