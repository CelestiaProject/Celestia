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

#define GL_GLEXT_PROTOTYPES
#include "gl.h"
#include "glext.h"
#include "regcombine.h"


void SetupCombinersBumpMap(Texture& bumpTexture,
                           Texture& normalizationTexture,
                           Color ambientColor)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    glDisable(GL_LIGHTING);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_EXT);
    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, normalizationTexture.getName());

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bumpTexture.getName());

    // Just a single combiner stage required . . .
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ambient[0] = ambientColor.red();
    ambient[1] = ambientColor.green();
    ambient[2] = ambientColor.blue();
    glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, ambient);

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
    // C = zero
    glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = ambient color
    glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = diffuse illumination contribution = L dot N
    glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
}


// Set up register combiners for per-pixel diffuse lighting, with a base
// texture, ambient color, material color, and normal cube map.  We could use
// just a plain old color cube map, but we use a normal map instead for
// consistency with bump mapped surfaces.  Only one pass with a single
// combiner is required.
void SetupCombinersSmooth(Texture& baseTexture,
                          Texture& normalizationTexture,
                          Color ambientColor,
                          bool invert)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    glDisable(GL_LIGHTING);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_EXT);
    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, normalizationTexture.getName());
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, baseTexture.getName());

    // Just a single combiner stage required . . .
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ambient[0] = ambientColor.red();
    ambient[1] = ambientColor.green();
    ambient[2] = ambientColor.blue();
    glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, ambient);

    // A = primary color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                      GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV,
                      GL_RGB);
    // B = base texture color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                      GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // SPARE1_rgb = primary * texture
    glCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                       GL_SPARE1_NV, GL_DISCARD_NV, GL_DISCARD_NV,
                       GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = 1
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV,
                      GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // B = L.z
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV,
                      GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV,
                      GL_BLUE);
    // SPARE0_alpha = 1 * L.z
    glCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA,
                       GL_SPARE0_NV, GL_DISCARD_NV, GL_DISCARD_NV,
                       GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // E = SPARE1_rgb = base texture color * primary
    glFinalCombinerInputNV(GL_VARIABLE_E_NV,
                           GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // F = ambient color
    glFinalCombinerInputNV(GL_VARIABLE_F_NV,
                           GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // A = SPARE1_rgb = base texture color * primary
    glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = SPARE0_alpha = L.z
    glFinalCombinerInputNV(GL_VARIABLE_B_NV,
                           GL_SPARE0_NV,
                           invert ? GL_UNSIGNED_INVERT_NV : GL_UNSIGNED_IDENTITY_NV,
                           GL_ALPHA);
    // C = zero
    glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = SPARE1_rgb = E*F = texture * primary * ambient color
    glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = 1
    glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                           GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);

}


void SetupCombinersDecalAndBumpMap(Texture& bumpTexture,
                                   Color ambientColor)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    bumpTexture.bind();
    glActiveTextureARB(GL_TEXTURE0_ARB);

    // Just a single combiner stage required . . .
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 2);

    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ambient[0] = ambientColor.red();
    ambient[1] = ambientColor.green();
    ambient[2] = ambientColor.blue();
    glCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, ambient);

    // Compute N dot L in the RGB portion of combiner 0
    // Load register A with a normal N from the bump map
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                      GL_VARIABLE_A_NV, GL_TEXTURE1_ARB,
                      GL_EXPAND_NORMAL_NV, GL_RGB);

    // Load register B with the primary color, which contains the surface
    // space light direction L.  Because the color is linearly interpolated
    // across triangles, the direction may become denormalized; however, in
    // Celestia, planet surfaces are tessellated to the point where this
    // is not a problem.
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                      GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV,
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
                      GL_PRIMARY_COLOR_NV, GL_EXPAND_NORMAL_NV, GL_BLUE);
    // C = 1
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV,
                      GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // D = L.z
    glCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV,
                      GL_PRIMARY_COLOR_NV, GL_EXPAND_NORMAL_NV, GL_BLUE);

    // Create a steep ramp function for self-shadowing
    // SPARE0 = 4*(A*B+C*D) = 4*(1*L.z + 1*L.z) = 8 * L.z
    glCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA,
                       GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                       GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // In the second combiner, sum the ambient color and product of the
    // diffuse and self-shadowing terms.
    
#if 1
    // A = SPARE0_alpha = per-pixel self-shadowing term
    glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    glFinalCombinerInputNV(GL_VARIABLE_B_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

    // E = SPARE0_alpha = per-pixel self-shadowing term
    glFinalCombinerInputNV(GL_VARIABLE_E_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    // F = SPARE0_rgb = diffuse illumination (L dot N)
    glFinalCombinerInputNV(GL_VARIABLE_F_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    glFinalCombinerInputNV(GL_

    // C = zero
    glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

    // D = ambient color
    glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = diffuse illumination contribution = L dot N
    glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
#endif
}


// Set up the combiners to a texture with gloss map in the alpha channel.
void SetupCombinersGlossMap()
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    // Just a single combiner stage required . . .
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    // A = primary color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                      GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = base texture color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                      GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = secondary color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV,
                      GL_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = texture alpha (gloss mask)
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                      GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);

    // SPARE0_rgb = primary * texture.rgb + secondary * texture.alpha
    glCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                       GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                       GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = SPARE0_rgb
    glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = 1
    glFinalCombinerInputNV(GL_VARIABLE_B_NV,
                           GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_RGB);
    // C = zero
    glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = zero
    glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = 1
    glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                           GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
}


// Set up the combiners to a texture with gloss in the alpha channel.
void SetupCombinersGlossMapWithFog()
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    // Just a single combiner stage required . . .
    glCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    // A = primary color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                      GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = base texture color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                      GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = secondary color
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV,
                      GL_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = texture alpha (gloss mask)
    glCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                      GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);

    // SPARE0_rgb = primary * texture.rgb + secondary * texture.alpha
    glCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                       GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                       GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = fog factor
    glFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    // B = spare0_rgb
    glFinalCombinerInputNV(GL_VARIABLE_B_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = fog color
    glFinalCombinerInputNV(GL_VARIABLE_C_NV,
                           GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = zero
    glFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = 1
    glFinalCombinerInputNV(GL_VARIABLE_G_NV,
                           GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
}


void DisableCombiners()
{
    glDisable(GL_REGISTER_COMBINERS_NV);
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_EXT);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
}
