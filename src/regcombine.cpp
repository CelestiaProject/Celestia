// regcombine.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Some functions for setting up the nVidia register combiners
// extension for pretty rendering effects.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "gl.h"
#include "glext.h"
#include "regcombine.h"


void SetupCombinersBumpMap(CTexture& bumpTexture,
                           CTexture& normalizationTexture,
                           float* ambientColor)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    glDisable(GL_LIGHTING);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_EXT);
    glBindTexture(GL_TEXTURE_2D, normalizationTexture.getName());

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bumpTexture.getName());

    // Just a single combiner stage required . . .
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, ambientColor);

    // Compute N dot L in the RGB portion of combiner 0
    // Load register A with a normal N from the normal map
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                      GL_VARIABLE_A_NV, GL_TEXTURE0_ARB,
                      GL_EXPAND_NORMAL_NV, GL_RGB);

    // Load register B with the normalized light direction L
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                      GL_VARIABLE_B_NV, GL_TEXTURE1_ARB,
                      GL_EXPAND_NORMAL_NV, GL_RGB);
    
    // Compute N dot L
    glCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                       GL_SPARE0_NV, GL_DISCARD_NV, GL_DISCARD_NV,
                       GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE);

    // Compute the self-shadowing term in the alpha portion of combiner 0
    // A = 1
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV,
                      GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // B = L.z
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV,
                      GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);
    // C = 1
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV,
                      GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // D = L.z
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV,
                      GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);

    // Create a steep ramp function for self-shadowing
    // SPARE0 = 4*(A*B+C*D) = 4*(1*L.z + 1*L.z) = 8 * L.z
    glCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA,
                       GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                       GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = SPARE0_alpha = per-pixel self-shadowing term
    glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    glFinalCombinerInputNV(GL_VARIABLE_B_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = ambient color
    glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = diffuse illumination contribution = L dot N
    glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
}


void DisableCombiners()
{
    glDisable(GL_REGISTER_COMBINERS_NV);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_EXT);
    glActiveTextureARB(GL_TEXTURE0_ARB);
}
