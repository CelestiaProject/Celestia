// modelfile.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMESH_MODELFILE_H_
#define _CELMESH_MODELFILE_H_

#include "model.h"
#include <iostream>
#include <string>

#define CEL_MODEL_HEADER_LENGTH 16
#define CEL_MODEL_HEADER_ASCII "#celmodel__ascii"
#define CEL_MODEL_HEADER_BINARY "#celmodel_binary"

class ModelLoader
{
 public:
    ModelLoader();
    virtual ~ModelLoader();

    virtual Model* load() = 0;

    const std::string& getErrorMessage() const;
    void setTexturePath(const std::string&);
    const std::string& getTexturePath() const;
    
    static ModelLoader* OpenModel(std::istream& in);

 protected:
    virtual void reportError(const std::string&);

 private:
    std::string errorMessage;
    std::string texPath;
};


class ModelWriter
{
 public:
    virtual ~ModelWriter() {};

    virtual bool write(const Model&) = 0;
};



Model* LoadModel(std::istream&);
Model* LoadModel(std::istream& in, const std::string& texPath);

bool SaveModelAscii(const Model* model, std::ostream& out);

#endif // !_CELMESH_MODEL_H_
