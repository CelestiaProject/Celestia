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

#if 0
namespace rc
{
    enum {
        Combiner0 = GL_COMBINER0_NV,
        Combiner1 = GL_COMBINER1_NV,
        Combiner2 = GL_COMBINER2_NV,
        Combiner3 = GL_COMBINER3_NV,
    };

    enum {
        A  = GL_VARIABLE_A_NV,
        B  = GL_VARIABLE_B_NV,
        C  = GL_VARIABLE_C_NV,
        D  = GL_VARIABLE_D_NV,
        E  = GL_VARIABLE_E_NV,
        F  = GL_VARIABLE_F_NV,
        G  = GL_VARIABLE_G_NV,
    };

    enum {
        RGBPortion   = GL_RGB,
        AlphaPortion = GL_ALPHA,
        BluePortion  = GL_BLUE,
    };

    enum {
        UnsignedIdentity    = GL_UNSIGNED_IDENTITY_NV,
        UnsignedInvert      = GL_UNSIGNED_INVERT_NV,
        ExpandNormal        = GL_EXPAND_NORMAL_NV,
    };
};
#endif

namespace rc
{
    void parameter(GLenum, Color);
};


void rc::parameter(GLenum which, Color color)
{
    float f[4];
    f[0] = color.red();
    f[1] = color.green();
    f[2] = color.blue();
    f[3] = color.alpha();
    EXTglCombinerParameterfvNV(which, f);
}

void SetupCombinersBumpMap(Texture& bumpTexture,
                           Texture& normalizationTexture,
                           Color ambientColor)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    glDisable(GL_LIGHTING);
    EXTglActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_EXT);
    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, normalizationTexture.getName());

    EXTglActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bumpTexture.getName());

    // Just a single combiner stage required . . .
    EXTglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ambient[0] = ambientColor.red();
    ambient[1] = ambientColor.green();
    ambient[2] = ambientColor.blue();
    EXTglCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, ambient);

    // Compute N dot L in the RGB portion of combiner 0
    // Load register A with a normal N from the normal map
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                         GL_VARIABLE_A_NV, GL_TEXTURE0_ARB,
                         GL_EXPAND_NORMAL_NV, GL_RGB);

    // Load register B with the normalized light direction L
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                         GL_VARIABLE_B_NV, GL_TEXTURE1_ARB,
                         GL_EXPAND_NORMAL_NV, GL_RGB);

    // Compute N dot L
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                          GL_SPARE0_NV, GL_DISCARD_NV, GL_DISCARD_NV,
                          GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE);

    // Compute the self-shadowing term in the alpha portion of combiner 0
    // A = 1
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV,
                         GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // B = L.z
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV,
                         GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);
    // C = 1
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV,
                         GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // D = L.z
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV,
                         GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV, GL_BLUE);

    // Create a steep ramp function for self-shadowing
    // SPARE0 = 4*(A*B+C*D) = 4*(1*L.z + 1*L.z) = 8 * L.z
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA,
                          GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                          GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = SPARE0_alpha = per-pixel self-shadowing term
    EXTglFinalCombinerInputNV(GL_VARIABLE_A_NV,
                              GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    EXTglFinalCombinerInputNV(GL_VARIABLE_B_NV,
                              GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = zero
    EXTglFinalCombinerInputNV(GL_VARIABLE_C_NV,
                              GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = ambient color
    EXTglFinalCombinerInputNV(GL_VARIABLE_D_NV,
                              GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = diffuse illumination contribution = L dot N
    EXTglFinalCombinerInputNV(GL_VARIABLE_G_NV,
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
    EXTglActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_CUBE_MAP_EXT);
    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, normalizationTexture.getName());
    EXTglActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, baseTexture.getName());

    // Just a single combiner stage required . . .
    EXTglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    float ambient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ambient[0] = ambientColor.red();
    ambient[1] = ambientColor.green();
    ambient[2] = ambientColor.blue();
    EXTglCombinerParameterfvNV(GL_CONSTANT_COLOR0_NV, ambient);

    // A = primary color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                         GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV,
                         GL_RGB);
    // B = base texture color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                         GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // SPARE1_rgb = primary * texture
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                          GL_SPARE1_NV, GL_DISCARD_NV, GL_DISCARD_NV,
                          GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = 1
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV,
                         GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // B = L.z
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV,
                         GL_TEXTURE1_ARB, GL_EXPAND_NORMAL_NV,
                         GL_BLUE);
    // SPARE0_alpha = 1 * L.z
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA,
                          GL_SPARE0_NV, GL_DISCARD_NV, GL_DISCARD_NV,
                          GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // E = SPARE1_rgb = base texture color * primary
    EXTglFinalCombinerInputNV(GL_VARIABLE_E_NV,
                              GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // F = ambient color
    EXTglFinalCombinerInputNV(GL_VARIABLE_F_NV,
                              GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // A = SPARE1_rgb = base texture color * primary
    EXTglFinalCombinerInputNV(GL_VARIABLE_A_NV,
                              GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = SPARE0_alpha = L.z
    EXTglFinalCombinerInputNV(GL_VARIABLE_B_NV,
                              GL_SPARE0_NV,
                              invert ? GL_UNSIGNED_INVERT_NV : GL_UNSIGNED_IDENTITY_NV,
                              GL_ALPHA);
    // C = zero
    EXTglFinalCombinerInputNV(GL_VARIABLE_C_NV,
                              GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = SPARE1_rgb = E*F = texture * primary * ambient color
    EXTglFinalCombinerInputNV(GL_VARIABLE_D_NV,
                              GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = 1
    EXTglFinalCombinerInputNV(GL_VARIABLE_G_NV,
                              GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);

}


void SetupCombinersDecalAndBumpMap(Texture& bumpTexture,
                                   Color ambientColor,
                                   Color diffuseColor)
{
    glEnable(GL_REGISTER_COMBINERS_NV);
#if 0
    EXTglActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    bumpTexture.bind();
    EXTglActiveTextureARB(GL_TEXTURE0_ARB);
#endif
    EXTglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 2);

    rc::parameter(GL_CONSTANT_COLOR0_NV, ambientColor);
    rc::parameter(GL_CONSTANT_COLOR1_NV, diffuseColor);

    // Compute N dot L in the RGB portion of combiner 0
    // Load register A with a normal N from the bump map
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                         GL_VARIABLE_A_NV, GL_TEXTURE1_ARB,
                         GL_EXPAND_NORMAL_NV, GL_RGB);

    // Load register B with the primary color, which contains the surface
    // space light direction L.  Because the color is linearly interpolated
    // across triangles, the direction may become denormalized; however, in
    // Celestia, planet surfaces are tessellated to the point where this
    // is not a problem.
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                         GL_VARIABLE_B_NV, GL_PRIMARY_COLOR_NV,
                         GL_EXPAND_NORMAL_NV, GL_RGB);

    // Product C*D computes diffuse color * texture
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                         GL_VARIABLE_C_NV, GL_TEXTURE0_ARB,
                         GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB,
                         GL_VARIABLE_D_NV, GL_CONSTANT_COLOR1_NV,
                         GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    
    // Compute N dot L in spare0 and diffuse * decal texture in spare1
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                          GL_SPARE0_NV, GL_SPARE1_NV, GL_DISCARD_NV,
                          GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE);

    // Compute the self-shadowing term in the alpha portion of combiner 0
    // A = 1
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_A_NV,
                         GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // B = L.z
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_B_NV,
                         GL_PRIMARY_COLOR_NV, GL_EXPAND_NORMAL_NV, GL_BLUE);
    // C = 1
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_C_NV,
                         GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
    // D = L.z
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_ALPHA, GL_VARIABLE_D_NV,
                         GL_PRIMARY_COLOR_NV, GL_EXPAND_NORMAL_NV, GL_BLUE);

    // Create a steep ramp function for self-shadowing
    // SPARE0 = 4*(A*B+C*D) = 4*(1*L.z + 1*L.z) = 8 * L.z
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_ALPHA,
                          GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                          GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // In the second combiner, sum the ambient color and product of the
    // diffuse and self-shadowing terms.
    EXTglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_A_NV,
                         GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    EXTglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_B_NV,
                         GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    EXTglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_C_NV,
                         GL_CONSTANT_COLOR0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    EXTglCombinerInputNV(GL_COMBINER1_NV, GL_RGB, GL_VARIABLE_D_NV,
                         GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_RGB);
    EXTglCombinerOutputNV(GL_COMBINER1_NV, GL_RGB,
                          GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                          GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);
    
    // E = SPARE0 = fragment brightness, including ambient, diffuse, and
    // self shadowing.
    EXTglFinalCombinerInputNV(GL_VARIABLE_E_NV,
                              GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // F = spare1 = decal texture rgb * diffuse color
    EXTglFinalCombinerInputNV(GL_VARIABLE_F_NV,
                              GL_SPARE1_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

    // A = fog factor
    EXTglFinalCombinerInputNV(GL_VARIABLE_A_NV,
                              GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    // B = color
    EXTglFinalCombinerInputNV(GL_VARIABLE_B_NV,
                              GL_E_TIMES_F_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = fog color
    EXTglFinalCombinerInputNV(GL_VARIABLE_C_NV,
                              GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = zero
    EXTglFinalCombinerInputNV(GL_VARIABLE_D_NV,
                              GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

    // G = diffuse illumination contribution = L dot N
    EXTglFinalCombinerInputNV(GL_VARIABLE_G_NV,
                              GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
}


// Set up the combiners to a texture with gloss map in the alpha channel.
void SetupCombinersGlossMap(int glossMap)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    // Just a single combiner stage required . . .
    EXTglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    // A = primary color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                         GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = base texture color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                         GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = secondary color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV,
                         GL_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    if (glossMap != 0)
    {
        // D = texture1 rgb (gloss mask)
        EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                             glossMap, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    }
    else
    {
        // D = texture alpha (gloss mask)
        EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                             GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }

    // SPARE0_rgb = primary * texture.rgb + secondary * texture.alpha
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                          GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                          GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = SPARE0_rgb
    EXTglFinalCombinerInputNV(GL_VARIABLE_A_NV,
                              GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = 1
    EXTglFinalCombinerInputNV(GL_VARIABLE_B_NV,
                              GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_RGB);
    // C = zero
    EXTglFinalCombinerInputNV(GL_VARIABLE_C_NV,
                              GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = zero
    EXTglFinalCombinerInputNV(GL_VARIABLE_D_NV,
                              GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = 1
    EXTglFinalCombinerInputNV(GL_VARIABLE_G_NV,
                              GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
}


// Set up the combiners to a texture with gloss in the alpha channel.
void SetupCombinersGlossMapWithFog(int glossMap)
{
    glEnable(GL_REGISTER_COMBINERS_NV);

    // Just a single combiner stage required . . .
    EXTglCombinerParameteriNV(GL_NUM_GENERAL_COMBINERS_NV, 1);

    // A = primary color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV,
                         GL_PRIMARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // B = base texture color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV,
                         GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = secondary color
    EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV,
                         GL_SECONDARY_COLOR_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    if (glossMap != 0)
    {
        // D = texture1 rgb (gloss mask)
        EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                             glossMap, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    }
    else
    {
        // D = texture alpha (gloss mask)
        EXTglCombinerInputNV(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV,
                             GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }

    // SPARE0_rgb = primary * texture.rgb + secondary * texture.alpha
    EXTglCombinerOutputNV(GL_COMBINER0_NV, GL_RGB,
                          GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV,
                          GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

    // A = fog factor
    EXTglFinalCombinerInputNV(GL_VARIABLE_A_NV,
                           GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA);
    // B = spare0_rgb
    EXTglFinalCombinerInputNV(GL_VARIABLE_B_NV,
                           GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // C = fog color
    EXTglFinalCombinerInputNV(GL_VARIABLE_C_NV,
                           GL_FOG, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // D = zero
    EXTglFinalCombinerInputNV(GL_VARIABLE_D_NV,
                           GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
    // G = 1
    EXTglFinalCombinerInputNV(GL_VARIABLE_G_NV,
                              GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA);
}


void DisableCombiners()
{
    glDisable(GL_REGISTER_COMBINERS_NV);
    EXTglActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_CUBE_MAP_EXT);
    glDisable(GL_TEXTURE_2D);
    EXTglActiveTextureARB(GL_TEXTURE0_ARB);
}
