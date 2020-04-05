// renderinfo.h
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

struct RenderInfo
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

#ifdef HDR_COMPRESS
    Color color{ 0.5f, 0.5f, 0.5f };
#else
    Color color{ 1.0f, 1.0f, 1.0f };
#endif
    Texture* baseTex{ nullptr };
    Texture* bumpTex{ nullptr };
    Texture* nightTex{ nullptr };
    Texture* glossTex{ nullptr };
    Texture* overlayTex{ nullptr };
    Color specularColor{ 0.0f, 0.0f, 0.0f };
    float specularPower{ 0.0f };
    Eigen::Vector3f sunDir_eye{ Eigen::Vector3f::UnitZ() };
    Eigen::Vector3f sunDir_obj{ Eigen::Vector3f::UnitZ() };
    Eigen::Vector3f eyeDir_obj{ Eigen::Vector3f::UnitZ() };
    Eigen::Vector3f eyePos_obj{ Eigen::Vector3f::Zero() };
    Color sunColor{ 1.0f, 1.0f, 1.0f };
    Color ambientColor{ 0.0f, 0.0f, 0.0f };
#ifdef USE_HDR
    float nightLightScale{ 1.0f };
#endif
    float lunarLambert{ 0.0f };
    Eigen::Quaternionf orientation{ Eigen::Quaternionf::Identity() };
    float pixWidth{ 1.0f };
    float pointScale{ 1.0f };
};

extern LODSphereMesh* g_lodSphere;

