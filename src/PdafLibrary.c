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

/****************************************************************/
/*                          include                             */
/****************************************************************/

#include "PdafMathFunc.h"
#include "PdafLibrary.h"

/****************************************************************/
/*                          version                             */
/****************************************************************/

#define D_MAJOR_VERSION (1)                         /* Integer part of PDAF Library version. */
#define D_MINOR_VERSION (00)                        /* Decimal part of PDAF Library version. */

/****************************************************************/
/*                 local function declaration                   */
/****************************************************************/

static void job_init_output_data ( PdLibOutputData_t *pfa_OutputData );
static signed long job_check_input ( PdLibInputData_t *pfa_InputData );
static void job_calc_defocus ( PdLibInputData_t *pfa_InputData, signed long *pfa_Defocus );
static void job_calc_defocus_confidence_level ( PdLibInputData_t *pfa_InputData, unsigned long *pfa_DefocusConfidenceLevel );
static void job_calc_defocus_confidence ( unsigned long fa_DefocusConfidenceLevel, signed char *pfa_DefocusConfidence );
static void job_calc_phase_difference ( PdLibInputData_t *pfa_InputData, signed long *pfa_PhaseDifference );

static signed long calc_defocus_formula ( PdLibInputData_t *pfa_InputData, unsigned short fa_Index );
static signed long limit_defocus_formula ( double fa_Defocus );
static signed long calc_defocus_ok_ng_thr ( PdLibInputData_t *pfa_InputData, unsigned short fa_Index );
static unsigned long limit_defocus_confidence_level ( double fa_DefocusConfidenceLevel );

/****************************************************************/
/*                      external function                       */
/****************************************************************/
/* API : Get version information of PDAF Library. */
extern void PdLibGetVersion
( 
    PdLibVersion_t  *pfa_PdLibVersion                       /* Output : PDAF Library version */
)
{
    (*pfa_PdLibVersion).MajorVersion = D_MAJOR_VERSION;     /* Set integer part of PDAF Library version. */
    (*pfa_PdLibVersion).MinorVersion = D_MINOR_VERSION;     /* Set decimal part of PDAF Library version. */

    return ;
}

/* API : Get defocus data according to a PDAF window. */
extern signed long PdLibGetDefocus 
(
    PdLibInputData_t    *pfa_PdLibInputData,                /* Input  : Input data structure */
    PdLibOutputData_t   *pfa_PdLibOutputData                /* Output : Output data structure */
)
{
    signed long ret;
    signed long RetCheckInput;
    PdLibOutputData_t PdLibOutputData;

    job_init_output_data ( pfa_PdLibOutputData );           /* Initialization of  output data structure */

    RetCheckInput = job_check_input ( pfa_PdLibInputData ); /* Check value of input data structure */

    if ( RetCheckInput != D_PD_LIB_E_OK ) {                 /* Check the value of input */
        ret = RetCheckInput;
        return ret;                                         /* Return error value */
    } else {
        ret = D_PD_LIB_E_OK;                                /* Set return value as OK */
    }

    job_calc_defocus ( pfa_PdLibInputData, &(PdLibOutputData.Defocus) );    /* Calculate defocus */

    /* Check XKnotNumDefocusOKNG and YKnotNumDefocusOKNG */ 
    if ((pfa_PdLibInputData->XKnotNumDefocusOKNG != 0) && (pfa_PdLibInputData->YKnotNumDefocusOKNG != 0)) {
        /* Check the value of input */
        if ( (*pfa_PdLibInputData).PhaseDifference != ( D_PD_ERROR_VALUE << 4 ) ) {
            /* Calculate defocus confidence level */
            job_calc_defocus_confidence_level(pfa_PdLibInputData, &(PdLibOutputData.DefocusConfidenceLevel));
            /* Calculate defocus confidence */
            job_calc_defocus_confidence ( PdLibOutputData.DefocusConfidenceLevel, &(PdLibOutputData.DefocusConfidence) );
        } else {                                            /* Error of phase difference */
            PdLibOutputData.DefocusConfidenceLevel = 0;     /* Set defocus confidence level as Zero */
            PdLibOutputData.DefocusConfidence = -EPDVALERR; /* Set defocus confidence as input error */
        }

    } else {                                                /* Error of XKnotNumDefocusOKNG or YKnotNumDefocusOKNG */
        PdLibOutputData.DefocusConfidenceLevel = 0;         /* Set defocus confidence level as Zero */
        PdLibOutputData.DefocusConfidence = -ENCWDDON;      /* Set defocus confidence as NCW */
    }

    /* Calculate phase difference */
    job_calc_phase_difference ( pfa_PdLibInputData, &(PdLibOutputData.PhaseDifference) );

    (*pfa_PdLibOutputData) = PdLibOutputData;               /* Set result of job_calc_phase_difference() */

    return ret;                                             /* Return OK */
}

/****************************************************************/
/*                       local function                         */
/****************************************************************/
/* Function for initializing output data structure */
static void job_init_output_data 
( 
    PdLibOutputData_t *pfa_OutputData                       /* Output : Output data structure */
)
{
    (*pfa_OutputData).Defocus                = 0;
    (*pfa_OutputData).DefocusConfidence      = D_PD_LIB_E_NG;
    (*pfa_OutputData).DefocusConfidenceLevel = 0;
    (*pfa_OutputData).PhaseDifference        = 0;

    return ;
}

/* Function for checking value of input data structure */
static signed long job_check_input 
( 
    PdLibInputData_t *pfa_InputData                         /* Input : Input data structure */
)
{
    signed long ret;

    ret = D_PD_LIB_E_OK;                                    /* Set return value as OK */

    /* Check the value of input */
    /* Check XSizeOfImage */
    if ( 2 <= (*pfa_InputData).XSizeOfImage ) {             /* Check XSizeOfImage */
    } else {
        ret = -EINXSOI;                                     /* Out of range of XSizeOfImage */
    }
    /* Check YSizeOfImage */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        if ( 2 <= (*pfa_InputData).YSizeOfImage ) {         /* Check YSizeOfImage */
        } else {
            ret = -EINYSOI;                                 /* Out of range of YSizeOfImage */
        }
    }
    /* Check PDAFWindowsX */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        if ( ( (*pfa_InputData).XAddressOfWindowStart <= ( (*pfa_InputData).XAddressOfWindowEnd - 1 ) ) &&
             ( (*pfa_InputData).XAddressOfWindowEnd <= ( (*pfa_InputData).XSizeOfImage - 1 ) ) ) {
        } else {
            ret = -EINPDAFWX;                               /* Out of range of PDAFWindowsX */
        }
    }
    /* Check PDAFWindowsY */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        if ( ( (*pfa_InputData).YAddressOfWindowStart <= ( (*pfa_InputData).YAddressOfWindowEnd - 1 ) ) &&
             ( (*pfa_InputData).YAddressOfWindowEnd <= ( (*pfa_InputData).YSizeOfImage - 1 ) ) ) {
        } else {
            ret = -EINPDAFWY;                               /* Out of range of PDAFWindowsY */
        }
    }
    /* Check Slope and Offset (defocus vs phase difference) */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        if ( 2 <= (*pfa_InputData).XKnotNumSlopeOffset &&
             2 <= (*pfa_InputData).YKnotNumSlopeOffset ) {
        } else {
            ret = -EINSO;                                   /* Out of range of SlopeOffset */
        }
    }
    /* Check AdjCoeffSlope */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        if ( ( (*pfa_InputData).AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE0 ) ||
             ( (*pfa_InputData).AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE1 ) ||
             ( (*pfa_InputData).AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE2 ) ||
             ( (*pfa_InputData).AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE3 ) ||
             ( (*pfa_InputData).AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE4 ) ) {
        } else {
            ret = -EINACS;                                  /* Out of range of AdjCoeffSlope */
        }
    }
    /* Check SlopeOffsetXAddressKnot */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        unsigned short i;
        unsigned short XKnotNum;
        unsigned short *p_XAddressKnot;
        
        XKnotNum = (*pfa_InputData).XKnotNumSlopeOffset;
        p_XAddressKnot = (*pfa_InputData).p_XAddressKnotSlopeOffset;
        for ( i = 0; i < ( XKnotNum - 1 ); i++ ) {
            if ( p_XAddressKnot[i] < p_XAddressKnot[i + 1] ){
            } else {
                ret = -EINSOXAK;                            /* Out of range of SlopeOffsetXAddressKnot */
                break ;
            }
        }
    }
    /* Check SlopeOffsetYAddressKnot */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        unsigned short i;
        unsigned short YKnotNum;
        unsigned short *p_YAddressKnot;
        
        YKnotNum = (*pfa_InputData).YKnotNumSlopeOffset;
        p_YAddressKnot = (*pfa_InputData).p_YAddressKnotSlopeOffset;
        for ( i = 0; i < ( YKnotNum - 1 ); i++ ) {
            if ( p_YAddressKnot[i] < p_YAddressKnot[i + 1] ) {
            } else {
                ret = -EINSOYAK;                            /* Out of range of SlopeOffsetYAddressKnot */
                break ;
            }
        }
    }
    /* Check Defocus OK/NG */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        /* Check disable of the judge function of confidence */
        if ( 0 == (*pfa_InputData).XKnotNumDefocusOKNG ||
             0 == (*pfa_InputData).YKnotNumDefocusOKNG ) {
            if ( 0 == (*pfa_InputData).XKnotNumDefocusOKNG &&
                 0 == (*pfa_InputData).YKnotNumDefocusOKNG ) {
            }
            else {
                ret = -EINVALDISCONFJ;                      /* Invalid of Disable Confidence Judgement */
            }
        }
        /* Check disable of the judge function of confidence in each image area */
        else if ( 1 == (*pfa_InputData).XKnotNumDefocusOKNG ||
                  1 == (*pfa_InputData).YKnotNumDefocusOKNG ) {
            if ( 1 == (*pfa_InputData).XKnotNumDefocusOKNG &&
                 1 == (*pfa_InputData).YKnotNumDefocusOKNG ) {
            } else {
                ret = -EINVALDISIHC;                        /* Invalid of Disable compensation relation with image height */
            }
        }
    }
    /* Check DefocusOKNGThrPointNum */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        if ( 2 <= (*((*pfa_InputData).p_DefocusOKNGThrLine)).PointNum ) {
        } else {
            ret = -EINDONTPN;                               /* Out of range of DefocusOKNGThrPointNum */
        }
    }
    /* Check DefocusOKNGXAddressKnot */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        unsigned short i;
        unsigned short XKnotNum;
        unsigned short *p_XAddressKnot;
        
        XKnotNum = (*pfa_InputData).XKnotNumDefocusOKNG;
        p_XAddressKnot = (*pfa_InputData).p_XAddressKnotDefocusOKNG;
        for ( i = 0; i < ( XKnotNum - 1 ); i++ ) {
            if ( p_XAddressKnot[i] < p_XAddressKnot[i + 1] ) {
            } else {
                ret = -EINDONXAK;                           /* Out of range of DefocusOKNGXAddressKnot */
                break ;
            }
        }
    }
    /*  Check DefocusOKNGYAddressKnot */
    if ( ret == D_PD_LIB_E_OK ) {                           /* Check return value */
        unsigned short i;
        unsigned short YKnotNum;
        unsigned short *p_YAddressKnot;
        
        YKnotNum = (*pfa_InputData).YKnotNumDefocusOKNG;
        p_YAddressKnot = (*pfa_InputData).p_YAddressKnotDefocusOKNG;
        for ( i = 0; i < ( YKnotNum - 1 ); i++ ) {
            if ( p_YAddressKnot[i] < p_YAddressKnot[i + 1] ) {
            } else {
                ret = -EINDONYAK;                           /* Out of range of DefocusOKNGYAddressKnot */
                break ;
            }
        }
    }
    /* Check Phase Detection Pixel Density */
    if ( ret == D_PD_LIB_E_OK ) {               /* Check return value */
        if ( ( (*pfa_InputData).DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE0 ) ||
             ( (*pfa_InputData).DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE1 ) ||
             ( (*pfa_InputData).DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE2 ) ||
             ( (*pfa_InputData).DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE3 ) ||
             ( (*pfa_InputData).DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE4 ) ) {
        } else {
            ret = -EINDOP;                                  /* Out of range of DensityOfPhasePix */
        }
    }

    return ret;                                             /* Return result */
}

/* Function for calculating defocus */
static void job_calc_defocus 
( 
    PdLibInputData_t *pfa_InputData,                        /* Input  : Input data structure */
    signed long *pfa_Defocus                                /* Output : Defocus */
)
{
    unsigned short  i;
    unsigned short  XKnotNum;
    unsigned short  YKnotNum;
    unsigned short  *p_XAddressKnot;
    unsigned short  *p_YAddressKnot;
    unsigned short  XKnotStart;
    unsigned short  YKnotStart;
    unsigned char   AreaIndex;
    signed long     Defocus;
    signed long     XAddressPDAFWindowCenter;
    signed long     YAddressPDAFWindowCenter;

    XKnotNum = (*pfa_InputData).XKnotNumSlopeOffset;
    YKnotNum = (*pfa_InputData).YKnotNumSlopeOffset;

    p_XAddressKnot = (*pfa_InputData).p_XAddressKnotSlopeOffset;
    p_YAddressKnot = (*pfa_InputData).p_YAddressKnotSlopeOffset;

    XAddressPDAFWindowCenter = ( (*pfa_InputData).XAddressOfWindowStart + 
                                 (*pfa_InputData).XAddressOfWindowEnd ) / 2;
    YAddressPDAFWindowCenter = ( (*pfa_InputData).YAddressOfWindowStart + 
                                 (*pfa_InputData).YAddressOfWindowEnd ) / 2;
    
    XKnotStart = 0;
    for ( i = 0; i < XKnotNum-1; i++ ) {                    /* Check XKnotStart */
        if ( p_XAddressKnot[i] <= XAddressPDAFWindowCenter && 
             XAddressPDAFWindowCenter <= p_XAddressKnot[i+1] ) {
            XKnotStart = i;
            break ;
        }
    }

    YKnotStart = 0;
    for ( i = 0; i < YKnotNum-1; i++ ) {                    /* Check YKnotStar */
        if ( p_YAddressKnot[i] <= YAddressPDAFWindowCenter && 
             YAddressPDAFWindowCenter <= p_YAddressKnot[i+1] ) {
            YKnotStart = i;
            break ;
        }
    }

/*

    Divided by area. AreaIndex

    +---+---+---+
    | 0 | 1 | 2 |
    +---+---+---+
    | 3 | 4 | 5 |
    +---+---+---+
    | 6 | 7 | 8 |
    +---+---+---+

    Area 4 is surrounded by the knot points.
    Other area does not have knot points.

*/

    if ( YAddressPDAFWindowCenter < p_YAddressKnot[0] ) {
        /* Top */
             if ( XAddressPDAFWindowCenter   < p_XAddressKnot[0]        ) {/* Left   */ AreaIndex = 0;}
        else if ( p_XAddressKnot[XKnotNum-1] < XAddressPDAFWindowCenter ) {/* Right  */ AreaIndex = 2;}
        else                                                              {/* Center */ AreaIndex = 1;}       
    }
    else if ( p_YAddressKnot[YKnotNum-1] < YAddressPDAFWindowCenter ) {
        /* Bottom */
             if ( XAddressPDAFWindowCenter   < p_XAddressKnot[0]        ) {/* Left   */ AreaIndex = 6;}
        else if ( p_XAddressKnot[XKnotNum-1] < XAddressPDAFWindowCenter ) {/* Right  */ AreaIndex = 8;}
        else                                                              {/* Center */ AreaIndex = 7;}       
    } else {
        /* Center */
             if ( XAddressPDAFWindowCenter   < p_XAddressKnot[0]        ) {/* Left   */ AreaIndex = 3;}
        else if ( p_XAddressKnot[XKnotNum-1] < XAddressPDAFWindowCenter ) {/* Right  */ AreaIndex = 5;}
        else                                                              {/* Center */ AreaIndex = 4;}
    }

    if ( AreaIndex == 4 ) {                                 /* Center */
        unsigned short  Index;
        signed long     LineX[2];
        signed long     LineY[2];
        signed long     PlaneZ[4];
        signed long     PointX;
        signed long     PointY;
        signed long     PointZ = 0;
        
        Index = YKnotStart*XKnotNum+XKnotStart;

        LineX[0]  = p_XAddressKnot[XKnotStart  ];
        LineX[1]  = p_XAddressKnot[XKnotStart+1];           /* Next to LineX[0] */
        LineY[0]  = p_YAddressKnot[YKnotStart  ];
        LineY[1]  = p_YAddressKnot[YKnotStart+1];           /* Next to LineY[0] */

        /* Calculate defocus value of each knot point */
        PlaneZ[0] = calc_defocus_formula ( pfa_InputData, Index            );
        PlaneZ[1] = calc_defocus_formula ( pfa_InputData, Index+1          );
        PlaneZ[2] = calc_defocus_formula ( pfa_InputData, Index+XKnotNum   );
        PlaneZ[3] = calc_defocus_formula ( pfa_InputData, Index+XKnotNum+1 );

        PointX    = XAddressPDAFWindowCenter;
        PointY    = YAddressPDAFWindowCenter;

        /* Calculate coordination at the point of the plane */
        CalcAddressOnPlane_slXslYslZ (LineX, LineY, PlaneZ, PointX, PointY, &PointZ);

        Defocus = PointZ;
    } else if ( AreaIndex == 0 || AreaIndex == 2 || AreaIndex == 6 || AreaIndex == 8 ) {    /* Cornar of area */
        unsigned short Index;
        
             if ( AreaIndex == 2 ) { Index = XKnotNum-1; }
        else if ( AreaIndex == 6 ) { Index = (YKnotNum-1)*XKnotNum; }
        else if ( AreaIndex == 8 ) { Index = YKnotNum*XKnotNum-1; }
        else                       { Index = 0; }

        /* Calculate defocus value which uses slope and offset of index point */
        Defocus = calc_defocus_formula ( pfa_InputData, Index ); 

    } else if ( AreaIndex == 1 ) {                          /* Top Center */
        unsigned short  Index;
        signed long     LineX[2];
        signed long     LineY[2];
        signed long     PointX;
        signed long     PointY;

        Index = XKnotStart;

        LineX[0] = p_XAddressKnot[XKnotStart  ];
        LineX[1] = p_XAddressKnot[XKnotStart+1];            /* Next to LineX[0] */

        /* Calculate defocus value of each knot point */
        LineY[0] = calc_defocus_formula ( pfa_InputData, Index   );
        LineY[1] = calc_defocus_formula ( pfa_InputData, Index+1 ); /* Next to LineY[0] */

        PointX   = XAddressPDAFWindowCenter;
        /* Calculate coordination at the point of the line */
        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        Defocus = PointY;
    } else if ( AreaIndex == 7 ) {                          /* Bottom Center */
        unsigned short  Index;
        signed long     LineX[2];
        signed long     LineY[2];
        signed long     PointX;
        signed long     PointY;

        Index = (YKnotNum-1)*XKnotNum + XKnotStart;

        LineX[0] = p_XAddressKnot[XKnotStart  ];
        LineX[1] = p_XAddressKnot[XKnotStart+1];            /* Next to LineX[0] */

        /* Calculate defocus value of each knot point */
        LineY[0] = calc_defocus_formula ( pfa_InputData, Index   );
        LineY[1] = calc_defocus_formula ( pfa_InputData, Index+1 ); /* Next to LineY[0] */

        PointX   = XAddressPDAFWindowCenter;

        /* Calculate coordination at the point of the line */
        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        Defocus = PointY;
    } else if ( AreaIndex == 3 ) {                          /* Center Left */
        unsigned short  Index;
        signed long     LineX[2];
        signed long     LineY[2];
        signed long     PointX;
        signed long     PointY;

        Index = YKnotStart*XKnotNum;

        LineX[0] = p_YAddressKnot[YKnotStart  ];
        LineX[1] = p_YAddressKnot[YKnotStart+1];            /* Next to LineX[0] */

        /* Calculate defocus value of each knot point */
        LineY[0] = calc_defocus_formula ( pfa_InputData, Index   );
        LineY[1] = calc_defocus_formula ( pfa_InputData, Index+XKnotNum );  /* Next to LineY[0] */

        PointX   = YAddressPDAFWindowCenter;

        /* Calculate coordination at the point of the line */
        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        Defocus = PointY;
    } else {                                                /* Center Right(5) */
        unsigned short  Index;
        signed long     LineX[2];
        signed long     LineY[2];
        signed long     PointX;
        signed long     PointY;

        Index = (YKnotStart+1)*XKnotNum-1;

        LineX[0] = p_YAddressKnot[YKnotStart  ];
        LineX[1] = p_YAddressKnot[YKnotStart+1];            /* Next to LineX[0] */

        /* Calculate defocus value of each knot point */
        LineY[0] = calc_defocus_formula ( pfa_InputData, Index   );
        LineY[1] = calc_defocus_formula ( pfa_InputData, Index+XKnotNum );  /* Next to LineY[0] */

        PointX   = YAddressPDAFWindowCenter;

        /* Calculate coordination at the point of the line */
        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        Defocus = PointY;
    }

    (*pfa_Defocus) = Defocus;

    return ;
}

/* Function for calculating defocus confidence level */
static void job_calc_defocus_confidence_level 
( 
    PdLibInputData_t *pfa_InputData,                        /* Input  : Input data structure */
    unsigned long *pfa_DefocusConfidenceLevel               /* Output : Defocus confidence level */
)
{
    signed long     DefocusOkNgThr;
    unsigned short  XKnotNum;
    unsigned short  YKnotNum;

    XKnotNum = (*pfa_InputData).XKnotNumDefocusOKNG;
    YKnotNum = (*pfa_InputData).YKnotNumDefocusOKNG;
    
    /* Disable of the judge function of confidence in each image area */
    if ( ( XKnotNum == 1 ) && ( YKnotNum == 1 ) ) {
        /* Disable compensation relation with image height. */
        DefocusOkNgThr = calc_defocus_ok_ng_thr ( pfa_InputData, 0);
    } else {
        unsigned short i;    
        unsigned short  *p_XAddressKnot;
        unsigned short  *p_YAddressKnot;
        unsigned short  XKnotStart;
        unsigned short  YKnotStart;
        unsigned char   AreaIndex;
        signed long     XAddressPDAFWindowCenter;
        signed long     YAddressPDAFWindowCenter;

        p_XAddressKnot = (*pfa_InputData).p_XAddressKnotDefocusOKNG;
        p_YAddressKnot = (*pfa_InputData).p_YAddressKnotDefocusOKNG;

        XAddressPDAFWindowCenter = ( (*pfa_InputData).XAddressOfWindowStart + 
                                     (*pfa_InputData).XAddressOfWindowEnd ) / 2;
        YAddressPDAFWindowCenter = ( (*pfa_InputData).YAddressOfWindowStart + 
                                     (*pfa_InputData).YAddressOfWindowEnd ) / 2;
        
        XKnotStart = 0;
        for ( i = 0; i < XKnotNum-1; i++ ) {                /* Check XKnotStart */
            if ( p_XAddressKnot[i] <= XAddressPDAFWindowCenter && 
                 XAddressPDAFWindowCenter <= p_XAddressKnot[i+1] ) {
                XKnotStart = i;
                break ;
            }
        }

        YKnotStart = 0;
        for ( i = 0; i < YKnotNum-1; i++ ) {                /* Check YKnotStar */
            if ( p_YAddressKnot[i] <= YAddressPDAFWindowCenter && 
                 YAddressPDAFWindowCenter <= p_YAddressKnot[i+1] ) {
                YKnotStart = i;
                break ;
            }
        }

    /*

        Divided by area. AreaIndex

        +---+---+---+
        | 0 | 1 | 2 |
        +---+---+---+
        | 3 | 4 | 5 |
        +---+---+---+
        | 6 | 7 | 8 |
        +---+---+---+

        Area 4 is surrounded by the knot points.
        Other area does not have knot points.

    */

        if ( YAddressPDAFWindowCenter < p_YAddressKnot[0] ) {
            /* Top */
                 if ( XAddressPDAFWindowCenter   < p_XAddressKnot[0]        ) {/* Left */   AreaIndex = 0;}
            else if ( p_XAddressKnot[XKnotNum-1] < XAddressPDAFWindowCenter ) {/* Right */  AreaIndex = 2;}
            else                                                              {/* Center */ AreaIndex = 1;}       
        } else if ( p_YAddressKnot[YKnotNum-1] < YAddressPDAFWindowCenter ) {
            /* Bottom */
                 if ( XAddressPDAFWindowCenter   < p_XAddressKnot[0]        ) {/* Left */   AreaIndex = 6;}
            else if ( p_XAddressKnot[XKnotNum-1] < XAddressPDAFWindowCenter ) {/* Right */  AreaIndex = 8;}
            else                                                              {/* Center */ AreaIndex = 7;}       
        } else {
            /* Center */
                 if ( XAddressPDAFWindowCenter   < p_XAddressKnot[0]        ) {/* Left */   AreaIndex = 3;}
            else if ( p_XAddressKnot[XKnotNum-1] < XAddressPDAFWindowCenter ) {/* Right */  AreaIndex = 5;}
            else                                                              {/* Center */ AreaIndex = 4;}
        }

        if ( AreaIndex == 4 ) {                                             /* Center */
            unsigned short  Index;
            signed long     LineX[2];
            signed long     LineY[2];
            signed long     PlaneZ[4];
            signed long     PointX;
            signed long     PointY;
            signed long     PointZ = 0;
            
            Index = YKnotStart*XKnotNum+XKnotStart;         /* Obtain index of knot point */

            LineX[0]  = p_XAddressKnot[XKnotStart  ];
            LineX[1]  = p_XAddressKnot[XKnotStart+1];       /* Next to LineX[0] */
            LineY[0]  = p_YAddressKnot[YKnotStart  ];
            LineY[1]  = p_YAddressKnot[YKnotStart+1];       /* Next to LineY[0] */
            
            /* Calculate threshold of confidence of each knot point */
            PlaneZ[0] = calc_defocus_ok_ng_thr ( pfa_InputData, Index            );
            PlaneZ[1] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+1          );
            PlaneZ[2] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+XKnotNum   );
            PlaneZ[3] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+XKnotNum+1 );

            PointX    = XAddressPDAFWindowCenter;
            PointY    = YAddressPDAFWindowCenter;

            /* Calculate coordination at the point of the plane */
            CalcAddressOnPlane_slXslYslZ (LineX, LineY, PlaneZ, PointX, PointY, &PointZ);

            DefocusOkNgThr = PointZ;
        } else if ( AreaIndex == 0 || AreaIndex == 2 || AreaIndex == 6 || AreaIndex == 8 ) {    /* Cornar of area */
            unsigned short Index;
            
                 if ( AreaIndex == 2 ) { Index = XKnotNum-1; }
            else if ( AreaIndex == 6 ) { Index = (YKnotNum-1)*XKnotNum; }
            else if ( AreaIndex == 8 ) { Index = YKnotNum*XKnotNum-1; }
            else                       { Index = 0; }
            DefocusOkNgThr = calc_defocus_ok_ng_thr ( pfa_InputData, Index );   /* Calculate threshold of confidence */               
        } else if ( AreaIndex == 1 ) {                      /* Top Center */
            unsigned short  Index;
            signed long     LineX[2];
            signed long     LineY[2];
            signed long     PointX;
            signed long     PointY;

            Index = XKnotStart;

            LineX[0] = p_XAddressKnot[XKnotStart  ];
            LineX[1] = p_XAddressKnot[XKnotStart+1];        /* Next to LineX[0] */

            /* Calculate threshold of confidence of each knot point */            
            LineY[0] = calc_defocus_ok_ng_thr ( pfa_InputData, Index   );
            LineY[1] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+1 );   /* Next to LineY[0] */

            PointX   = XAddressPDAFWindowCenter;

            /* Calculate coordination at the point of the line */
            CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

            DefocusOkNgThr = PointY;
        } else if ( AreaIndex == 7 ) {                      /* Bottom Center */
            unsigned short  Index;
            signed long     LineX[2];
            signed long     LineY[2];
            signed long     PointX;
            signed long     PointY;

            Index = (YKnotNum-1)*XKnotNum + XKnotStart;

            LineX[0] = p_XAddressKnot[XKnotStart  ];
            LineX[1] = p_XAddressKnot[XKnotStart+1];        /* Next to LineX[0] */

            /* Calculate threshold of confidence of each knot point */
            LineY[0] = calc_defocus_ok_ng_thr ( pfa_InputData, Index   );
            LineY[1] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+1 );   /* Next to LineY[0] */

            PointX   = XAddressPDAFWindowCenter;

            /* Calculate coordination at the point of the line */
            CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

            DefocusOkNgThr = PointY;
        } else if ( AreaIndex == 3 ) {                      /* Center Left */
            unsigned short  Index;
            signed long     LineX[2];
            signed long     LineY[2];
            signed long     PointX;
            signed long     PointY;

            Index = YKnotStart*XKnotNum;

            LineX[0] = p_YAddressKnot[YKnotStart  ];
            LineX[1] = p_YAddressKnot[YKnotStart+1];        /* Next to LineX[0] */

            /* Calculate threshold of confidence of each knot point */
            LineY[0] = calc_defocus_ok_ng_thr ( pfa_InputData, Index   );
            LineY[1] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+XKnotNum );    /* Next to LineY[0] */

            PointX   = YAddressPDAFWindowCenter;

            /* Calculate coordination at the point of the line */
            CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

            DefocusOkNgThr = PointY;
        } else {                                            /* Center Right(5) */
            unsigned short  Index;
            signed long     LineX[2];
            signed long     LineY[2];
            signed long     PointX;
            signed long     PointY;

            Index = (YKnotStart+1)*XKnotNum-1;

            LineX[0] = p_YAddressKnot[YKnotStart  ];
            LineX[1] = p_YAddressKnot[YKnotStart+1];        /* Next to LineX[0] */

            /* Calculate threshold of confidence of each knot point */
            LineY[0] = calc_defocus_ok_ng_thr ( pfa_InputData, Index   );
            LineY[1] = calc_defocus_ok_ng_thr ( pfa_InputData, Index+XKnotNum );

            PointX   = YAddressPDAFWindowCenter;

            /* Calculate coordination at the point of the line */
            CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

            DefocusOkNgThr = PointY;
        }
    }

    if ( DefocusOkNgThr <= 0 ) DefocusOkNgThr = 0;          /* Check DefocusOkNgThr */

    
    if ( DefocusOkNgThr == 0 ) {                            /* If DefocusOkNgThr is Zero */
        (*pfa_DefocusConfidenceLevel) = 1024;               /* Set max value to ConfidenceLevel */
    } else {
        double ConfidenceLevel;
        double DensityOfPhasePix;
        double DefocusConfidenceLevel;

        ConfidenceLevel   = (double)((*pfa_InputData).ConfidenceLevel);
        
        if ( (*pfa_InputData).DensityOfPhasePix == 0 ) {    /* If DensityOfPhasePix is not set */
            DensityOfPhasePix = 2304.0;                     /* Set default value to DensityOfPhasePix */
        } else {
            DensityOfPhasePix = (double)((*pfa_InputData).DensityOfPhasePix);
        }

        /* Calculate defocus confidence level */
        DefocusConfidenceLevel = 1024.0 * ConfidenceLevel * 2304.0 / DensityOfPhasePix / (double)DefocusOkNgThr;

        (*pfa_DefocusConfidenceLevel) = limit_defocus_confidence_level(DefocusConfidenceLevel);
    }

    return ;
}

/* Function for calculating defocus confidence */
static void job_calc_defocus_confidence 
( 
    unsigned long fa_DefocusConfidenceLevel,                /* Input  : Defocus confidence level */
    signed char *pfa_DefocusConfidence                      /* Output : Defocus confidence */
)
{
    if ( 1024 <= fa_DefocusConfidenceLevel ) {
        (*pfa_DefocusConfidence) = D_PD_LIB_E_OK;
    } else {
        /* Check the value of input */
        (*pfa_DefocusConfidence) = -ELDCL;                  /* Low DefocusConfidenceLevel */
    }

    return ;
}

/* Function for calculating phase difference */
static void job_calc_phase_difference 
( 
    PdLibInputData_t *pfa_InputData,                        /* Input : Input data structure */
    signed long *pfa_PhaseDifference                        /* Output : Phase difference */
)
{
    (*pfa_PhaseDifference) = (*pfa_InputData).PhaseDifference;
    return ;
}

/* Sub function of job_calc_defocus() */
/* Function for calculating defocus value which uses slope and offset of index point */
static signed long calc_defocus_formula 
( 
    PdLibInputData_t *pfa_InputData,                        /* Input : Input data structure */
    unsigned short fa_Index                                 /* Input : Index of knot point  */
)
{
    signed long PhaseDifference;
    signed long AdjCoeffSlope;
    double Z;

    PhaseDifference = (*pfa_InputData).PhaseDifference;
    AdjCoeffSlope   = (*pfa_InputData).AdjCoeffSlope;

    Z = (double)AdjCoeffSlope * (double)((*pfa_InputData).p_SlopeData[fa_Index]) * (double)PhaseDifference / 2304.0 + (double)((*pfa_InputData).p_OffsetData[fa_Index]);

    return limit_defocus_formula(Z);                        /* Return defocus value with limitation */
}

/* Sub function of calc_defocus_formula() */
/* Function for Limiting defocus value as  0x80000000 - 0x7FFFFFFF */
static signed long limit_defocus_formula 
( 
    double fa_Defocus                                       /* Input : Defocus */
)
{
    signed long ret;

    /*  0x80000000 - 0x7FFFFFFF */
    /* -2147483648 - +2147483647 */
    /* -2147483647 - +2147483646 */

    if ( fa_Defocus <= -2147483647.0 ) {
        ret = -2147483647;                                  /* Limit min */
    } else  if ( +2147483646.0 <= fa_Defocus ) {
        ret = 2147483646;                                   /* Limit max */
    } else {
        ret = (signed long)fa_Defocus;
    }

    return ret;                                             /* Return limited value */
}

/* Sub function of job_calc_defocus_confidence_level() */
/* Function for calculating threshold of confidence */
static signed long calc_defocus_ok_ng_thr 
( 
    PdLibInputData_t *pfa_InputData,                        /* Input : Input data structure */
    unsigned short fa_Index                                 /* Input : Index of knot point  */
)
{

    unsigned long *LineX;
    unsigned long *LineY;
    unsigned long PointNum;
    unsigned long PointX;
    unsigned long PointY = 0;

    LineX    = (*pfa_InputData).p_DefocusOKNGThrLine[fa_Index].p_AnalogGain;
    LineY    = (*pfa_InputData).p_DefocusOKNGThrLine[fa_Index].p_Confidence;
    PointNum = (*pfa_InputData).p_DefocusOKNGThrLine[fa_Index].PointNum;
    PointX   = (*pfa_InputData).ImagerAnalogGain;

    /* Calculate coordination at the point of the broken line */
    CalcAddressOnBrokenLine_ulXulY ( LineX, LineY, PointNum, PointX, &PointY );

    return (signed long)PointY;                             /* return threshold of confidence */
}

/* Sub function of job_calc_defocus_confidence_level() */
/* Function for Limiting defocus confidence level */
static unsigned long limit_defocus_confidence_level 
( 
    double fa_DefocusConfidenceLevel                        /* Input : Defocus confidence level */
)
{
    unsigned long ret;

    /*  0x00000000 - 0xFFFFFFFF */
    /*           0 - 4294967295 */
    /*           0 - 4294967294 */

    if ( fa_DefocusConfidenceLevel <= 0.0 ) {               /* limit min */
        ret = 0;
    } else if ( +4294967294.0 <= fa_DefocusConfidenceLevel ) {  /* limit max */
        /* ret = 4294967294(0xFFFFFFFE) */
        ret = 0xFFFFFFFE;
    } else {
        ret = (unsigned long)fa_DefocusConfidenceLevel;
    }

    return ret;                                             /* Return defocus confidence level */
}
