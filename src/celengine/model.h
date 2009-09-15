// model.h
//
// Copyright (C) 2004-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MODEL_H_
#define _CELENGINE_MODEL_H_

#include "mesh.h"

class Geometry
{
public:
    Geometry() {};
    virtual ~Geometry() {};

    //! Render the geometry in the specified OpenGL context
    virtual void render(RenderContext& rc, double t = 0.0) = 0;

    /*! Find the closest intersection between the ray and the
     *  model.  If the ray intersects the model, return true
     *  and set distance; otherwise return false and leave
     *  distance unmodified.
     */
    virtual bool pick(const Ray3d& r, double& distance) const = 0;
    
    virtual bool isOpaque() const = 0;

    virtual bool isNormalized() const
    {
        return true;
    }
    
    /*! Return true if the specified texture map type is used at
     *  all within this geometry object. This information is used
     *  to decide whether multiple rendering passes are required.
     */
    virtual bool usesTextureType(Mesh::TextureSemantic) const
    {
        return false;
    }

    /*! Load all textures used by the model. */
    virtual void loadTextures()
    {
    }
};


/*!
 * Model is the standard geometry object in Celestia.  A Model
 * consists of a library of materials together with a list of
 * meshes.  Each mesh object contains a pool of vertices and a
 * set of primitive groups.  A primitive groups consists of a
 * a primitive group type and a list of vertex indices.  This
 * structure is exactly the one used in Celestia model (.cmod)
 * files.
 */
class Model : public Geometry
{
 public:
    Model();
    ~Model();

    /*! Return the material with the specified index, or NULL if
     *  the index is out of range.
     */
    const Mesh::Material* getMaterial(uint32) const;

    /*! Add a new material to the model's material library; the
     *  return value is the number of materials in the model.
     */
    uint32 addMaterial(const Mesh::Material*);

    /*! Return the number of materials in the model
     */
    uint32 getMaterialCount() const;

    /*! Return the total number of vertices in the model
     */
    uint32 getVertexCount() const;

    /*! Return the total number of primitives in the model
     */
    uint32 getPrimitiveCount() const;

    /*! Return the mesh with the specified index, or NULL if the
     *  index is out of range.
     */
    Mesh* getMesh(uint32) const;

    /*! Add a new mesh to the model; the return value is the
     *  total number of meshes in the model.
     */
    uint32 addMesh(Mesh*);

    /*! Find the closest intersection between the ray and the
     *  model.  If the ray intersects the model, return true
     *  and set distance; otherwise return false and leave
     *  distance unmodified.
     */
    virtual bool pick(const Ray3d& r, double& distance) const;

    //! Render the model in the current OpenGL context
    virtual void render(RenderContext&, double t = 0.0);

    void transform(const Eigen::Vector3f& translation, float scale);

    /*! Apply a uniform scale to the model so that it fits into
     *  a box with a center at centerOffset and a maximum side
     *  length of one.
     */
    void normalize(const Eigen::Vector3f& centerOffset);

    /*! Return true if the specified texture map type is used at
     *  all within a mesh. This information is used to decide
     *  if multiple rendering passes are required.
     */
    virtual bool usesTextureType(Mesh::TextureSemantic) const;

    /*! Return true if the model has no translucent components. */
    virtual bool isOpaque() const
    {
        return opaque;
    }

    virtual bool isNormalized() const
    {
        return normalized;
    }

    /*! Set the opacity flag based on material usage within the model */
    void determineOpacity();


    class MeshComparator
    {
     public:
        virtual ~MeshComparator() {};

        virtual bool operator()(const Mesh&, const Mesh&) const = 0;
    };

    /*! Sort the model's meshes in place. */
    void sortMeshes(const MeshComparator&);

    /*! Optimize the model by eliminating all duplicated materials */
    void uniquifyMaterials();

    void loadTextures();

    /*! This comparator will roughly sort the model's meshes by
     *  opacity so that transparent meshes are rendered last.  It's far
     *  from perfect, but covers a lot of cases.  A better method of
     *  opacity sorting would operate at the primitive group level, or
     *  even better at the triangle level.
     *
     *  Standard usage for this class is:
     *     model->sortMeshes(Model::OpacityComparator());
     *
     *  uniquifyMaterials() should be used before sortMeshes(), since
     *  the opacity comparison depends on material indices being ordered
     *  by opacity.
     */
    class OpacityComparator : public MeshComparator
    {
     public:
        OpacityComparator();
        virtual ~OpacityComparator() {};

        virtual bool operator()(const Mesh&, const Mesh&) const;

     private:
        int unused;
    };

 private:
    std::vector<const Mesh::Material*> materials;
    std::vector<Mesh*> meshes;

    bool textureUsage[Mesh::TextureSemanticMax];
    bool opaque;
    bool normalized;
};

#endif // !_CELENGINE_MODEL_H_
