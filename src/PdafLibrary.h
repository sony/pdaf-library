/*
Copyright (c)  2016, Sony Corporation All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __PDAF_LIBRARY_H__
#define __PDAF_LIBRARY_H__

/* For PhaseDifference */
#define D_PD_ERROR_VALUE                            (-64)   /* This value is different by each sensor type */
                                                            /* Please set value according to your environment */   
                                                            /* IMX230, IMX298, IMX330, IMX338 : -32 */
                                                            /* Other sensor type : -64 */
/* For DensityOfPhasePix */
#define D_PD_LIB_DENSITY_SENS_MODE0                 (2304)  /* Density of phase detection pixel of mode 0 */
#define D_PD_LIB_DENSITY_SENS_MODE1                 (2304)  /* Density of phase detection pixel of mode 1 */
#define D_PD_LIB_DENSITY_SENS_MODE2                 (2304/2)/* Density of phase detection pixel of mode 2 */
#define D_PD_LIB_DENSITY_SENS_MODE3                 (2304/2)/* Density of phase detection pixel of mode 3 */
#define D_PD_LIB_DENSITY_SENS_MODE4                 (2304/4)/* Density of phase detection pixel of mode 4 */

/* For AdjCoeffSlope */
#define D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE0         (2304)  /* Adjustment coefficient of slope of mode 0 */
#define D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE1         (2304)  /* Adjustment coefficient of slope of mode 0 */
#define D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE2         (2304)  /* Adjustment coefficient of slope of mode 0 */
#define D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE3         (2304)  /* Adjustment coefficient of slope of mode 0 */
#define D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE4         (2304)  /* Adjustment coefficient of slope of mode 0 */

#define D_PD_LIB_E_OK                               (0)     /* OK value */
#define D_PD_LIB_E_NG                               (-1)    /* NG value of DefocusConfidence */

#define ENCWDDON                                    (2)     /* NCW determines Defocus OK/NG by disable this functionality */
#define EPDVALERR                                   (3)     /* Input error value of phase difference data */
#define EINXSOI                                     (40)    /* XSizeOfImage Input out of range */
#define EINYSOI                                     (41)    /* YSizeOfImage Input out of range */
#define EINPDAFWX                                   (42)    /* PDAFWindowsX Input out of range */
#define EINPDAFWY                                   (43)    /* PDAFWindowsY Input out of range */
#define EINSO                                       (44)    /* SlopeOffset Input out of range */
#define EINACS                                      (45)    /* AdjCoeffSlope Input out of range */
#define EINSOXAK                                    (46)    /* SlopeOffsetXAddressKnot Input out of range */
#define EINSOYAK                                    (47)    /* SlopeOffsetYAddressKnot Input out of range */
#define EINVALDISCONFJ                              (48)    /* Invalid of Disable Confidence Judgement */
#define EINVALDISIHC                                (49)    /* Invalid of Disable compensation relation with image height */
#define EINDONTPN                                   (50)    /* DefocusOKNGThrPointNum Input out of range */
#define EINDONXAK                                   (51)    /* DefocusOKNGXAddressKnot Input out of range */
#define EINDONYAK                                   (52)    /* DefocusOKNGYAddressKnot Input out of range */
#define EINDOP                                      (53)    /* DensityOfPhasePix Input out of range */
#define ELDCL                                       (80)    /* Low DefocusConfidenceLevel */

typedef struct
{
    unsigned long       MajorVersion;               /* Integer part of PDAF Library version. */
    unsigned long       MinorVersion;               /* Decimal part of PDAF Library version. */
} PdLibVersion_t;

typedef struct
{
    unsigned long       PointNum;                   /* Number of points on the threshold line. */
    unsigned long       *p_AnalogGain;              /* Array of x address of points on the threshold line. */
    unsigned long       *p_Confidence;              /* Array of y address of points on the threshold line. */
} DefocusOKNGThrLine_t;

typedef struct
{
    signed long         PhaseDifference;            /* Phase difference data which is output data from image sensor. */
    unsigned long       ConfidenceLevel;            /* Confidence level which is output data from image sensor. */
    unsigned short      XSizeOfImage;               /* X size of image in all-pixel mode */
    unsigned short      YSizeOfImage;               /* Y size of image in all-pixel mode. */
    unsigned short      XAddressOfWindowStart;      /* X address of PDAF window start position in all-pixel mode. */
    unsigned short      YAddressOfWindowStart;      /* Y address of PDAF window start position in all-pixel mode. */
    unsigned short      XAddressOfWindowEnd;        /* X address of PDAF window end position in all-pixel mode. */
    unsigned short      YAddressOfWindowEnd;        /* Y address of PDAF window end position in all-pixel mode. */
    unsigned short      XKnotNumSlopeOffset;        /* Number of knots in x-direction. */
    unsigned short      YKnotNumSlopeOffset;        /* Number of knots in y-direction. */
    signed long         *p_SlopeData;               /* Array of slope data. */
    signed long         *p_OffsetData;              /* Array of offset data. */
    unsigned short      *p_XAddressKnotSlopeOffset; /* Array of x address of knots. */
    unsigned short      *p_YAddressKnotSlopeOffset; /* Array of y address of knots. */
    signed long         AdjCoeffSlope;              /* Adjustment coefficient of slope. */
    unsigned long       ImagerAnalogGain;           /* Image sensor analog gain. */
    unsigned short      XKnotNumDefocusOKNG;        /* Number of knots in x-direction. */
    unsigned short      YKnotNumDefocusOKNG;        /* Number of knots in y-direction. */
    DefocusOKNGThrLine_t    *p_DefocusOKNGThrLine;  /* Array of threshold line data which determines Defocus OK/NG. */
    unsigned short      *p_XAddressKnotDefocusOKNG; /* Array of x address of knots. */
    unsigned short      *p_YAddressKnotDefocusOKNG; /* Array of y address of knots. */
    unsigned long       DensityOfPhasePix;          /* Density of phase detection pixel. */
} PdLibInputData_t;

typedef struct
{
    signed long         Defocus;                    /* Defocus. Unit is DN (Digital Number). */
    signed char         DefocusConfidence;          /* Defocus OK/NG. */
    unsigned long       DefocusConfidenceLevel;     /* Defocus OK/NG level. */
    signed long         PhaseDifference;            /* Phase difference which is the same information as input data. */
} PdLibOutputData_t;

/* ------- PdLibGetVersion API */
#ifdef __cplusplus 
extern "C" {
#endif

#if defined __GNUC__
__attribute__ ((visibility ("default"))) void PdLibGetVersion
#elif defined(_DLL)
__declspec( dllexport ) void PdLibGetVersion
#else
extern void PdLibGetVersion                         /* Get version information of PDAF Library. */
#endif
( 
    PdLibVersion_t  *pfa_PdLibVersion
);

/* ------- PdLibGetDefocus API */
#if defined __GNUC__
__attribute__ ((visibility ("default"))) signed long PdLibGetDefocus
#elif defined(_DLL)
__declspec( dllexport ) signed long PdLibGetDefocus
#else
extern signed long PdLibGetDefocus                  /* Get defocus data according to a PDAF window. */
#endif
(
    PdLibInputData_t    *pfa_PdLibInputData,        /* Input data needed for defocus data output. */
    PdLibOutputData_t   *pfa_PdLibOutputData        /* Defocus data. */
);

#ifdef __cplusplus
}
#endif          /* __cplusplus */

#endif
