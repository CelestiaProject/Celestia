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

#include "celestia.h"
#include "astro.h"
#include "render.h"
#include "globular.h"
#include "vecgl.h"
#include "texture.h"
#include <celmath/mathlib.h>
#include <celmath/perlin.h>
#include <celmath/intersect.h>
#include <celutil/util.h>
#include <celutil/debug.h>
#include <GL/glew.h>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include "eigenport.h"

using namespace Eigen;
using namespace std;

static int cntrTexWidth = 512, cntrTexHeight = 512; 
static int starTexWidth = 128, starTexHeight = 128;
static Color colorTable[256];
static const unsigned int GLOBULAR_POINTS  = 8192;
static const float LumiShape = 3.0f, Lumi0 = exp(-LumiShape);

// Reference values ( = data base averages) of core radius, King concentration 
// and mu25 isophote radius:  
 
static const float R_c_ref = 0.83f, C_ref = 2.1f, R_mu25 = 40.32f;  

// min/max c-values of globular cluster data

static const float MinC = 0.50f, MaxC = 2.58f, BinWidth = (MaxC - MinC) / 8.0f + 0.02f;

// P1 determines the zoom level, where individual cluster stars start to appear.
// The smaller P2 (< 1), the faster stars show up when resolution increases.

static const float P1 = 65.0f, P2 = 0.75f;

static const float RRatio_min = pow(10.0f, 1.7f);
static float CBin, RRatio, XI, DiskSizeInPixels, Rr = 1.0f, Gg = 1.0f, Bb = 1.0f;

static GlobularForm** globularForms = NULL;
static Texture* globularTex = NULL;
static Texture* centerTex[8] = {NULL};
static void InitializeForms();
static GlobularForm* buildGlobularForms(float);
static bool formsInitialized = false;

static bool decreasing (const GBlob& b1, const GBlob& b2)
{
	return (b1.radius_2d > b2.radius_2d);
}

static void GlobularTextureEval(float u, float v, float /*w*/, unsigned char *pixel)
{
	// use an exponential luminosity shape for the individual stars
    // giving sort of a halo for the brighter (i.e.bigger) stars.

    float lumi = exp(- LumiShape * sqrt(u * u + v * v)) - Lumi0;

	if (lumi <= 0.0f)
		lumi = 0.0f;

    int pixVal = (int) (lumi * 255.99f);
#ifdef HDR_COMPRESS
    pixel[0] = 127;
    pixel[1] = 127;
    pixel[2] = 127;
#else
    pixel[0] = 255;
    pixel[1] = 255;
    pixel[2] = 255;
#endif
    pixel[3] = pixVal;
}


float relStarDensity(float eta)
{
	/*! As alpha blending weight (relStarDensity) I take the theoretical
	 *  number of globular stars in 2d projection at a distance 
	 *  rho = r / r_c = eta * r_t from the center (cf. King_1962's Eq.(18)), 
	 *  divided by the area = PI * rho * rho . This number density of stars 
	 *  I normalized to 1 at rho=0.   

	 *  The resulting blending weight increases strongly -> 1 if the 
	 *  2d number density of stars rises, i.e for rho -> 0. 
	 */	 	 
     // Since the central "cloud" is due to lack of visual resolution,
	 // rather than cluster morphology, we limit it's size by
	 // taking max(C_ref, CBin). Smaller c gives a shallower distribution!
     
     float rRatio = max(RRatio_min, RRatio);
     float Xi = 1.0f / sqrt(1.0f + rRatio * rRatio);           	 
     float XI2 = Xi * Xi; 
	 float rho2 = 1.0001f + eta * eta * rRatio * rRatio; //add 1e-4 as regulator near rho=0

	 return ((log(rho2) + 4.0f * (1.0f - sqrt(rho2)) * Xi) / (rho2 - 1.0f) + XI2) / (1.0f - 2.0f * Xi + XI2);
}

static void CenterCloudTexEval(float u, float v, float /*w*/, unsigned char *pixel)
{	
	/*! For reasons of speed, calculate central "cloud" texture only for  
	 *  8 bins of King_1962 concentration, c = CBin, XI(CBin), RRatio(CBin). 
	 */
	 
	// Skyplane projected King_1962 profile at center (rho = eta = 0):	
	float c2d   = 1.0f - XI;	
		
	float eta = sqrt(u * u + v * v);  // u,v = (-1..1)
	
	// eta^2 = u * u  + v * v = 1 is the biggest circle fitting into the quadratic
	// procedural texture. Hence clipping 
		
	if (eta >= 1.0f)
		eta  = 1.0f;

	// eta = 1 corresponds to tidalRadius:

	float rho   = eta  * RRatio;
	float rho2  = 1.0f + rho * rho;		

    // Skyplane projected King_1962 profile (Eq.(14)), vanishes for eta = 1:
	// i.e. absolutely no globular stars for r > tidalRadius:	
		
	float profile_2d = (1.0f / sqrt(rho2) - 1.0f)/c2d + 1.0f ;	
	profile_2d = profile_2d * profile_2d;	

#ifdef HDR_COMPRESS
    pixel[0] = 127;
    pixel[1] = 127; 
    pixel[2] = 127;
#else    
    pixel[0] = 255;
    pixel[1] = 255;
    pixel[2] = 255;
#endif
	pixel[3] = (int) (relStarDensity(eta) * profile_2d * 255.99f);
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

unsigned int Globular::cSlot(float conc) const
{
	// map the physical range of c, minC <= c <= maxC, 
	// to 8 integers (bin numbers), 0 < cSlot <= 7:
	
	if (conc <= MinC)
		conc  = MinC;
	if (conc >= MaxC)
		conc  = MaxC;
  	 
    return (unsigned int) floor((conc - MinC) / BinWidth);
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

void Globular::setCoreRadius(const float coreRadius)
{
	r_c = coreRadius;
	recomputeTidalRadius();
}
	
float Globular::getHalfMassRadius() const
{
	// Aproximation to the half-mass radius r_h [ly]
	// (~ 20% accuracy) 
    
	return std::tan(degToRad(r_c / 60.0f)) * (float) getPosition().norm() * pow(10.0f, 0.6f * c - 0.4f); 	  
}

float Globular::getConcentration() const
{
	return c;
}
void Globular::setConcentration(const float conc)
{
    c = conc;
    if (!formsInitialized)
		InitializeForms();
	
	// For saving time, account for the c dependence via 8 bins only,

	form = globularForms[cSlot(conc)];
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
     * The selection ellipsoid should be slightly larger to compensate for the fact
     * that blobs are considered points when globulars are built, but have size
     * when they are drawn. 
	 */
	Vec3d ellipsoidAxes(getRadius() * (form->scale.x + RADIUS_CORRECTION),
                        getRadius() * (form->scale.y + RADIUS_CORRECTION),
                        getRadius() * (form->scale.z + RADIUS_CORRECTION));

    Eigen::Vector3d p = getPosition();
    return testIntersection(Ray3d(ray.origin - p, ray.direction).transform(getOrientation().cast<double>().toRotationMatrix()),
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

    if (params->getNumber("Detail", detail))
    	setDetail((float) detail);
			
	string customTmpName;
	if (params->getString("CustomTemplate", customTmpName ))
		setCustomTmpName(customTmpName);
    
    double coreRadius;
	if (params->getAngle("CoreRadius", coreRadius, 1.0 / MINUTES_PER_DEG))
    {
        r_c = coreRadius;
		setCoreRadius(r_c);
    }
			
	if (params->getNumber("KingConcentration", c))
		setConcentration(c);
	
    return true;
}


void Globular::render(const GLContext& context,
                    const Vector3f& offset,
                    const Quaternionf& viewerOrientation,
                    float brightness,
                    float pixelSize)
{					        
    renderGlobularPointSprites(context, fromEigen(offset), fromEigen(viewerOrientation), brightness, pixelSize);
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
	
	DiskSizeInPixels = getRadius() / minimumFeatureSize;
	
	/*
     * Is the globular's apparent size big enough to
     * be noticeable on screen? If it's not, break right here to
     * avoid all the overhead of the matrix transformations and
	 * GL state changes: 
	 */
	
	if (DiskSizeInPixels < 1.0f)	
		return;	    			
    /*
	 * When resolution (zoom) varies, the blended texture opacity is controlled by the 
	 * factor 'pixelWeight'. At low resolution, the latter starts at 1, but tends to 0,  
     * if the resolution increases sufficiently (DiskSizeInPixels >= P1 pixels)! 
	 * The smaller P2 (<1), the faster pixelWeight -> 0, for DiskSizeInPixels >= P1.
	 */	
			
	float pixelWeight = (DiskSizeInPixels >= P1)? 1.0f/(P2 + (1.0f - P2) * DiskSizeInPixels / P1): 1.0f;
							
    // Use same 8 c-bins as in globularForms below!        
    
    unsigned int ic = cSlot(c); 
    CBin = MinC + ((float) ic + 0.5f) * BinWidth; // center value of (ic+1)th c-bin

	RRatio = pow(10.0f, CBin); 
    XI = 1.0f / sqrt(1.0f + RRatio * RRatio);           
    
    if(centerTex[ic] == NULL)
	{
		centerTex[ic] = CreateProceduralTexture( cntrTexWidth, cntrTexHeight, GL_RGBA, CenterCloudTexEval);
	}
	assert(centerTex[ic] != NULL);
		
	if (globularTex == NULL)
    {
        globularTex = CreateProceduralTexture( starTexWidth, starTexHeight, GL_RGBA,
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
	
	float tidalSize = 2 * tidalRadius;
	Mat3f m =
        Mat3f::scaling(form->scale) * fromEigen(getOrientation()).toMatrix3() * 
		Mat3f::scaling(tidalSize);
				     
    vector<GBlob>* points = form->gblobs;
    unsigned int nPoints = 
				 (unsigned int) (points->size() * clamp(getDetail()));
				 				 	
	/* Render central cloud sprite (centerTex). It fades away when
	 * distance from center or resolution increases sufficiently. 	
	 */
	
	centerTex[ic]->bind();	
    
	float br = 2 * brightness;

	glColor4f(Rr, Gg, Bb, min(br * pixelWeight, 1.0f));	

	glBegin(GL_QUADS);		
		
	glTexCoord2f(0, 0);          glVertex(v0 * tidalSize);
    glTexCoord2f(1, 0);          glVertex(v1 * tidalSize);
    glTexCoord2f(1, 1);          glVertex(v2 * tidalSize);
    glTexCoord2f(0, 1);          glVertex(v3 * tidalSize);   					
		
	glEnd();						     
			 				
	/*! Next, render globular cluster via distinct "star" sprites (globularTex) 
	 * for sufficiently large resolution and distance from center of globular.
	 *  
	 * This RGBA texture fades away when resolution decreases (e.g. via automag!), 
	 * or when distance from globular center decreases.
	 */
	
	
	globularTex->bind();
	
	
	int pow2 = 128;			     // Associate "Red Giants" with the 128 biggest star-sprites  

	float starSize =  br * 0.5f; // Maximal size of star sprites -> "Red Giants"
	float clipDistance = 100.0f; // observer distance [ly] from globular, where we 
		                         // start "morphing" the star-sprite sizes towards 
			                     // their physical values
	glBegin(GL_QUADS);
    
	for (unsigned int i = 0; i < nPoints; ++i)
	{  						  
		GBlob         b  = (*points)[i];
		Point3f       p  = b.position * m;		
		float    eta_2d  = b.radius_2d;
		
		/*! Note that the [axis,angle] input in globulars.dsc transforms the 
		 *  2d projected star distance r_2d in the globular frame to refer to the 
		 *  skyplane frame for each globular! That's what I need here.
	     *
	     *  The [axis,angle] input will be needed anyway, when upgrading to 
		 *  account for  ellipticities, with corresponding  inclinations and 
		 *  position angles...
	     */


		if ((i & pow2) != 0)
		{      
			pow2    <<=   1;		   
			starSize /= 1.25f;			
	   			
			if (starSize < minimumFeatureSize)
           		break;
		} 

		float obsDistanceToStarRatio = (p + offset).distanceFromOrigin() / clipDistance;		
		float saveSize = starSize;

		if (obsDistanceToStarRatio < 1.0f)
		{  
			// "Morph" the star-sprite sizes at close observer distance such that
			// the overdense globular core is dissolved upon closing in.
            
			starSize = starSize * min(obsDistanceToStarRatio, 1.0f);     
		}	
         
		/* Colors of normal globular stars are given by color profile.
		 * Associate orange "Red Giant" stars with the largest sprite
		 * sizes (while pow2 = 128).
		 */

		Color col  = (pow2 < 256)? colorTable[255]: colorTable[b.colorIndex];		
		glColor4f(col.red(), col.green(), col.blue(), 
			      min(br * (1.0f - pixelWeight * relStarDensity(eta_2d)), 1.0f));
	   
		glTexCoord2f(0, 0);          glVertex(p + (v0 * starSize));
		glTexCoord2f(1, 0);          glVertex(p + (v1 * starSize));
		glTexCoord2f(1, 1);          glVertex(p + (v2 * starSize));
		glTexCoord2f(0, 1);          glVertex(p + (v3 * starSize));
	   					        
		starSize = saveSize;	
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
		

void Globular::recomputeTidalRadius()
{
	// Convert the core radius from arcminutes to light years
	// Compute the tidal radius in light years
        
	float coreRadiusLy = std::tan(degToRad(r_c / 60.0f)) * (float) getPosition().norm();
    tidalRadius = coreRadiusLy * std::pow(10.0f, c);     
}


GlobularForm* buildGlobularForms(float c)
{
	GBlob b;
	vector<GBlob>* globularPoints = new vector<GBlob>;

	float rRatio = pow(10.0f, c); //  = r_t / r_c
	float prob;
	float cc =  1.0f + rRatio * rRatio;
	unsigned int i = 0, k = 0;
	
	// Value of King_1962 luminosity profile at center:	
	
	float prob0 = sqrt(cc) - 1.0f;

	/*! Generate the globular star distribution randomly, according 
	 *  to the  King_1962 surface density profile f(r), eq.(14).
	 *
	 *  rho = r / r_c = eta r_t / r_c, 0 <= eta <= 1,  
  	 *  coreRadius r_c, tidalRadius r_t, King concentration c = log10(r_t/r_c). 
	 */	
	
	while (i < GLOBULAR_POINTS)
	{	
	    /*!  
		 *  Use a combination of the Inverse Transform method and 
		 *  Von Neumann's Acceptance-Rejection method for generating sprite stars 
		 *  with eta distributed according to the exact King luminosity profile.    
		 *
		 * This algorithm leads to almost 100% efficiency for all values of  
		 * parameters and variables!
		 */
	
		float uu = Mathf::frand();
		
		/* First step: eta distributed as inverse power distribution (~1/Z^2) 
		 * that majorizes the exact King profile. Compute eta in terms of uniformly 
		 * distributed variable uu! Normalization to 1 for eta -> 0.
         */ 				
		
		float eta = tan(uu *atan(rRatio))/rRatio;

		float rho = eta * rRatio;		
		float  cH = 1.0f/(1.0f + rho * rho);
        float   Z = sqrt((1.0f + rho * rho)/cc); // scaling variable
		
	    // Express King_1962 profile in terms of the UNIVERSAL variable 0 < Z <= 1,		
		
		prob = (1.0f - 1.0f / Z) / prob0;
		prob = prob * prob;
								
  	    /* Second step: Use Acceptance-Rejection method (Von Neumann) for
		 * correcting the power distribution of eta into the exact, 
		 * desired King form 'prob'!
		 */

        k++;

		if (Mathf::frand() < prob / cH)
		{
		   	/* Generate 3d points of globular cluster stars in polar coordinates:
			 * Distribution in eta (<=> r) according to King's profile. 
		     * Uniform distribution on any spherical surface for given eta. 
			 * Note: u = cos(phi) must be used as a stochastic variable to get uniformity in angle!
			 */
			float u = Mathf::sfrand(); 
			float theta = 2 * (float) PI * Mathf::frand();
			float sthetu2 = sin(theta) * sqrt(1.0f - u * u);
			
			// x,y,z points within -0.5..+0.5, as required for consistency: 
			b.position = 0.5f * Point3f(eta * sqrt(1.0f - u * u) * cos(theta), eta * sthetu2 , eta * u);
			            
			/* 
			 * Note: 2d projection in x-z plane, according to Celestia's
			 * conventions! Hence...
			 */			
			b.radius_2d = eta * sqrt(1.0f - sthetu2 * sthetu2);

			/* For now, implement only a generic spectrum for normal cluster
			 * stars, modelled from Hubble photo of M80. 
			 * Blue Stragglers are qualitatively accounted for...
   			 * assume color index poportional to Z as function of which the King profile
			 * becomes universal!
			 */
			  
			b.colorIndex  = (unsigned int) (Z * 254);
            
		    globularPoints->push_back(b);					
			i++;		
		}
	}
	
	// Check for efficiency of sprite-star generation => close to 100 %!
	//cout << "c =  "<< c <<"  i =  " << i - 1 <<"  k =  " << k - 1 <<                                                       "  Efficiency:  " << 100.0f * i / (float)k<<"%" << endl;	
	
	GlobularForm* globularForm  = new GlobularForm();
	globularForm->gblobs        = globularPoints;
	globularForm->scale         = Vec3f(1.0f, 1.0f, 1.0f);	
		
	return globularForm;
}

void InitializeForms()
{

	// Build RGB color table, using hue, saturation, value as input.
	// Hue in degrees.
	
	// Location of hue transition and saturation peak in color index space: 
	int i0 = 36, i_satmax = 16; 
	// Width of hue transition in color index space: 	
	int i_width = 3;
				
	float sat_l = 0.08f, sat_h = 0.1f, hue_r = 27.0f, hue_b = 220.0f;
	
	// Red Giant star color: i = 255:
	// -------------------------------
	// Convert hue, saturation and value to RGB

	DeepSkyObject::hsv2rgb(&Rr, &Gg, &Bb, 25.0f, 0.65f, 1.0f);
	colorTable[255]  = Color(Rr, Gg, Bb);
		
	// normal stars: i < 255, generic color profile for now, improve later
	// --------------------------------------------------------------------
    // Convert hue, saturation, value to RGB
		
	for (int i = 254; i >=0; i--)
    {
		// simple qualitative saturation profile:	
		// i_satmax is value of i where sat = sat_h + sat_l maximal

	    float x = (float) i / (float) i_satmax, x2 = x ;
		float sat = sat_l + 2 * sat_h /(x2 + 1.0f / x2);
				
		// Fast transition from hue_r to hue_b at i = i0 within a width
		// i_width in color index space:

		float hue = hue_r + 0.5f * (hue_b - hue_r) * (std::tanh((float)(i - i0) / (float) i_width) + 1.0f);
		
		DeepSkyObject::hsv2rgb(&Rr, &Gg, &Bb, hue, sat, 0.85f);
		colorTable[i]  = Color(Rr, Gg, Bb);
	}
	// Define globularForms corresponding to 8 different bins of King concentration c 
   
	globularForms = new GlobularForm*[8];   

	for (unsigned int ic  = 0; ic <= 7; ++ic)
    {
        float CBin = MinC + ((float) ic + 0.5f) * BinWidth;
		globularForms[ic] = buildGlobularForms(CBin); 
	}				
	formsInitialized = true;

}
