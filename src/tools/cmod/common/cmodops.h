// cmodops.h
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Perform various adjustments to a Celestia mesh.

#pragma once

#include <cstdint>

#include <Eigen/Core>

#include <celmodel/mesh.h>


namespace cmod
{
class Model;
}


// Mesh operations
extern cmod::Mesh* GenerateNormals(const cmod::Mesh& mesh, float smoothAngle, bool weld, float weldTolerance = 0.0f);
extern cmod::Mesh* GenerateTangents(const cmod::Mesh& mesh, bool weld);
extern bool UniquifyVertices(cmod::Mesh& mesh);

// Model operations
extern cmod::Model* MergeModelMeshes(const cmod::Model& model);
extern cmod::Model* GenerateModelNormals(const cmod::Model& model, float smoothAngle, bool weldVertices, float weldTolerance);
#ifdef TRISTRIP
extern bool ConvertToStrips(cmod::Mesh& mesh);
#endif
