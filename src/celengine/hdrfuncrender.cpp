// render.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifdef USE_HDR
void Renderer::genBlurTextures()
{
    for (size_t i = 0; i < BLUR_PASS_COUNT; ++i)
    {
        if (blurTextures[i] != nullptr)
        {
            delete blurTextures[i];
            blurTextures[i] = nullptr;
        }
    }
    if (blurTempTexture != nullptr)
    {
        delete blurTempTexture;
        blurTempTexture = nullptr;
    }

    blurBaseWidth = sceneTexWidth, blurBaseHeight = sceneTexHeight;

    if (blurBaseWidth > blurBaseHeight)
    {
        while (blurBaseWidth > BLUR_SIZE)
        {
            blurBaseWidth  >>= 1;
            blurBaseHeight >>= 1;
        }
    }
    else
    {
        while (blurBaseHeight > BLUR_SIZE)
        {
            blurBaseWidth  >>= 1;
            blurBaseHeight >>= 1;
        }
    }
    genBlurTexture(0);
    genBlurTexture(1);

    Image *tempImg;
    ImageTexture *tempTexture;
    tempImg = new Image(GL_LUMINANCE, blurBaseWidth, blurBaseHeight);
    tempTexture = new ImageTexture(*tempImg, Texture::EdgeClamp, Texture::DefaultMipMaps);
    delete tempImg;
    if (tempTexture && tempTexture->getName() != 0)
        blurTempTexture = tempTexture;
}

void Renderer::genBlurTexture(int blurLevel)
{
    Image *img;
    ImageTexture *texture;

#ifdef DEBUG_HDR
    HDR_LOG <<
        "Window width = "    << windowWidth << ", " <<
        "Window height = "   << windowHeight << ", " <<
        "Blur tex width = "  << (blurBaseWidth>>blurLevel) << ", " <<
        "Blur tex height = " << (blurBaseHeight>>blurLevel) << endl;
#endif
    img = new Image(blurFormat,
                    blurBaseWidth>>blurLevel,
                    blurBaseHeight>>blurLevel);
    texture = new ImageTexture(*img,
                               Texture::EdgeClamp,
                               Texture::NoMipMaps);
    delete img;

    if (texture && texture->getName() != 0)
        blurTextures[blurLevel] = texture;
}

void Renderer::genSceneTexture()
{
    unsigned int *data;
    if (sceneTexture != 0)
        glDeleteTextures(1, &sceneTexture);

    sceneTexWidth  = 1;
    sceneTexHeight = 1;
    while (sceneTexWidth < windowWidth)
        sceneTexWidth <<= 1;
    while (sceneTexHeight < windowHeight)
        sceneTexHeight <<= 1;
    sceneTexWScale = (windowWidth > 0)  ? (GLfloat)sceneTexWidth  / (GLfloat)windowWidth :
        1.0f;
    sceneTexHScale = (windowHeight > 0) ? (GLfloat)sceneTexHeight / (GLfloat)windowHeight :
        1.0f;
    data = (unsigned int* )malloc(sceneTexWidth*sceneTexHeight*4*sizeof(unsigned int));
    memset(data, 0, sceneTexWidth*sceneTexHeight*4*sizeof(unsigned int));

    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, sceneTexWidth, sceneTexHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);

    free(data);
#ifdef DEBUG_HDR
    static int genSceneTexCounter = 1;
    HDR_LOG <<
        "[" << genSceneTexCounter++ << "] " <<
        "Window width = "  << windowWidth << ", " <<
        "Window height = " << windowHeight << ", " <<
        "Tex width = "  << sceneTexWidth << ", " <<
        "Tex height = " << sceneTexHeight << endl;
#endif
}

void Renderer::renderToBlurTexture(int blurLevel)
{
    if (blurTextures[blurLevel] == nullptr)
        return;
    GLsizei blurTexWidth  = blurBaseWidth>>blurLevel;
    GLsizei blurTexHeight = blurBaseHeight>>blurLevel;
    GLsizei blurDrawWidth = (GLfloat)windowWidth/(GLfloat)sceneTexWidth * blurTexWidth;
    GLsizei blurDrawHeight = (GLfloat)windowHeight/(GLfloat)sceneTexHeight * blurTexHeight;
    GLfloat blurWScale = 1.f;
    GLfloat blurHScale = 1.f;
    GLfloat savedWScale = 1.f;
    GLfloat savedHScale = 1.f;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
    glClearColor(0, 0, 0, 1.f);
    glViewport(0, 0, blurDrawWidth, blurDrawHeight);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);

    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
    // Do not need to scale alpha so mask it off
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glEnable(GL_BLEND);
    savedWScale = sceneTexWScale;
    savedHScale = sceneTexHScale;

    // Remove ldr part of image
    {
    const GLfloat bias  = -0.5f;
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquationEXT(GL_FUNC_REVERSE_SUBTRACT_EXT);
    glColor4f(-bias, -bias, -bias, 0.0f);

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(1.f,  0.0f);
    glVertex2f(1.f,  1.f);
    glVertex2f(0.0f, 1.f);
    glEnd();

    glEnable(GL_TEXTURE_2D);
    blurTextures[blurLevel]->bind();
    glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                     blurTexWidth, blurTexHeight, 0);
    }

    // Scale back up hdr part
    {
    glBlendEquationEXT(GL_FUNC_ADD_EXT);
    glBlendFunc(GL_DST_COLOR, GL_ONE);

    glBegin(GL_QUADS);
    drawBlendedVertices(0.f, 0.f, 1.f); //x2
    drawBlendedVertices(0.f, 0.f, 1.f); //x2
    glEnd();
    }

    glDisable(GL_BLEND);

    if (!useLuminanceAlpha)
    {
        blurTempTexture->bind();
        glCopyTexImage2D(GL_TEXTURE_2D, blurLevel, GL_LUMINANCE, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
        // Erase color, replace with luminance image
        glBegin(GL_QUADS);
        glColor4f(0.f, 0.f, 0.f, 1.f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(0.0f, 1.0f);
        glEnd();
        glBegin(GL_QUADS);
        drawBlendedVertices(0.f, 0.f, 1.f);
        glEnd();
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    blurTextures[blurLevel]->bind();
    glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                     blurTexWidth, blurTexHeight, 0);
// blending end

    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat xdelta = 1.0f / (GLfloat)blurTexWidth;
    GLfloat ydelta = 1.0f / (GLfloat)blurTexHeight;
    blurWScale = ((GLfloat)blurTexWidth / (GLfloat)blurDrawWidth);
    blurHScale = ((GLfloat)blurTexHeight / (GLfloat)blurDrawHeight);
    sceneTexWScale = blurWScale;
    sceneTexHScale = blurHScale;

    // Butterworth low pass filter to reduce flickering dots
    {
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f,    0.0f, .5f*1.f);
        drawBlendedVertices(-xdelta, 0.0f, .5f*0.333f);
        drawBlendedVertices( xdelta, 0.0f, .5f*0.25f);
        glEnd();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta, .5f*0.667f);
        drawBlendedVertices(0.0f,  ydelta, .5f*0.333f);
        glEnd();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         blurTexWidth, blurTexHeight, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Gaussian blur
    switch (blurLevel)
    {
/*
    case 0:
        drawGaussian3x3(xdelta, ydelta, blurTexWidth, blurTexHeight, 1.f);
        break;
*/
#ifdef __APPLE__
    case 0:
        drawGaussian5x5(xdelta, ydelta, blurTexWidth, blurTexHeight, 1.f);
        break;
    case 1:
        drawGaussian9x9(xdelta, ydelta, blurTexWidth, blurTexHeight, .3f);
        break;
#else
    // Gamma correct: windows=(mac^1.8)^(1/2.2)
    case 0:
        drawGaussian5x5(xdelta, ydelta, blurTexWidth, blurTexHeight, 1.f);
        break;
    case 1:
        drawGaussian9x9(xdelta, ydelta, blurTexWidth, blurTexHeight, .373f);
        break;
#endif
    default:
        break;
    }

    blurTextures[blurLevel]->bind();
    glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                     blurTexWidth, blurTexHeight, 0);

    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);
    glPopAttrib();
    sceneTexWScale = savedWScale;
    sceneTexHScale = savedHScale;
}

void Renderer::renderToTexture(const Observer& observer,
                               const Universe& universe,
                               float faintestMagNight,
                               const Selection& sel)
{
    if (sceneTexture == 0)
        return;
    glPushAttrib(GL_COLOR_BUFFER_BIT);

    draw(observer, universe, faintestMagNight, sel);

    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0,
                     sceneTexWidth, sceneTexHeight, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPopAttrib();
}

void Renderer::drawSceneTexture()
{
    if (sceneTexture == 0)
        return;
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
}

void Renderer::drawBlendedVertices(float xdelta, float ydelta, float blend)
{
    glColor4f(1.0f, 1.0f, 1.0f, blend);
    glTexCoord2i(0, 0); glVertex2f(xdelta,                ydelta);
    glTexCoord2i(1, 0); glVertex2f(sceneTexWScale+xdelta, ydelta);
    glTexCoord2i(1, 1); glVertex2f(sceneTexWScale+xdelta, sceneTexHScale+ydelta);
    glTexCoord2i(0, 1); glVertex2f(xdelta,                sceneTexHScale+ydelta);
}

void Renderer::drawGaussian3x3(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend)
{
#ifdef USE_BLOOM_LISTS
    if (gaussianLists[0] == 0)
    {
        gaussianLists[0] = glGenLists(1);
        glNewList(gaussianLists[0], GL_COMPILE);
#endif
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, blend);
        drawBlendedVertices(-xdelta, 0.0f, 0.25f*blend);
        drawBlendedVertices( xdelta, 0.0f, 0.20f*blend);
        glEnd();

        // Take result of horiz pass and apply vertical pass
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         width, height, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta, 0.429f);
        drawBlendedVertices(0.0f,  ydelta, 0.300f);
        glEnd();
#ifdef USE_BLOOM_LISTS
        glEndList();
    }
    glCallList(gaussianLists[0]);
#endif
}

void Renderer::drawGaussian5x5(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend)
{
#ifdef USE_BLOOM_LISTS
    if (gaussianLists[1] == 0)
    {
        gaussianLists[1] = glGenLists(1);
        glNewList(gaussianLists[1], GL_COMPILE);
#endif
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, blend);
        drawBlendedVertices(-xdelta,      0.0f, 0.475f*blend);
        drawBlendedVertices( xdelta,      0.0f, 0.475f*blend);
        drawBlendedVertices(-2.0f*xdelta, 0.0f, 0.075f*blend);
        drawBlendedVertices( 2.0f*xdelta, 0.0f, 0.075f*blend);
        glEnd();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         width, height, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta,      0.475f);
        drawBlendedVertices(0.0f,  ydelta,      0.475f);
        drawBlendedVertices(0.0f, -2.0f*ydelta, 0.075f);
        drawBlendedVertices(0.0f,  2.0f*ydelta, 0.075f);
        glEnd();
#ifdef USE_BLOOM_LISTS
        glEndList();
    }
    glCallList(gaussianLists[1]);
#endif
}

void Renderer::drawGaussian9x9(float xdelta, float ydelta, GLsizei width, GLsizei height, float blend)
{
#ifdef USE_BLOOM_LISTS
    if (gaussianLists[2] == 0)
    {
        gaussianLists[2] = glGenLists(1);
        glNewList(gaussianLists[2], GL_COMPILE);
#endif
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, 0.0f, blend);
        drawBlendedVertices(-xdelta,      0.0f, 0.632f*blend);
        drawBlendedVertices( xdelta,      0.0f, 0.632f*blend);
        drawBlendedVertices(-2.0f*xdelta, 0.0f, 0.159f*blend);
        drawBlendedVertices( 2.0f*xdelta, 0.0f, 0.159f*blend);
        drawBlendedVertices(-3.0f*xdelta, 0.0f, 0.016f*blend);
        drawBlendedVertices( 3.0f*xdelta, 0.0f, 0.016f*blend);
        glEnd();

        glCopyTexImage2D(GL_TEXTURE_2D, 0, blurFormat, 0, 0,
                         width, height, 0);
        glBegin(GL_QUADS);
        drawBlendedVertices(0.0f, -ydelta,      0.632f);
        drawBlendedVertices(0.0f,  ydelta,      0.632f);
        drawBlendedVertices(0.0f, -2.0f*ydelta, 0.159f);
        drawBlendedVertices(0.0f,  2.0f*ydelta, 0.159f);
        drawBlendedVertices(0.0f, -3.0f*ydelta, 0.016f);
        drawBlendedVertices(0.0f,  3.0f*ydelta, 0.016f);
        glEnd();
#ifdef USE_BLOOM_LISTS
        glEndList();
    }
    glCallList(gaussianLists[2]);
#endif
}

void Renderer::drawBlur()
{
    blurTextures[0]->bind();
    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
    blurTextures[1]->bind();
    glBegin(GL_QUADS);
    drawBlendedVertices(0.0f, 0.0f, 1.0f);
    glEnd();
}

bool Renderer::getBloomEnabled()
{
    return bloomEnabled;
}

void Renderer::setBloomEnabled(bool aBloomEnabled)
{
    bloomEnabled = aBloomEnabled;
}

void Renderer::increaseBrightness()
{
    brightPlus += 1.0f;
}

void Renderer::decreaseBrightness()
{
    brightPlus -= 1.0f;
}

float Renderer::getBrightness()
{
    return brightPlus;
}
#endif // USE_HDR
