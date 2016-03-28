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

/****************************************************************/
/*                      external function                       */
/****************************************************************/

/* Function for calculating coordination at the point of the line */
extern void CalcAddressOnLine_slXslY
(
    /* Input */
    signed long *pf_x,
    signed long *pf_y,
    signed long f_xx,
    /* Output */
    signed long *fp_yy
)
{
    signed long y;

         if ( pf_x[0] == pf_x[1] && pf_y[0] == pf_y[1] ) { y = pf_y[0]; }
    else if ( pf_x[0] != pf_x[1] && pf_y[0] == pf_y[1] ) { y = pf_y[0]; }
    else if ( pf_x[0] == pf_x[1] ) { y = (pf_y[0]+pf_y[1])/2; }
    else {
        /* Equation of the line passing through (x0, y0) and (x1, y1). */ 
        /* y = y0 + (y1 - y0) * (x - x0) / (x1 - x0) */
        
        signed long y0;
        signed long y1;
        signed long x0;
        signed long x1;
        signed long x;

        x = f_xx;

        if ( pf_x[0] <= pf_x[1] ) {
            x0 = pf_x[0];
            x1 = pf_x[1];
            y0 = pf_y[0];
            y1 = pf_y[1];
        } else {
            x0 = pf_x[1];
            x1 = pf_x[0];
            y0 = pf_y[1];
            y1 = pf_y[0];
        }

        if ( x < x0 ) {
            y = y0;
        } else if ( x1 < x ) {
            y = y1;
        } else {
            double yy;
            /* y = y0 + (y1 - y0) * (x - x0) / (x1 - x0) */
            yy  = (double)y0 
                + ((double)y1 - (double)y0)
                * ((double) x - (double)x0) 
                / ((double)x1 - (double)x0);
            y = (signed long)yy;
        }
        
    }

    (*fp_yy) = y;

    return ;
}

/* Function for calculating coordination at the point of the broken line */
extern signed char CalcAddressOnBrokenLine_ulXulY
(
    /* Input */
    unsigned long *pf_x,
    unsigned long *pf_y,
    unsigned long f_PointNum,
    unsigned long f_xx,
    /* Output */
    unsigned long *pf_yy
)
{
    unsigned long i;
    unsigned long y;

    if ( 2 <= f_PointNum ) {
    } else {
        return D_MATH_FUNC_NG;
    }

    /*
        *pf_x
        *pf_y
        f_xx
        The range needs equal or less than 0x7FFFFFFF.
        In the case of out of bounds, return D_MATH_FUNC_NG.
    */
    
    if( pf_x[f_PointNum-1] <= 0x7FFFFFFF ) {
    } else {
        return D_MATH_FUNC_NG;
    }
    
    if( f_xx <= 0x7FFFFFFF ) {
    } else {
        return D_MATH_FUNC_NG;
    }
    
    for( i=0; i < f_PointNum; i++ ) {
        if( pf_y[i] <= 0x7FFFFFFF ) {
            
        } else {
            return D_MATH_FUNC_NG;
        }
    }
    
    for ( i = 0; i < f_PointNum-1; i++ ) {
        if( pf_x[i] <= pf_x[i+1] ) {
        } else {
            return D_MATH_FUNC_NG;
        }
    }

    if ( f_xx < pf_x[0] ) {
        y = pf_y[0];
    } else if ( pf_x[f_PointNum-1] < f_xx ) {
        y = pf_y[f_PointNum-1];
    } else {
        y = 0;
        
        for ( i = 0; i < f_PointNum-1; i++ ) {
            if( pf_x[i] <= f_xx && f_xx <= pf_x[i+1] ) {
                
                signed long LineX[2];
                signed long LineY[2];
                signed long PointX;
                signed long PointY;

                LineX[0] = (signed long)(pf_x[i]);
                LineX[1] = (signed long)(pf_x[i+1]);
                LineY[0] = (signed long)(pf_y[i]);
                LineY[1] = (signed long)(pf_y[i+1]);
                PointX   = (signed long)(f_xx);

                CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

                if ( PointY <= 0 ) {
                    PointY = 0;
                }

                y = (unsigned long)PointY;

                break ;
                
            }
        }
    }

    (*pf_yy) = y;

    return D_MATH_FUNC_OK;
}

/* Function for calculating coordination at the point of the plane */
extern signed char CalcAddressOnPlane_slXslYslZ
(
    /* Input */
    signed long *pf_x,
    signed long *pf_y,
    signed long *pf_z,
    signed long f_xx,
    signed long f_yy,
    /* Output */
    signed long *pf_zz
)
{
    if ( pf_x[0] <= f_xx && f_xx <= pf_x[1] &&  
         pf_y[0] <= f_yy && f_yy <= pf_y[1] &&  
         pf_x[0] < pf_x[1] &&  pf_y[0] < pf_y[1] ) {
    } else {
        return D_MATH_FUNC_NG;
    }

    {
        signed long z1;
        signed long z2;
        signed long z;

        signed long LineX[2];
        signed long LineY[2];
        signed long PointX;
        signed long PointY;

        LineX[0] = pf_x[0];
        LineX[1] = pf_x[1];
        LineY[0] = pf_z[0];
        LineY[1] = pf_z[1];
        PointX   = f_xx;

        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        z1 = PointY;

        LineY[0] = pf_z[2];
        LineY[1] = pf_z[3];

        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        z2 = PointY;

        LineX[0] = pf_y[0];
        LineX[1] = pf_y[1];
        LineY[0] = z1;
        LineY[1] = z2;
        PointX   = f_yy;

        CalcAddressOnLine_slXslY ( LineX, LineY, PointX, &PointY );

        z = PointY;

        (*pf_zz) = z;
    }

    return D_MATH_FUNC_OK;
}
