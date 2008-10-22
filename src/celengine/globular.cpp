// qlobular.cpp
//
// Copyright (C) 2008, Celestia Development Team
// Initial code by Dr. Fridger Schrempp <fridger.schrempp@desy.de>
//
// Simulation of globular clusters, theoretical framework by  
// Ivan King, Astron. J. 67 (1962) 471; ibid. 71 (1966) 64
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "celestia.h"
#include <celmath/mathlib.h>
#include <celmath/perlin.h>
#include <celmath/intersect.h>
#include "astro.h"
#include "globular.h"
#include <celutil/util.h>
#include <celutil/debug.h>
#include "gl.h"
#include "vecgl.h"
#include "render.h"
#include "texture.h"
#include <math.h>
using namespace std;

static int width = 128, height = 128;
static Color colorTable[256];
static const unsigned int GLOBULAR_POINTS  = 8192;

// Reference values ( = data base averages) of core radius and  
// King concentration:  
static const float R_c_ref = 0.83f, C_ref = 1.47f, R_mu25 = 40.32f;  
static const float R_end = 7.0f * R_c_ref, XI = 1.0f/sqrt(1.0f + pow(100.0f, C_ref));

// P1 determines zoom level (pixels) where individual cluster stars appear
// FoV decreases (zoom increases) when P1 increases.
static const float P1 = 65.0f, P2 = 0.1f;

static float DiskSizeInPixels, Rr = 1.0f, Gg = 1.0f, Bb = 1.0f;

static GlobularForm** globularForms = NULL;
static Texture* globularTex = NULL;
static Texture* centerTex = NULL;

static void InitializeForms();
static GlobularForm* buildGlobularForms(float);
static bool formsInitialized = false;

static bool decreasing (const GBlob& b1, const GBlob& b2)
{
	return (b1.radius_2d > b2.radius_2d);
}
  
static void GlobularTextureEval(float u, float v, float /*w*/, unsigned char *pixel)
{
	// use a Gaussian luminosity shape for the individual stars
	float r = exp(- 2 * (u * u + v * v)) - 0.13533528f; // = exp(-2.0f)
	if (r <= 0.0f)
		r = 0.0f;

    int pixVal = (int) (r * 255.99f);
#ifdef HDR_COMPRESS
    pixel[0] = 127;
    pixel[1] = 127;
    pixel[2] = 127;
#else
    pixel[0] = 255;//65;
    pixel[1] = 255;//64;
    pixel[2] = 255;//65;
#endif
    pixel[3] = pixVal;
}

float relStarDensity(float rho)
{
	/*
	
	As alpha blending weight (relStarDensity) I take the theoretical number of 
    globular stars in 2d projection within a distance rho from the center, divided 
	by the area = PI * rho * rho. This number density of stars I normalized to 1 at rho=0. 
    (cf. King_1962's expression (Eq.(18)).

	The resulting blending weight increases strongly -> 1 if the 2d number density of stars
	rises, i.e for rho -> 0. 
	
	A typical globular concentration parameter c = C_ref (M4) is entered via XI.
	
	*/
    float XI2 = XI * XI; 
	float rho2 = 1.0001f + rho * rho; //add 1e-4 as regulator near rho=0
	 
	return ((log(rho2) + 4.0f * (1.0f - sqrt(rho2)) * XI) / (rho2 - 1.0f) + XI2) / (1.0f - 2.0f * XI + XI2);
}

static void CenterCloudTexEval(float u, float v, float /*w*/, unsigned char *pixel)
{
	/*

	Calculate central "cloud" texture with fixed /typical/ King_1962 parameters
	XI(c = C_ref) and r_c = R_c_ref (<=> M 4) for reasons of speed. Since it is 
	used only for the central regime, details are not very important...
	
	*/
	
	// 2d projected King_1962 profile at center (rho = 0)	

	float c2d   = 1.0f - XI;	
		
	// 2d projected King_1962 profile (Eq.(14))		
		
	float rho   = R_end * sqrt(0.5f *(u * u + v * v)) / R_c_ref;
	float rho2  = 1.0f + rho * rho;		
	float profile_2d = (1.0f / sqrt(rho2) - 1.0f)/c2d + 1.0f ;
	
	// absolutely no globular stars for r > r_t, where r_t = tidal radius:

	if (profile_2d < 0.0f) 
		  profile_2d = 0.0f; 
	
	profile_2d = profile_2d * profile_2d;
	int pixVal  = (int) (relStarDensity(rho) * profile_2d * 255.99f);

#ifdef HDR_COMPRESS
    pixel[0] = 127;
    pixel[1] = 127;
    pixel[2] = 127;
#else
    pixel[0] = 255;//65;
    pixel[1] = 255;//64;
    pixel[2] = 255;//65;
#endif
    pixel[3] = pixVal;
}
	
Globular::Globular() :
    detail (1.0f),
    customTmpName (NULL),
    form (NULL),
	r_c (R_c_ref),
	c (C_ref),
    tidalRadius(0.0f)
{
    recomputeTidalRadius();
}

const char* Globular::getType() const
{
    return "Globular";
}


void Globular::setType(const std::string& /*typeStr*/)
{
}

float Globular::getDetail() const
{
    return detail;
}


void Globular::setDetail(float d)
{
    detail = d;
}

string Globular::getCustomTmpName() const
{
    if (customTmpName == NULL)
        return "";
    else
        return *customTmpName;
}

void Globular::setCustomTmpName(const string& tmpNameStr)
{
    if (customTmpName == NULL)
        customTmpName = new string(tmpNameStr);
    else
        *customTmpName = tmpNameStr;
}

float Globular::getCoreRadius() const
{
    return r_c;
}

void Globular::setCoreRadius(float coreRadius)
{
	r_c = coreRadius;
    recomputeTidalRadius();
}
	
float Globular::getHalfMassRadius(float c, float r_c)
{
	// Aproximation to the half-mass radius r_h [arcmin]
	// formula (~ 20% accuracy) 
    return r_c * pow(10.0f, 0.6f * c - 0.4f); 	  
}

float Globular::getConcentration() const
{
	return c;
}

void Globular::setConcentration(float conc)
{
	int cslot;
    c = conc;
    if (!formsInitialized)
		InitializeForms();
	
	//account for the actual c values via 8 bins only, for saving time 
	cslot = (int)floor((conc - 0.5)/0.35 + 0.5);		
	form = globularForms[cslot];
    
    recomputeTidalRadius();
}


size_t Globular::getDescription(char* buf, size_t bufLength) const
{
	return snprintf(buf, bufLength, _("Globular (core radius: %4.2f', King concentration: %4.2f)"), r_c, c);
}


GlobularForm* Globular::getForm() const
{
    return form;
}

const char* Globular::getObjTypeName() const
{
    return "globular";
}


static const float RADIUS_CORRECTION = 0.025f;
bool Globular::pick(const Ray3d& ray,
                  double& distanceToPicker,
                  double& cosAngleToBoundCenter) const
{
    if (!isVisible())
        return false;
	/*

    The ellipsoid should be slightly larger to compensate for the fact
    that blobs are considered points when globulars are built, but have size
    when they are drawn.
    
	*/
	Vec3d ellipsoidAxes(getRadius()*(form->scale.x + RADIUS_CORRECTION),
                        getRadius()*(form->scale.y + RADIUS_CORRECTION),
                        getRadius()*(form->scale.z + RADIUS_CORRECTION));

    Quatf qf= getOrientation();
    Quatd qd(qf.w, qf.x, qf.y, qf.z);

    return testIntersection(Ray3d(Point3d() + (ray.origin - getPosition()), ray.direction) * conjugate(qd).toMatrix3(),
                            Ellipsoidd(ellipsoidAxes),
                            distanceToPicker,
                            cosAngleToBoundCenter);
}
bool Globular::load(AssociativeArray* params, const string& resPath)
{
    // Load the basic DSO parameters first
    bool ok = DeepSkyObject::load(params, resPath);
    if (!ok)
        return false;
    
    if(params->getNumber("Detail", detail))
    	setDetail((float) detail);
			
	string customTmpName;
	if(params->getString("CustomTemplate", customTmpName ))
		setCustomTmpName(customTmpName);
			
	if(params->getNumber("CoreRadius", r_c))
		setCoreRadius(r_c);
			
	if(params->getNumber("KingConcentration", c))
		setConcentration(c);
    
    return true;
}


void Globular::render(const GLContext& context,
                    const Vec3f& offset,
                    const Quatf& viewerOrientation,
                    float brightness,
                    float pixelSize)
{					        
	renderGlobularPointSprites(context, offset, viewerOrientation, brightness, pixelSize);
}


void Globular::renderGlobularPointSprites(const GLContext&,
                                      const Vec3f& offset,
                                      const Quatf& viewerOrientation,
                                      float brightness,
                                      float pixelSize)
{
    if (form == NULL)
        return;
			
	float distanceToDSO = offset.length() - getRadius();
	if (distanceToDSO < 0)
    	distanceToDSO = 0;
			
	float minimumFeatureSize = 0.5f * pixelSize * distanceToDSO;

	float size0 = 2 * getRadius(); // mu25 isophote radius
		
	DiskSizeInPixels = size0 / minimumFeatureSize;
	
	/*

	Is the globular's apparent size big enough to
    be noticeable on screen? If it's not, break right here to
    avoid all the overhead of the matrix transformations and
	GL state changes: 
	
	*/

	if (DiskSizeInPixels < 1.0f)	
		return;	    			
    /*

	The factor pixelWeight affects the blended texture opacity, depending on
	whether resolution increases or decreases. It approaches 0 if
	DiskSizeInPixels >= P1 pixels, ie if the resolution increases sufficiently! 
	
	*/	
	
	float pixelWeight = (DiskSizeInPixels >= P1)? 1.0f/(P2 + (1.0f - P2) * DiskSizeInPixels / P1): 1.0f;
			
	if (centerTex == NULL)
    {		
		centerTex = CreateProceduralTexture( width, height, GL_RGBA, 		CenterCloudTexEval);
    }
    assert(centerTex != NULL);
		
	if (globularTex == NULL)
    {
        globularTex = CreateProceduralTexture( width, height, GL_RGBA,
                                               GlobularTextureEval);
    }
    assert(globularTex != NULL);

	glEnable (GL_BLEND);
	glEnable (GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					    
	
    Mat3f viewMat = viewerOrientation.toMatrix3();
    Vec3f v0 = Vec3f(-1, -1, 0) * viewMat;
    Vec3f v1 = Vec3f( 1, -1, 0) * viewMat;
    Vec3f v2 = Vec3f( 1,  1, 0) * viewMat;
    Vec3f v3 = Vec3f(-1,  1, 0) * viewMat;
		    
    // Rescale globular clusters to actual core radius r_c:
	
	float rescale = R_c_ref / r_c;
	form->scale = Vec3f(rescale, rescale, rescale);
	    
	Mat3f m =
        Mat3f::scaling(form->scale) * getOrientation().toMatrix3() * Mat3f::scaling(size0);
				     
    vector<GBlob>* points = form->gblobs;
    unsigned int nPoints = 
				 (unsigned int) (points->size() * clamp(getDetail()));
				 				 	
	/* 

	Render central cloud sprite (centerTex). It is designed to fade away when
     i) resolution increases
	ii) distance from center increases 
	
	*/
				 		
	centerTex->bind();	
			 	
	float size = size0;
		
	float A =  3.0f  * pixelWeight;
	
	if (A > 1.0f)
    	A = 1.0f;

    float br = 2 * brightness;
	if (br > 1.0f) 
		br = 1.0f;

	glColor4f(Rr * br, Gg * br, Bb * br, A);	
		
	glBegin(GL_QUADS);		
		
	glTexCoord2f(0, 0);          glVertex((v0 * size));
    glTexCoord2f(1, 0);          glVertex((v1 * size));
    glTexCoord2f(1, 1);          glVertex((v2 * size));
    glTexCoord2f(1, 0);          glVertex((v3 * size));   					
		
	glEnd();						     
		 				
	/*

	Next, render globular cluster via distinct "star" sprites (globularTex) for
	sufficiently large resolution and distance from center of globular. 
	
	This RGBA texture fades away when
     i) resolution decreases (e.g. via automag!)
	ii) distance from globular center decreases
	
	*/
		
	globularTex->bind();			 	
	
	size = size0 / 64.0f; // Initialize size of small "star" sprites						
	
	int pow2 = 128;		  // "Red Giants" will come from 128 biggest stars with 128 <= pow2 < 256 

	glBegin(GL_QUADS);
	
	for (unsigned int i = 0; i < nPoints; ++i)
    {  						  
		GBlob         b  = (*points)[i];
		Point3f       p  = b.position * m;
		float      r_3d  = b.position.distanceFromOrigin();		
		float      r_2d  = b.radius_2d;
		
		/*! Note that the [axis,angle] input in globulars.dsc transforms the 
		 *  2d star distance r_2d in the globular frame to refer to the 
		 *  skyplane frame for each globular! That's what I need here.
		 *
		 *  The [axis,angle] input will be needed anyway, when upgrading to 
		 *  account for  ellipticities, with corresponding  inclinations and 
		 *  position angles...
		 */
		       

		if ((i & pow2) != 0)
        {
			pow2 <<=   1;		   
			size /= 1.2f;
			
			//cout<< i <<"  "<< pow2 <<"  "<< size0/size <<endl;
				
            if (size < minimumFeatureSize)
            	break;
		} 
		
		float wgt = A * relStarDensity(r_2d * rescale);
	
		// Culling: Don't render overlapping stars that would be hidden under the central "cloud" texture
				
		if (wgt < 0.95f)
		{    
			// Colors of normal globular stars according to size-rescaled color profile:		
			unsigned int iR = (unsigned int) (rescale * b.colorIndex);
		
			if (iR >= 254)
				iR  = 254;
		
			// Associate orange "Red Giant" stars with the largest sprite sizes. 
			// Don't allow them to lie RG's too far off center in 3d:
		
			Color col  = (r_3d < 2 * r_c && pow2 < 256)? colorTable[255]: colorTable[iR];		
		
			glColor4f(col.red() * br, col.green() * br, col.blue() * br, 1.0f - wgt);
						
			Point3f relPos = p + offset;
			float screenFrac = size / relPos.distanceFromOrigin();         
            
			// clamp star sprite sizes at close distance
			float size_hold = size;
			if (screenFrac >= 0.006f)   
    			size = 0.006f * relPos.distanceFromOrigin();
            	
			glTexCoord2f(0, 0);          glVertex(p + (v0 * size));
        	glTexCoord2f(1, 0);          glVertex(p + (v1 * size));
        	glTexCoord2f(1, 1);          glVertex(p + (v2 * size));
			glTexCoord2f(0, 1);          glVertex(p + (v3 * size));
						        
            size = size_hold;
		}	
	}
	glEnd();
}

unsigned int Globular::getRenderMask() const
{
    return Renderer::ShowGlobulars;
}

unsigned int Globular::getLabelMask() const
{
    return Renderer::GlobularLabels;
}
		
void Globular::hsv2rgb( float *r, float *g, float *b, float h, float s, float v )
{
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]

   int i;
   float f, p, q, t;

   if( s == 0 ) {
       // achromatic (grey)
       *r = *g = *b = v;
   return;
   }

   h /= 60;            // sector 0 to 5
   i = (int) floorf( h );
   f = h - (float) i;            // factorial part of h
   p = v * ( 1 - s );
   q = v * ( 1 - s * f );
   t = v * ( 1 - s * ( 1 - f ) );

   switch( i ) {
   case 0:
     *r = v;
     *g = t;
     *b = p;
     break;
   case 1:
     *r = q;
     *g = v;
     *b = p;
     break;
   case 2:
     *r = p;
     *g = v;
     *b = t;
     break;
   case 3:
     *r = p;
     *g = q;
     *b = v;
     break;
   case 4:
     *r = t;
     *g = p;
     *b = v;
     break;
   default:
     *r = v;
     *g = p;
     *b = q;
     break;
   }
}


// Compute the tidal radius in light years
void
Globular::recomputeTidalRadius()
{
    // Convert the core radius from arcminutes to light years
    float coreRadiusLy = std::tan(degToRad(r_c / 60.0)) * (float) getPosition().distanceFromOrigin();
    tidalRadius = coreRadiusLy * std::pow(10.0f, c);
}


GlobularForm* buildGlobularForms(float c)
{
	GBlob b;
	vector<GBlob>* globularPoints = new vector<GBlob>;
	
	float prob;
	float cc =  1.0f + pow(100.0f,c);
	unsigned int i = 0;
	
	// 3d King_1962 profile at center:	
	
	float prob0 = acos(1.0f/sqrt(cc)) - pow(10.0f,c)/cc; 
	
	/*

	Generate 3d star distribution of globulars following King_1962, 
	The probability prob is proportional to King's phi(rho); rho =r/r_c
	King concentration parameter: c=log10(r_t/r_c). 
	
	*/	
	
	while (i < GLOBULAR_POINTS)
	{	
		float r = R_end * Mathf::frand();
	    float rho = r / R_c_ref;
		float rho2 = 1.0f + rho * rho;
		
		// 3d profile from King_1962 (derived independently by me as a cross-check):  
				
		prob = (acos(sqrt(rho2/cc))/sqrt(rho2) - sqrt(cc-rho2)/cc)/rho2;
		
		/*	

		Generate 3d points of globular cluster stars in polar coordinates:
		Distribution in r according to King's 3d profile. 
		Uniform distribution on any spherical surface for given r. 
				
  	    Use acceptance-rejection method (Von Neumann) for generating points 
	    
		*/

		if (Mathf::frand() < prob/prob0)
		{
			// Note: u = cos(phi) must be used as a stochastic variable to get uniformity!
			float u = Mathf::sfrand(); 
			float theta = 2 * (float) PI * Mathf::frand();
			float sthetu2 = sin(theta) * sqrt(1.0f - u * u);
			b.position = Point3f(r * sqrt(1.0f - u * u) * cos(theta), r * sthetu2 , r * u);

			// note: 2d projection in x-z plane, according to Celestia's
			// conventions! Hence...
						
			b.radius_2d = r * sqrt(1.0f - sthetu2 * sthetu2);
			
			/*

			For now, implement only a generic spectrum for normal cluster stars, modelled from best Hubble photo of M80. 
			Blue Stragglers are qualitatively accounted for...
            Need 3d radius r in b.colorIndex, since colororation must 
			be OK also at close distance.
            
			*/

			b.colorIndex  = (unsigned int) (r / R_end * 254);
						
			globularPoints->push_back(b);					
			i++;		
		}
	} 
	GlobularForm* globularForm  = new GlobularForm();
	globularForm->gblobs        = globularPoints;
	globularForm->scale         = Vec3f(1.0f,1.0f,1.0f);	
		
	return globularForm;
}
float transit(int i, int i0, int i_width, float x0, float x1)
{
	// Fast transition from x0 to x1 at i = i0 within a width i_width.
	return x0 + 0.5f * (x1 -x0) * (tanh((float)(i - i0) / (float) i_width) + 1.0f);
}

void InitializeForms()
{

	// Build RGB color table, using hue, saturation, value as input.
	// Hue in degrees.
	
	int i0 = 24, i_satmax = 24; // location of hue transition and saturation peak in color index space 
	int i_width = 4;			// width of hue transition in color index space 
	float sat_l = 0.05f, sat_h = 0.18f, hue_r = 27.0f, hue_b = 220.0f;
	
	
	// Red Giant star color: i = 255:
	// -------------------------------
	// Convert hue, saturation and value to RGB

	Globular::hsv2rgb(&Rr, &Gg, &Bb, 25.0f, 0.55f, 1.0f);
	colorTable[255]  = Color(Rr, Gg, Bb);
		
	// normal stars: i < 255, only generic color profile for now, improve later
	// ------------------------------------------------------------------------
    // Convert hue, saturation, value to RGB
		
	for (int i = 254; i >=0; i--)
    {
		// simple qualitative saturation profile:	
		// i_satmax is value of i where sat = sat_h + sat_l maximal

	    float x = (float) i / (float) i_satmax, x2 = x ;
		float sat = sat_l + 2 * sat_h /(x2 + 1.0f / x2);
         
 	    Globular::hsv2rgb(&Rr, &Gg, &Bb, transit(i, i0, i_width, hue_r, hue_b), sat, 0.85f);    
		colorTable[i]  = Color(Rr, Gg, Bb);
	}
	// Define globularForms corresponding to 8 different bins of King concentration c 
	   
	globularForms = new GlobularForm*[8];
    for (unsigned int cslot  = 0; cslot <= 7; ++cslot)
    {
        float c = 0.5f + (float) cslot * 0.35f;
		globularForms[cslot] = buildGlobularForms(c);			    
	}				
	formsInitialized = true;

}
	
