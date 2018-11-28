// celdat2txt.cpp
// version 1.6.0 2017-02-25
// Redesign Copyright (C) 2017, Hans Bruggink (HB)
//
// Based on original code of startextdump.cpp & makeindex.cpp
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// Dump the contents of the Celestia star database files in a text
// format that's easy to read, edit or to use in other db applications.

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <cassert>
#include <stdio.h>
#include <math.h>       /* All mathematics */
#include <celutil/bytes.h>
#include <celengine/astro.h>
#include <celengine/stellarclass.h>
#include <celengine/star.h>

using namespace std;
#define PI 3.14159265358979323846
#define HEADER_NEW  "HIP number;RAdeg;DECdeg;RA(hms);DEC(dms);Distance(LY);Parallax;AppMag;AbsMag;StellarClass\n"

#define DCOS     cos
#define DSIN     sin
#define DATAN2   atan2
#define DASIN    asin

#define X1(i)    x1[i-1]
#define X2(i)    x2[i-1]
#define R(i,j)   r[i-1][j-1]

static string inputFilename;
static string outputFilename;
static string hdFilename;

static bool useXindexDump = false;
static bool DumpAll = false;
static bool convert = false;
static bool ConstDump = false;

char *CON;
static double CONVH = 0.2617993878;
static double CONVD = 0.1745329251994e-01;
static double PI4 = 6.28318530717948;
static double E75 = 1875.;
double ARAD,DRAD,A,D,E,RAH,RA,DEC,RAL,RAU,DECL,DECD;
double Ein ;
char buf[BUFSIZ], *p;
char dEin[] = "1991.25";
int StringLength(char []);

double *RA1, *DEC1, *EPOCH1, *EPOCH2, *RA2, *DEC2;
double RA2c,DE2c;

FILE *f2 = 0;

extern "C"
{
void HGTPRC(double*,double*,double*,double*,double*,double*);
}


void Usage()
{
    cerr << "Usage:\n";
    cerr << "celdat2txt [options] <star database file> [output file]\n\n";
    cerr << "  Options for new format star database as input:\n";
    cerr << "    no options          : Dumps data from stardb.dat to text (HIPnr,RAdeg,DECdeg,\n";
    cerr << "                          absMag, distance and stellarclass).\n";
    cerr << "    --list (or -l)      : Make a list of Constellations from stardb.dat in\n";
    cerr << "                          csv-format with calculated RA,dec from Epoch JYYYY.nn\n";
    cerr << "    --convert (or -c)   : Convert stardb.dat to extended csv-format including RA(hh:mm:ss.ss),\n";
    cerr << "                          dec(dd:mm:ss.ss),distance(ly),absMag,appMag,pa,stellarclass)\n";
    cerr << "    --convert --all     : Convert stardb.dat to extended csv-format\n";
    cerr << "          (or -c -a)      as above and add Constellation.\n\n";
    cerr << "    --index (or -x)     : dump new HD or SAO binary x-ref catalogs to text\n\n";
}

static uint32_t readUint(istream& in)
{
    uint32_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT32(n, n);
    return n;
}

static float readFloat(istream& in)
{
    float f;
    in.read(reinterpret_cast<char*>(&f), sizeof f);
    LE_TO_CPU_FLOAT(f, f);
    return f;
}

static int16_t readShort(istream& in)
{
    int16_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT16(n, n);
    return n;
}

static uint16_t readUshort(istream& in)
{
    uint16_t n;
    in.read(reinterpret_cast<char*>(&n), sizeof n);
    LE_TO_CPU_INT16(n, n);
    return n;
}

static uint8_t readUbyte(istream& in)
{
    uint8_t n;
    in.read((char*) &n, 1);
    return n;
}

void printStellarClass(uint16_t sc, ostream& out)
{
    StellarClass::StarType st = (StellarClass::StarType) (sc >> 12);
    if (st == StellarClass::WhiteDwarf)
    {
        unsigned int spectralClass = (sc >> 8 & 0xf);
        unsigned int spectralSubclass = (sc >> 4 & 0xf);
        unsigned int luminosityClass = (sc & 0xf);
        out << "D";                               // Better than just "WD".
        out << "ABCOQZGXVPHE"[spectralClass];     // A guess for Whitedwarfs
        out << "0123456789?"[spectralSubclass];   // "?" added to avoid NULL chars.
    }
    else if (st == StellarClass::NeutronStar)
    {
        out << "Q";
    }
    else if (st == StellarClass::BlackHole)
    {
        out << "X";
    }
    else if (st == StellarClass::NormalStar)
    {
        unsigned int spectralClass = (sc >> 8 & 0xf);
        unsigned int spectralSubclass = (sc >> 4 & 0xf);
        unsigned int luminosityClass = (sc & 0xf);

        if (spectralClass == 12)
        {
            out << '?';
        }
        else
        {
            out << "OBAFGKMRSNWW?LTC"[spectralClass];
            out << "0123456789?"[spectralSubclass]; //add ? because a 10(hex a) is declared as a unknown subclass.
            switch (luminosityClass)                //without this questionmark a nullchar is written to the file
            {                                       //causing that a dump of stardb is not a textfile but binary.
            case StellarClass::Lum_Ia0:
                out << "I-a0";
                break;
            case StellarClass::Lum_Ia:
                out << "I-a";
                break;
            case StellarClass::Lum_Ib:
                out << "I-b";
                break;
            case StellarClass::Lum_II:
                out << "II";
                break;
            case StellarClass::Lum_III:
                out << "III";
                break;
            case StellarClass::Lum_IV:
                out << "IV";
                break;
            case StellarClass::Lum_V:
                out << "V";
                break;
            case StellarClass::Lum_VI:
                out << "VI";
                break;
            case StellarClass::Lum_Unknown:
                out << "?";  //Add ? because the Lum is classified in stardb as unknown
                break;
            }
        }
    }
    else
    {
        out << '?';
    }
}

bool DumpxRefDatabase(istream& in, ostream& out)
{
    // Verify that the star database file has a correct header
    char header[8];
    in.read(header, sizeof header);
    if (strncmp(header, "CELINDEX", sizeof header))
    {
        cerr << "Bad header for cross index\n";
        return false;
    }

    // Verify the version
    uint16_t version = readUshort(in);
    if (version != 0x0100)
    {
        cerr << "Bad version for cross index\n";
        return false;
    }


    uint32_t record = 0;
    for (;;)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << record << endl;
            return false;
        }

        uint32_t catalogNum    = readUint(in);    //HD or SAO number
        if (in.eof())
            break;
        uint32_t celcatalogNum = readUint(in);    //HIP number

        out << catalogNum << ' ';
        out << celcatalogNum << '\n';
        //out << '\n';

        record++;
    }
    return true;
}

char con2find(double Ein, double RA2c, double DE2c)
/*-------------------------------------------------------
C PROGRAM TO FIND CONSTELLATION NAME  FROM A POSITION
C ---> Modified FO, CDS Strasbourg, DARSIN is NOT a standard F77 function...
C    ... but DASIN !!!
C
C THE FIRST RECORD IN THE DATA SET MUST BE THE EQUINOX FOR THE POSITIONS
C  IN THE FORMAT XXXX.X
C THE REMAINING RECORDS MUST BE THE POSITION IN HOURS AND DECIMALS OF
C  AN HOUR FOLLOWED BY THE DECLINATION IN DEGREES AND FRACTIONS OF A
C  DEGREE IN THE FORMAT: F7.4,F8.4
C THE OUTPUT WILL REPEAT THE POSITION ENTERED AND GIVE THE THREE LETTER
C  ABBREVIATION OF THE CONSTELLATION IN WHICH IT IS LOCATED
C
-------------------------------------------------------*/
{
    f2 = fopen("data.dat", "r"); if (!f2) perror("data.dat") ;

    E = Ein; //(ICRS, Epoch=JJJJ.nn)

    RAH = RA2c;
    DECD = DE2c;
    /* PRECESS POSITION TO 1875.0 EQUINOX */
    ARAD = CONVH * RAH;
    DRAD = CONVD * DECD;
    HGTPRC(&ARAD,&DRAD,&E,&E75,&A,&D);
    if (A < 0.)
        A = A + PI4;
    if (A >= PI4)
        A = A - PI4;
    RA  = A/CONVH;
    DEC = D/CONVD;

    /* FIND CONSTELLATION SUCH THAT THE DECLINATION ENTERED IS HIGHER THAN
       THE LOWER BOUNDARY OF THE CONSTELLATION WHEN THE UPPER AND LOWER
       RIGHT ASCENSIONS FOR THE CONSTELLATION BOUND THE ENTERED RIGHT
       ASCENSION
    */
    fseek(f2, 0, 0);
    while(fgets(buf, sizeof(buf), f2)) {
        for (p=buf; isspace(*p); p++);
        RAL = atof(p); while(isgraph(*p)) p++; while(isspace(*p)) p++;
        RAU = atof(p); while(isgraph(*p)) p++; while(isspace(*p)) p++;
        DECL= atof(p); while(isgraph(*p)) p++; while(isspace(*p)) p++;
        CON = p; while(isgraph(*p)) p++; *p = 0;
        if (DECL > DEC) continue;
        if (RAU <= RA) continue;
        if (RAL >  RA) continue;
    /* if CONSTELLATION HAS BEEN FOUND, WRITE RESULT AND RERUN PROGRAM FOR
       NEXT ENTRY.  OTHERWISE, CONTINUE THE SEARCH BY RETURNING TO RAU
    */
        if (RA >= RAL &&  RA <  RAU &&  DECL <= DEC)
        {
          //con = *CON;
          //printf("%s",CON);
          fclose(f2);
          return *CON;
        }
        else
            if(RAU <  RA)
                continue;
            else
            {
                fclose(f2);
                break;
            }
    }
    return *CON;
}

bool DumpStarDatabase(istream& in, ostream& out)
{
    uint32_t hi;         // HI catalog number
    double rafrh;
    double rafrm;
    double rah;        // right ascension hh
    double ram;        // right ascension mm
    double ras;        // right ascension ss
    char   sign;
    double defrh;
    double defrm;
    double ded;        // declination degrees
    double dem;        // declination minutes
    double des;        // declination seconds
    uint16_t sc;         // stellar class
    double ly;         // light years
    char buf[256];     // character buffer


    char header[8];
    in.read(header, sizeof header);
    if (strncmp(header, "CELSTARS", sizeof header))
    {
        cerr << "Missing header in star database.\n";
        return false;
    }

    uint16_t version = readUshort(in);
    if (version != 0x0100)
    {
        cerr << "Unsupported file version " << (version >> 8) << '.' <<
                (version & 0xff) << '\n';
        return false;
    }

    uint32_t nStarsInFile = readUint(in);
    if (!in.good())
    {
        cerr << "Error reading count of stars from database.\n";
        return false;
    }

    out << nStarsInFile << '\n';
    buf[0] = 0;
    printf("\n\nStart converting %d", nStarsInFile);
    printf(" records.\n");

    for (uint32_t i = 0; i < nStarsInFile; i++)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << i << endl;
            return false;
        }
        if (i % 5000 == 0)
            printf(".");

        uint32_t catalogNum    = readUint(in);
        float  x               = readFloat(in);
        float  y               = readFloat(in);
        float  z               = readFloat(in);
        int16_t  absMag        = readShort(in);
        uint16_t stellarClass  = readUshort(in);
        double pi = 3.14159265358979323846;

        //distance = sqrt(x^2 + y^2 + z^2) in ly
        double distance = sqrt(pow(x,2)+pow(y,2)+pow(z,2));

        //ecliptic longitude lambda = atan(-z/x) ==>
        //(atan2 > gives correct quadrant for all atan functions)
        double lambda = atan2(-z,x);

        //ecliptic latitude beta = atan2(y/sqrt(x^2+z^2))
        double beta = atan2(y,(sqrt(pow(x,2)+pow(z,2))));

        //J2000.0 obliquity of the ecliptic (epsilon) = 23.4392911 deg (deg2rad)
        double epsilon = 23.4392911/180.0*pi;

        //Convert ecliptic coordinates to RA (alpha)
        double RAc = (atan2((sin(lambda)*cos(epsilon)-tan(beta)*sin(epsilon)),(cos(lambda))))*(180.0f/pi);
        if (RAc < 0)
        {
            RAc = RAc + 360;
        }

        //Convert ecliptic coordinates to declination (delta)
        double dec = (asin(sin(beta)*cos(epsilon)+cos(beta)*sin(epsilon)*sin(lambda)))* (180.0f/pi);

        // 1 parsec is 3.26167 ly > 1 ly = 0,30659141 parsec
        double appMag = absMag/256.0 - 5.0 + 5.0*log10(distance/3.26167f);
        double parallax = 3.26167f/distance*1000;

        hi = catalogNum;
        double ra = RAc;
        double de = dec;
        double pa = parallax;
        double am = appMag;
        sc = stellarClass;
        ly = distance;

        rafrh=modf((ra/15.0),&rah);
        rafrm=modf((rafrh*60),&ram);
        ras=rafrm*60;
        defrh=modf(de,&ded);
        defrm=modf((defrh*60),&dem);
        des=defrm*60;
        if (de < 0.0)
        {
            sign = '-';
            ded = -ded;
            dem = -dem;
            des = -des;
        }
        else if (de > 0.0)
            sign= '+';

        // write HIP number
        sprintf(buf,"%d  ",hi);
        out << std::setfill (' ') << std::setw(8) << buf;

        // write data
        sprintf(buf,"%.9f ",ra);   // ra in degrees
        out << std::setfill (' ') << std::setw(14) << buf;

        sprintf(buf,"%c",sign);     // +/-sign
        out << buf;
        double abs_de = abs(de);
        sprintf(buf,"%.9f ",abs_de);    //de in degrees
        out << std::setfill ('0') << std::setw(13) << buf;

        sprintf(buf,"%.6f ",ly);
        out << std::setfill (' ') << std::setw(12) << buf;

        sprintf(buf,"%.2f ",am);
        out << std::setfill (' ') << std::setw(6) << buf;
        //For test purpose only - write the sc-bytes in hex(0xnnnn):
        //   out << "0x" << std::setfill ('0') << std::setw(4) << hex << stellarClass;
        //   out << ' ';
        printStellarClass(sc, out);

        // write system end tag
        sprintf(buf,"\n",hi);
        out << buf;
   }

   printf("\n\n      Done....!\n");
   return true;
}

bool Convert2csv(istream& in, ostream& out,bool all)
{
    uint32_t hi;        // HI catalog number
    double rafrh;
    double rafrm;
    double rah;       // right ascension hh
    double ram;       // right ascension mm
    double ras;       // right ascension ss
    char   sign;
    double defrh;
    double defrm;
    double ded;       // declination degrees
    double dem;       // declination minutes
    double des;       // declination seconds
    uint16_t sc;        // stellar class
    double ly;        // light years
    char buf[256];    // character buffer


    char header[8];
    in.read(header, sizeof header);
    if (strncmp(header, "CELSTARS", sizeof header))
    {
        cerr << "Missing header in star database.\n";
        return false;
    }

    uint16_t version = readUshort(in);
    if (version != 0x0100)
    {
        cerr << "Unsupported file version " << (version >> 8) << '.' <<
                (version & 0xff) << '\n';
        return false;
    }

    uint32_t nStarsInFile = readUint(in);
    if (!in.good())
    {
        cerr << "Error reading count of stars from database.\n";
        return false;
    }

    if (all)
    {
        buf[0] = 0;
        int len = 0;
        printf("\n");
        printf("  Enter a year. and 2 digits. Leave empty for Epoch J1991.25\n");
        printf("  as been used for RA and DEC in the Hipparcos table.\n\n");
        printf("  CONSTELLATIONS derived from POSITION for Epoch(JJJJ.nn): ");
        gets(buf);
        len = StringLength(buf);
        printf("%d", len);
        if (len > 4)
        {
            Ein = atof(buf) ;
        }
        else if (len == 0 )
        {
            sprintf(buf,"%s",dEin);
            Ein = atof(buf);
        }
        else
        {
            printf("\nWrong input. Epoch 1991.25 will be used instead.");
            sprintf(buf,"%s",dEin);
            Ein = atof(buf);
        }
        printf("\n");
        printf(" %d ", len);
        printf("Start converting %d",nStarsInFile);
        printf(" records for Epoch %6.2f",Ein);
        printf("\n");
        // write header
        sprintf(buf,"HIP number;Constellation(J%6.2f);RAdeg(J%6.2f);DECdeg(J%6.2f);",Ein,Ein,Ein);
        out << buf;
        sprintf(buf,"RA(hms);DEC(dms);Distance(LY);Parallax;AppMag;AbsMag;Stellar Class\n");
        out << buf;
    }
    else
        out << HEADER_NEW;
        //HEADER_NEW = "HIP number;RAdeg;DECdeg;RA(hms);DEC(dms);Distance(LY);Parallax;AppMag;AbsMag;StellarClass\n"

    for (uint32_t i = 0; i < nStarsInFile; i++)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << i << endl;
            return false;
        }
        if (i % 5000 == 0)
            printf(".");

        uint32_t catalogNum   = readUint(in);
        float  x              = readFloat(in);
        float  y              = readFloat(in);
        float  z              = readFloat(in);
        int16_t  absMag       = readShort(in);
        uint16_t stellarClass = readUshort(in);

        double pi = 3.14159265358979323846;
        double distance = sqrt(pow(x,2)+pow(y,2)+pow(z,2));
        double lambda = atan2(-z,x);
        double beta = atan2(y,(sqrt(pow(x,2)+pow(z,2))));
        double epsilon = 23.4392911/180.0*pi;
        double RAc = (atan2((sin(lambda)*cos(epsilon)-tan(beta)*sin(epsilon)),(cos(lambda))))*(180.0f/pi);
        if (RAc < 0)
        {
            RAc = RAc + 360;
        }
        double dec = (asin(sin(beta)*cos(epsilon)+cos(beta)*sin(epsilon)*sin(lambda)))* (180.0f/pi);
        double appMag = absMag/256.0 - 5.0 + 5.0*log10(distance/3.26167f);
        double parallax = 3.26167f/distance*1000;

        hi = catalogNum;
        double ra = RAc;
        double de = dec;
        double pa = parallax;
        double am = appMag;
        sc = stellarClass;
        ly = distance;

        rafrh=modf((ra/15.0),&rah);
        rafrm=modf((rafrh*60),&ram);
        ras=rafrm*60;
        defrh=modf(de,&ded);
        defrm=modf((defrh*60),&dem);
        des=defrm*60;
        if (de < 0.0)
        {
            sign='-';
            ded=-ded;
            dem=-dem;
            des=-des;
        }
        else if (de >= 0.0)
            sign= '+';

        // write HIP number
        sprintf(buf,"%d;",hi);
        out << buf;

        // write data
        if (all)
        {
            con2find(Ein,RA2c,DE2c);

            sprintf(buf,"%s;",CON);
            out << buf;
        }
        sprintf(buf,"%.9f;",ra);   // ra in degrees
        out << buf;

        sprintf(buf,"%.9f;",de);    //de in degrees
        out << buf;

        sprintf(buf,"%.f:",rah);
        out << std::setfill ('0') << std::setw(3) << buf;
        sprintf(buf,"%.f:",ram);
        out << std::setfill ('0') << std::setw(3) << buf;
        sprintf(buf,"%.2f;",ras);
        out << buf;

        sprintf(buf,"%c",sign);     // +/-sign
        out << buf;
        sprintf(buf,"%.f:",ded);     //de in degrees
        out << std::setfill ('0') << std::setw(3) << buf;
        sprintf(buf,"%.f:",dem);     //de minutes
        out << std::setfill ('0') << std::setw(3) << buf;
        sprintf(buf,"%.2f;",des);     //de seconds
        out << buf;

        sprintf(buf,"%.3f;",ly);
        out << buf;

        sprintf(buf,"%.2f;",pa);
        out << buf;

        sprintf(buf,"%.2f;",am);
        out << buf;

        sprintf(buf,"%.4f;",((float) absMag / 256.0f));
        out << buf;

        RA2c = ra * 24.0 / 360.0;
        DE2c = de;

        printStellarClass(stellarClass, out);

        // write system end tag
        sprintf(buf,"  \n",hi);
        out << buf;
    }

    printf("\n\n      Done....!\n");
    return true;
}

bool Const2csv(istream& in, ostream& out)
{
    uint32_t hi;        // HI catalog number
    double rafrh;
    double rafrm;
    double rah;       // right ascension hh
    double ram;       // right ascension mm
    double ras;       // right ascension ss
    char   sign;
    double defrh;
    double defrm;
    double ded;       // declination degrees
    double dem;       // declination minutes
    double des;       // declination seconds
    uint16_t sc;        // stellar class
    double ly;        // light years
    char buf[256];    // character buffer

    char header[8];
    in.read(header, sizeof header);
    if (strncmp(header, "CELSTARS", sizeof header))
    {
        cerr << "Missing header in star database.\n";
        return false;
    }

    uint16_t version = readUshort(in);
    if (version != 0x0100)
    {
        cerr << "Unsupported file version " << (version >> 8) << '.' <<
                (version & 0xff) << '\n';
        return false;
    }

    uint32_t nStarsInFile = readUint(in);
    if (!in.good())
    {
        cerr << "Error reading count of stars from database.\n";
        return false;
    }

    buf[0] = 0;
    int len = 0;
    printf("\n");
    printf("  Enter a year. and 2 digits. Leave empty is Epoch J1991.25\n");
    printf("  as been used for RA and DEC in the Hipparcos table.\n\n");
    printf("  CONSTELLATIONS derived from POSITION for Epoch(JJJJ.nn): ") ;
    gets(buf);
    len = StringLength(buf);

    if (len > 4)
    {
        Ein = atof(buf) ;
    }
    else if (len == 0 )
    {
        sprintf(buf,"%s",dEin);
        Ein = atof(buf);
    }
    else
    {
        printf("\nWrong input. Epoch 1991.25 will be used instead.");
        sprintf(buf,"%s",dEin);
        Ein = atof(buf);
    }
    printf("\n\n");
    printf(" %d ", len);
    printf("Start converting %d",nStarsInFile);
    printf(" records for Epoch %6.2f",Ein);
    printf("\n");

    // write header
    sprintf(buf,"HIP number;Constellation(J%6.2f);RAdeg(J%6.2f);DecDeg(J%6.2f);",Ein,Ein,Ein);
    out << buf;
    sprintf(buf,"RA2c(h);DE2c(Deg);RAh(B1875.0);DECdeg(B1875.0)\n");
    out << buf;

    // calculate data for each star
    for (uint32_t i = 0; i < nStarsInFile; i++)
    {
        if (!in.good())
        {
            cerr << "Error reading from star database at record " << i << endl;
            return false;
        }
        if (i % 5000 == 0)
             printf(".");
        // read stardb.dat
        uint32_t catalogNum   = readUint(in);
        float  x              = readFloat(in);
        float  y              = readFloat(in);
        float  z              = readFloat(in);
        int16_t  absMag       = readShort(in);
        uint16_t stellarClass = readUshort(in);

        double pi = 3.14159265358979323846;
        double distance = sqrt(pow(x,2)+pow(y,2)+pow(z,2));
        double lambda = atan2(-z,x);
        double beta = atan2(y,(sqrt(pow(x,2)+pow(z,2))));
        double epsilon = 23.4392911/180.0*pi;
        double RAc = (atan2((sin(lambda)*cos(epsilon)-tan(beta)*sin(epsilon)),(cos(lambda))))*(180.0f/pi);
        if (RAc < 0)
        {
            RAc = RAc + 360;
        }
        double dec = (asin(sin(beta)*cos(epsilon)+cos(beta)*sin(epsilon)*sin(lambda)))* (180.0f/pi);
        double appMag = absMag/256.0 - 5.0 + 5.0*log10(distance/3.26167f);
        double parallax = 3.26167f/distance*1000;
        hi = catalogNum;
        double ra = RAc;
        double de = dec;
        double pa = parallax;
        double am = appMag;
        sc = stellarClass;
        ly = distance;
        rafrh=modf((ra/15.0),&rah);
        rafrm=modf((rafrh*60),&ram);
        ras=rafrm*60;
        defrh=modf(de,&ded);
        defrm=modf((defrh*60),&dem);
        des=defrm*60;
        if (de < 0.0)
        {
            sign='-';
            ded=-ded;
            dem=-dem;
            des=-des;
        }
        else sign= '+';
        RA2c = ra * 24.0 / 360.0;   // to be used for constellation
        DE2c = de;
        //Determine Constellation
        con2find(Ein,RA2c,DE2c);

        // write HIP number
        sprintf(buf,"%d;",hi);
        out << buf;

        // write Constellation
        sprintf(buf,"%s;",CON);
        out << buf;

        // write data

        // ra in degrees for Epoch JYYYY.nn (default = J1991.25 > Hipparcos)
        sprintf(buf,"%.9f;",ra);
        out << buf;

        //de in degrees for Epoch JYYYY.nn (default = J1991.25 > Hipparcos)
        sprintf(buf,"%.9f;",de);
        out << buf;

        // ra in hh.ddddd - input for determine constellation
        sprintf(buf,"%.8f;",RA2c);
        out << buf;

        //de in degrees - input for determine constellation
        sprintf(buf,"%.8f;",DE2c);
        out << buf;

        // ra in hh.ddddd - output RA Epoch B1875.0
        sprintf(buf,"%.8f;",RA);
        out << buf;

        //de in degrees - output DE Epoch B1875.0
        sprintf(buf,"%.8f",DEC);
        out << buf;

        // write system end tag
        sprintf(buf,"  \n",hi);
        out << buf;
   }

   printf("\n\n      Done....!\n");
   return true;
}

int StringLength (char x[])
{
    int counter = 0;
    for (int index = 0; index < 80; index++)
    {
        if (x[index] == '\0')
            break;
        counter++;

    }
    return counter;
}
bool parseCommandLine(int argc, char* argv[])
{
    int i = 1;
    int fileCount = 0;

    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--all"))
            {
                DumpAll = true;
            }
            else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--convert"))
            {
                convert = true;
            }
            else if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--index"))
            {
                useXindexDump = true;
            }
            else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list"))
            {
                ConstDump = true;
            }
            else
            {
                cerr << "Unknown or wrong command line switch: " << argv[i] << '\n';
                return false;
            }
            i++;
        }
        else
        {
            if (fileCount == 0)
            {
                // input filename first
                inputFilename = string(argv[i]);
                fileCount++;
            }
            else if (fileCount == 1)
            {
                // output filename second
                outputFilename = string(argv[i]);
                fileCount++;
            }
            else
            {
                // more than two filenames on the command line is an error
                return false;
            }
            i++;
        }
    }
    return true;
}


int main(int argc, char* argv[])
{
    if (!parseCommandLine(argc, argv) || inputFilename.empty())
    {
        Usage();
        return 1;
    }

    ifstream stardbFile(inputFilename, ios::in | ios::binary);
    if (!stardbFile.good())
    {
        cerr << "Error opening star database file " << inputFilename << '\n';
        return 1;
    }

    bool success;
    ostream* out = &cout;
    if (!outputFilename.empty())
    {
        out = new ofstream(outputFilename, ios::out);
        if (!out->good())
        {
            cerr << "Error opening output file " << outputFilename << '\n';
            return 1;
        }
    }

    if (useXindexDump)
    {
        success = DumpxRefDatabase(stardbFile, *out);
    }
    else if (convert)
    {
        success = Convert2csv(stardbFile, *out,DumpAll);
    }
    else if (ConstDump)
    {
        success = Const2csv(stardbFile, *out);
    }
    else
    {
        success = DumpStarDatabase(stardbFile, *out);
    }
    return success ? 0 : 1;
}
