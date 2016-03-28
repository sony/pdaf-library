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

#ifndef __PDAF_MATH_FUNC_H__
#define __PDAF_MATH_FUNC_H__

#define D_MATH_FUNC_NG  (-1)
#define D_MATH_FUNC_OK  (0)

/* Function for calculating coordination at the point of the line */
#if defined __GNUC__
__attribute__ ((visibility ("hidden"))) extern void CalcAddressOnLine_slXslY
#else
extern void CalcAddressOnLine_slXslY
#endif
(
    /* Input */
    signed long *pf_x,
    signed long *pf_y,
    signed long f_xx,
    /* Output */
    signed long *fp_yy
);

/* Function for calculating coordination at the point of the broken line */
#if defined __GNUC__
__attribute__ ((visibility ("hidden"))) extern signed char CalcAddressOnBrokenLine_ulXulY
#else
extern signed char CalcAddressOnBrokenLine_ulXulY
#endif
(
    /* Input */
    unsigned long *pf_x,
    unsigned long *pf_y,
    unsigned long f_PointNum,
    unsigned long f_xx,
    /* Output */
    unsigned long *pf_yy
);

/* Function for calculating coordination at the point of the plane */
#if defined __GNUC__
__attribute__ ((visibility ("hidden"))) extern signed char CalcAddressOnPlane_slXslYslZ
#else
extern signed char CalcAddressOnPlane_slXslYslZ
#endif
(
    /* Input */
    signed long *pf_x,
    signed long *pf_y,
    signed long *pf_z,
    signed long f_xx,
    signed long f_yy,
    /* Output */
    signed long *pf_zz
);

#endif
