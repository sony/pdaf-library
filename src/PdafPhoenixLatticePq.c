/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * PdafPhoenixLatticePq.c
 * Zero-Float `p/q` ("Counting Divided") Defocus & Laser Phoenix Lattice
 * Phase-Locked Autofocus Extension for Sony `pdaf-library`.
 *
 * Authored by 8b-is (https://8b.is) — Visiting Sony R&D in Tokyo
 * (through October 24th; WhatsApp: +1 757-614-7542 — Chris).
 */

#include <stddef.h>
#include <stdint.h>
#include "PdafPhoenixLatticePq.h"

/* Internal helper declarations */
static signed long check_input_pq(PdLibInputData_t *pfa_InputData);
static void calc_defocus_pq(PdLibInputData_t *pfa_InputData, signed long *pfa_Defocus);
static void calc_defocus_confidence_level_pq(PdLibInputData_t *pfa_InputData, unsigned long *pfa_DefocusConfidenceLevel);
static signed long calc_defocus_formula_pq(PdLibInputData_t *pfa_InputData, unsigned short fa_Index);
static signed long calc_defocus_ok_ng_thr_pq(PdLibInputData_t *pfa_InputData, unsigned short fa_Index);
static signed long limit_defocus_i64(int64_t fa_Defocus);
static unsigned long limit_confidence_u64(uint64_t fa_Confidence);

/* Zero-float exact 64-bit integer `p/q` ("Counting Divided") line interpolation */
extern void CalcAddressOnLinePq_slXslY
(
    signed long *pf_x,
    signed long *pf_y,
    signed long f_xx,
    signed long *fp_yy
)
{
    signed long y;

    if (pf_x[0] == pf_x[1] && pf_y[0] == pf_y[1]) {
        y = pf_y[0];
    } else if (pf_x[0] != pf_x[1] && pf_y[0] == pf_y[1]) {
        y = pf_y[0];
    } else if (pf_x[0] == pf_x[1]) {
        y = (signed long)(((int64_t)pf_y[0] + (int64_t)pf_y[1]) / 2);
    } else {
        int64_t x0, x1, y0, y1, x;

        x = (int64_t)f_xx;
        if (pf_x[0] <= pf_x[1]) {
            x0 = (int64_t)pf_x[0];
            x1 = (int64_t)pf_x[1];
            y0 = (int64_t)pf_y[0];
            y1 = (int64_t)pf_y[1];
        } else {
            x0 = (int64_t)pf_x[1];
            x1 = (int64_t)pf_x[0];
            y0 = (int64_t)pf_y[1];
            y1 = (int64_t)pf_y[0];
        }

        if (x < x0) {
            y = (signed long)y0;
        } else if (x1 < x) {
            y = (signed long)y1;
        } else {
            /*
             * Exact rational `p/q` ("Counting Divided") evaluation over common denominator:
             *   y = (y0 * (x1 - x0) + (y1 - y0) * (x - x0)) / (x1 - x0)
             */
            int64_t dx  = x1 - x0;
            int64_t num = y0 * dx + (y1 - y0) * (x - x0);
            y = (signed long)(num / dx);
        }
    }

    (*fp_yy) = y;
}

/* Zero-float exact 64-bit integer `p/q` broken-line interpolation */
extern signed char CalcAddressOnBrokenLinePq_ulXulY
(
    unsigned long *pf_x,
    unsigned long *pf_y,
    unsigned long f_PointNum,
    unsigned long f_xx,
    unsigned long *pf_yy
)
{
    unsigned long i;
    unsigned long y;

    if (f_PointNum < 2UL) {
        return D_MATH_FUNC_NG;
    }
    if (pf_x[f_PointNum - 1UL] > 0x7FFFFFFFUL || f_xx > 0x7FFFFFFFUL) {
        return D_MATH_FUNC_NG;
    }
    for (i = 0UL; i < f_PointNum; i++) {
        if (pf_y[i] > 0x7FFFFFFFUL) {
            return D_MATH_FUNC_NG;
        }
    }
    for (i = 0UL; i < f_PointNum - 1UL; i++) {
        if (pf_x[i] > pf_x[i + 1UL]) {
            return D_MATH_FUNC_NG;
        }
    }

    if (f_xx < pf_x[0]) {
        y = pf_y[0];
    } else if (pf_x[f_PointNum - 1UL] < f_xx) {
        y = pf_y[f_PointNum - 1UL];
    } else {
        y = 0UL;
        for (i = 0UL; i < f_PointNum - 1UL; i++) {
            if (pf_x[i] <= f_xx && f_xx <= pf_x[i + 1UL]) {
                signed long LineX[2];
                signed long LineY[2];
                signed long PointX;
                signed long PointY;

                LineX[0] = (signed long)pf_x[i];
                LineX[1] = (signed long)pf_x[i + 1UL];
                LineY[0] = (signed long)pf_y[i];
                LineY[1] = (signed long)pf_y[i + 1UL];
                PointX   = (signed long)f_xx;

                CalcAddressOnLinePq_slXslY(LineX, LineY, PointX, &PointY);
                if (PointY <= 0L) {
                    PointY = 0L;
                }
                y = (unsigned long)PointY;
                break;
            }
        }
    }

    (*pf_yy) = y;
    return D_MATH_FUNC_OK;
}

/* Zero-float exact 64-bit integer `p/q` bilinear plane interpolation */
extern signed char CalcAddressOnPlanePq_slXslYslZ
(
    signed long *pf_x,
    signed long *pf_y,
    signed long *pf_z,
    signed long f_xx,
    signed long f_yy,
    signed long *pf_zz
)
{
    signed long z1, z2, z;
    signed long LineX[2];
    signed long LineY[2];

    if (!(pf_x[0] <= f_xx && f_xx <= pf_x[1] &&
          pf_y[0] <= f_yy && f_yy <= pf_y[1] &&
          pf_x[0] < pf_x[1] && pf_y[0] < pf_y[1])) {
        return D_MATH_FUNC_NG;
    }

    LineX[0] = pf_x[0];
    LineX[1] = pf_x[1];
    LineY[0] = pf_z[0];
    LineY[1] = pf_z[1];
    CalcAddressOnLinePq_slXslY(LineX, LineY, f_xx, &z1);

    LineY[0] = pf_z[2];
    LineY[1] = pf_z[3];
    CalcAddressOnLinePq_slXslY(LineX, LineY, f_xx, &z2);

    LineX[0] = pf_y[0];
    LineX[1] = pf_y[1];
    LineY[0] = z1;
    LineY[1] = z2;
    CalcAddressOnLinePq_slXslY(LineX, LineY, f_yy, &z);

    (*pf_zz) = z;
    return D_MATH_FUNC_OK;
}

static signed long check_input_pq(PdLibInputData_t *pfa_InputData)
{
    unsigned short i;

    if (pfa_InputData->XSizeOfImage < 2U) {
        return -EINXSOI;
    }
    if (pfa_InputData->YSizeOfImage < 2U) {
        return -EINYSOI;
    }
    if (!((pfa_InputData->XAddressOfWindowStart <= (pfa_InputData->XAddressOfWindowEnd - 1)) &&
          (pfa_InputData->XAddressOfWindowEnd <= (pfa_InputData->XSizeOfImage - 1)))) {
        return -EINPDAFWX;
    }
    if (!((pfa_InputData->YAddressOfWindowStart <= (pfa_InputData->YAddressOfWindowEnd - 1)) &&
          (pfa_InputData->YAddressOfWindowEnd <= (pfa_InputData->YSizeOfImage - 1)))) {
        return -EINPDAFWY;
    }
    if (pfa_InputData->XKnotNumSlopeOffset < 2U || pfa_InputData->YKnotNumSlopeOffset < 2U) {
        return -EINSO;
    }
    if (!((pfa_InputData->AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE0) ||
          (pfa_InputData->AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE1) ||
          (pfa_InputData->AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE2) ||
          (pfa_InputData->AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE3) ||
          (pfa_InputData->AdjCoeffSlope == D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE4))) {
        return -EINACS;
    }
    for (i = 0U; i < (unsigned short)(pfa_InputData->XKnotNumSlopeOffset - 1U); i++) {
        if (pfa_InputData->p_XAddressKnotSlopeOffset[i] >= pfa_InputData->p_XAddressKnotSlopeOffset[i + 1U]) {
            return -EINSOXAK;
        }
    }
    for (i = 0U; i < (unsigned short)(pfa_InputData->YKnotNumSlopeOffset - 1U); i++) {
        if (pfa_InputData->p_YAddressKnotSlopeOffset[i] >= pfa_InputData->p_YAddressKnotSlopeOffset[i + 1U]) {
            return -EINSOYAK;
        }
    }
    if (pfa_InputData->XKnotNumDefocusOKNG == 0U || pfa_InputData->YKnotNumDefocusOKNG == 0U) {
        if (!(pfa_InputData->XKnotNumDefocusOKNG == 0U && pfa_InputData->YKnotNumDefocusOKNG == 0U)) {
            return -EINVALDISCONFJ;
        }
    } else if (pfa_InputData->XKnotNumDefocusOKNG == 1U || pfa_InputData->YKnotNumDefocusOKNG == 1U) {
        if (!(pfa_InputData->XKnotNumDefocusOKNG == 1U && pfa_InputData->YKnotNumDefocusOKNG == 1U)) {
            return -EINVALDISIHC;
        }
    }
    if (pfa_InputData->p_DefocusOKNGThrLine->PointNum < 2UL) {
        return -EINDONTPN;
    }
    if (pfa_InputData->XKnotNumDefocusOKNG >= 2U) {
        for (i = 0U; i < (unsigned short)(pfa_InputData->XKnotNumDefocusOKNG - 1U); i++) {
            if (pfa_InputData->p_XAddressKnotDefocusOKNG[i] >= pfa_InputData->p_XAddressKnotDefocusOKNG[i + 1U]) {
                return -EINDONXAK;
            }
        }
    }
    if (pfa_InputData->YKnotNumDefocusOKNG >= 2U) {
        for (i = 0U; i < (unsigned short)(pfa_InputData->YKnotNumDefocusOKNG - 1U); i++) {
            if (pfa_InputData->p_YAddressKnotDefocusOKNG[i] >= pfa_InputData->p_YAddressKnotDefocusOKNG[i + 1U]) {
                return -EINDONYAK;
            }
        }
    }
    if (!((pfa_InputData->DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE0) ||
          (pfa_InputData->DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE1) ||
          (pfa_InputData->DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE2) ||
          (pfa_InputData->DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE3) ||
          (pfa_InputData->DensityOfPhasePix == D_PD_LIB_DENSITY_SENS_MODE4))) {
        return -EINDOP;
    }

    return D_PD_LIB_E_OK;
}

static signed long limit_defocus_i64(int64_t fa_Defocus)
{
    if (fa_Defocus <= -2147483647LL) {
        return -2147483647L;
    }
    if (fa_Defocus >= 2147483646LL) {
        return 2147483646L;
    }
    return (signed long)fa_Defocus;
}

static unsigned long limit_confidence_u64(uint64_t fa_Confidence)
{
    if (fa_Confidence >= 0xFFFFFFFEULL) {
        return 0xFFFFFFFEUL;
    }
    return (unsigned long)fa_Confidence;
}

static signed long calc_defocus_formula_pq(PdLibInputData_t *pfa_InputData, unsigned short fa_Index)
{
    int64_t phase_diff = (int64_t)pfa_InputData->PhaseDifference;
    int64_t adj_slope  = (int64_t)pfa_InputData->AdjCoeffSlope;
    int64_t slope      = (int64_t)pfa_InputData->p_SlopeData[fa_Index];
    int64_t offset     = (int64_t)pfa_InputData->p_OffsetData[fa_Index];

    /*
     * Exact rational `p/q` ("Counting Divided"):
     *   Z = (AdjCoeffSlope * Slope * PhaseDifference + 2304 * Offset) / 2304
     */
    int64_t num = adj_slope * slope * phase_diff + 2304LL * offset;
    int64_t z   = num / 2304LL;

    return limit_defocus_i64(z);
}

static signed long calc_defocus_ok_ng_thr_pq(PdLibInputData_t *pfa_InputData, unsigned short fa_Index)
{
    unsigned long *LineX   = pfa_InputData->p_DefocusOKNGThrLine[fa_Index].p_AnalogGain;
    unsigned long *LineY   = pfa_InputData->p_DefocusOKNGThrLine[fa_Index].p_Confidence;
    unsigned long PointNum = pfa_InputData->p_DefocusOKNGThrLine[fa_Index].PointNum;
    unsigned long PointX   = pfa_InputData->ImagerAnalogGain;
    unsigned long PointY   = 0UL;

    CalcAddressOnBrokenLinePq_ulXulY(LineX, LineY, PointNum, PointX, &PointY);
    return (signed long)PointY;
}

static void calc_defocus_pq(PdLibInputData_t *pfa_InputData, signed long *pfa_Defocus)
{
    unsigned short i;
    unsigned short XKnotNum      = pfa_InputData->XKnotNumSlopeOffset;
    unsigned short YKnotNum      = pfa_InputData->YKnotNumSlopeOffset;
    unsigned short *p_XAddrKnot  = pfa_InputData->p_XAddressKnotSlopeOffset;
    unsigned short *p_YAddrKnot  = pfa_InputData->p_YAddressKnotSlopeOffset;
    unsigned short XKnotStart    = 0U;
    unsigned short YKnotStart    = 0U;
    unsigned char  AreaIndex;
    signed long    Defocus;
    signed long    XCenter       = (pfa_InputData->XAddressOfWindowStart + pfa_InputData->XAddressOfWindowEnd) / 2;
    signed long    YCenter       = (pfa_InputData->YAddressOfWindowStart + pfa_InputData->YAddressOfWindowEnd) / 2;

    for (i = 0U; i < (unsigned short)(XKnotNum - 1U); i++) {
        if ((signed long)p_XAddrKnot[i] <= XCenter && XCenter <= (signed long)p_XAddrKnot[i + 1U]) {
            XKnotStart = i;
            break;
        }
    }
    for (i = 0U; i < (unsigned short)(YKnotNum - 1U); i++) {
        if ((signed long)p_YAddrKnot[i] <= YCenter && YCenter <= (signed long)p_YAddrKnot[i + 1U]) {
            YKnotStart = i;
            break;
        }
    }

    if (YCenter < (signed long)p_YAddrKnot[0]) {
        if (XCenter < (signed long)p_XAddrKnot[0])                   { AreaIndex = 0U; }
        else if ((signed long)p_XAddrKnot[XKnotNum - 1U] < XCenter)  { AreaIndex = 2U; }
        else                                                         { AreaIndex = 1U; }
    } else if ((signed long)p_YAddrKnot[YKnotNum - 1U] < YCenter) {
        if (XCenter < (signed long)p_XAddrKnot[0])                   { AreaIndex = 6U; }
        else if ((signed long)p_XAddrKnot[XKnotNum - 1U] < XCenter)  { AreaIndex = 8U; }
        else                                                         { AreaIndex = 7U; }
    } else {
        if (XCenter < (signed long)p_XAddrKnot[0])                   { AreaIndex = 3U; }
        else if ((signed long)p_XAddrKnot[XKnotNum - 1U] < XCenter)  { AreaIndex = 5U; }
        else                                                         { AreaIndex = 4U; }
    }

    if (AreaIndex == 4U) {
        unsigned short Index = (unsigned short)(YKnotStart * XKnotNum + XKnotStart);
        signed long LineX[2], LineY[2], PlaneZ[4], PointZ = 0L;

        LineX[0]  = (signed long)p_XAddrKnot[XKnotStart];
        LineX[1]  = (signed long)p_XAddrKnot[XKnotStart + 1U];
        LineY[0]  = (signed long)p_YAddrKnot[YKnotStart];
        LineY[1]  = (signed long)p_YAddrKnot[YKnotStart + 1U];

        PlaneZ[0] = calc_defocus_formula_pq(pfa_InputData, Index);
        PlaneZ[1] = calc_defocus_formula_pq(pfa_InputData, (unsigned short)(Index + 1U));
        PlaneZ[2] = calc_defocus_formula_pq(pfa_InputData, (unsigned short)(Index + XKnotNum));
        PlaneZ[3] = calc_defocus_formula_pq(pfa_InputData, (unsigned short)(Index + XKnotNum + 1U));

        CalcAddressOnPlanePq_slXslYslZ(LineX, LineY, PlaneZ, XCenter, YCenter, &PointZ);
        Defocus = PointZ;
    } else if (AreaIndex == 0U || AreaIndex == 2U || AreaIndex == 6U || AreaIndex == 8U) {
        unsigned short Index;
        if (AreaIndex == 2U)      { Index = (unsigned short)(XKnotNum - 1U); }
        else if (AreaIndex == 6U) { Index = (unsigned short)((YKnotNum - 1U) * XKnotNum); }
        else if (AreaIndex == 8U) { Index = (unsigned short)(YKnotNum * XKnotNum - 1U); }
        else                      { Index = 0U; }
        Defocus = calc_defocus_formula_pq(pfa_InputData, Index);
    } else if (AreaIndex == 1U || AreaIndex == 7U) {
        unsigned short Index = (AreaIndex == 1U)
            ? XKnotStart
            : (unsigned short)((YKnotNum - 1U) * XKnotNum + XKnotStart);
        signed long LineX[2], LineY[2], PointY = 0L;

        LineX[0] = (signed long)p_XAddrKnot[XKnotStart];
        LineX[1] = (signed long)p_XAddrKnot[XKnotStart + 1U];
        LineY[0] = calc_defocus_formula_pq(pfa_InputData, Index);
        LineY[1] = calc_defocus_formula_pq(pfa_InputData, (unsigned short)(Index + 1U));

        CalcAddressOnLinePq_slXslY(LineX, LineY, XCenter, &PointY);
        Defocus = PointY;
    } else {
        unsigned short Index = (AreaIndex == 3U)
            ? (unsigned short)(YKnotStart * XKnotNum)
            : (unsigned short)((YKnotStart + 1U) * XKnotNum - 1U);
        signed long LineX[2], LineY[2], PointY = 0L;

        LineX[0] = (signed long)p_YAddrKnot[YKnotStart];
        LineX[1] = (signed long)p_YAddrKnot[YKnotStart + 1U];
        LineY[0] = calc_defocus_formula_pq(pfa_InputData, Index);
        LineY[1] = calc_defocus_formula_pq(pfa_InputData, (unsigned short)(Index + XKnotNum));

        CalcAddressOnLinePq_slXslY(LineX, LineY, YCenter, &PointY);
        Defocus = PointY;
    }

    (*pfa_Defocus) = Defocus;
}

static void calc_defocus_confidence_level_pq
(
    PdLibInputData_t *pfa_InputData,
    unsigned long *pfa_DefocusConfidenceLevel
)
{
    signed long    DefocusOkNgThr;
    unsigned short XKnotNum = pfa_InputData->XKnotNumDefocusOKNG;
    unsigned short YKnotNum = pfa_InputData->YKnotNumDefocusOKNG;

    if (XKnotNum == 1U && YKnotNum == 1U) {
        DefocusOkNgThr = calc_defocus_ok_ng_thr_pq(pfa_InputData, 0U);
    } else {
        unsigned short i;
        unsigned short *p_XAddrKnot = pfa_InputData->p_XAddressKnotDefocusOKNG;
        unsigned short *p_YAddrKnot = pfa_InputData->p_YAddressKnotDefocusOKNG;
        unsigned short XKnotStart   = 0U;
        unsigned short YKnotStart   = 0U;
        unsigned char  AreaIndex;
        signed long    XCenter      = (pfa_InputData->XAddressOfWindowStart + pfa_InputData->XAddressOfWindowEnd) / 2;
        signed long    YCenter      = (pfa_InputData->YAddressOfWindowStart + pfa_InputData->YAddressOfWindowEnd) / 2;

        for (i = 0U; i < (unsigned short)(XKnotNum - 1U); i++) {
            if ((signed long)p_XAddrKnot[i] <= XCenter && XCenter <= (signed long)p_XAddrKnot[i + 1U]) {
                XKnotStart = i;
                break;
            }
        }
        for (i = 0U; i < (unsigned short)(YKnotNum - 1U); i++) {
            if ((signed long)p_YAddrKnot[i] <= YCenter && YCenter <= (signed long)p_YAddrKnot[i + 1U]) {
                YKnotStart = i;
                break;
            }
        }

        if (YCenter < (signed long)p_YAddrKnot[0]) {
            if (XCenter < (signed long)p_XAddrKnot[0])                   { AreaIndex = 0U; }
            else if ((signed long)p_XAddrKnot[XKnotNum - 1U] < XCenter)  { AreaIndex = 2U; }
            else                                                         { AreaIndex = 1U; }
        } else if ((signed long)p_YAddrKnot[YKnotNum - 1U] < YCenter) {
            if (XCenter < (signed long)p_XAddrKnot[0])                   { AreaIndex = 6U; }
            else if ((signed long)p_XAddrKnot[XKnotNum - 1U] < XCenter)  { AreaIndex = 8U; }
            else                                                         { AreaIndex = 7U; }
        } else {
            if (XCenter < (signed long)p_XAddrKnot[0])                   { AreaIndex = 3U; }
            else if ((signed long)p_XAddrKnot[XKnotNum - 1U] < XCenter)  { AreaIndex = 5U; }
            else                                                         { AreaIndex = 4U; }
        }

        if (AreaIndex == 4U) {
            unsigned short Index = (unsigned short)(YKnotStart * XKnotNum + XKnotStart);
            signed long LineX[2], LineY[2], PlaneZ[4], PointZ = 0L;

            LineX[0]  = (signed long)p_XAddrKnot[XKnotStart];
            LineX[1]  = (signed long)p_XAddrKnot[XKnotStart + 1U];
            LineY[0]  = (signed long)p_YAddrKnot[YKnotStart];
            LineY[1]  = (signed long)p_YAddrKnot[YKnotStart + 1U];

            PlaneZ[0] = calc_defocus_ok_ng_thr_pq(pfa_InputData, Index);
            PlaneZ[1] = calc_defocus_ok_ng_thr_pq(pfa_InputData, (unsigned short)(Index + 1U));
            PlaneZ[2] = calc_defocus_ok_ng_thr_pq(pfa_InputData, (unsigned short)(Index + XKnotNum));
            PlaneZ[3] = calc_defocus_ok_ng_thr_pq(pfa_InputData, (unsigned short)(Index + XKnotNum + 1U));

            CalcAddressOnPlanePq_slXslYslZ(LineX, LineY, PlaneZ, XCenter, YCenter, &PointZ);
            DefocusOkNgThr = PointZ;
        } else if (AreaIndex == 0U || AreaIndex == 2U || AreaIndex == 6U || AreaIndex == 8U) {
            unsigned short Index;
            if (AreaIndex == 2U)      { Index = (unsigned short)(XKnotNum - 1U); }
            else if (AreaIndex == 6U) { Index = (unsigned short)((YKnotNum - 1U) * XKnotNum); }
            else if (AreaIndex == 8U) { Index = (unsigned short)(YKnotNum * XKnotNum - 1U); }
            else                      { Index = 0U; }
            DefocusOkNgThr = calc_defocus_ok_ng_thr_pq(pfa_InputData, Index);
        } else if (AreaIndex == 1U || AreaIndex == 7U) {
            unsigned short Index = (AreaIndex == 1U)
                ? XKnotStart
                : (unsigned short)((YKnotNum - 1U) * XKnotNum + XKnotStart);
            signed long LineX[2], LineY[2], PointY = 0L;

            LineX[0] = (signed long)p_XAddrKnot[XKnotStart];
            LineX[1] = (signed long)p_XAddrKnot[XKnotStart + 1U];
            LineY[0] = calc_defocus_ok_ng_thr_pq(pfa_InputData, Index);
            LineY[1] = calc_defocus_ok_ng_thr_pq(pfa_InputData, (unsigned short)(Index + 1U));

            CalcAddressOnLinePq_slXslY(LineX, LineY, XCenter, &PointY);
            DefocusOkNgThr = PointY;
        } else {
            unsigned short Index = (AreaIndex == 3U)
                ? (unsigned short)(YKnotStart * XKnotNum)
                : (unsigned short)((YKnotStart + 1U) * XKnotNum - 1U);
            signed long LineX[2], LineY[2], PointY = 0L;

            LineX[0] = (signed long)p_YAddrKnot[YKnotStart];
            LineX[1] = (signed long)p_YAddrKnot[YKnotStart + 1U];
            LineY[0] = calc_defocus_ok_ng_thr_pq(pfa_InputData, Index);
            LineY[1] = calc_defocus_ok_ng_thr_pq(pfa_InputData, (unsigned short)(Index + XKnotNum));

            CalcAddressOnLinePq_slXslY(LineX, LineY, YCenter, &PointY);
            DefocusOkNgThr = PointY;
        }
    }

    if (DefocusOkNgThr <= 0L) {
        (*pfa_DefocusConfidenceLevel) = 1024UL;
    } else {
        uint64_t conf_level = (uint64_t)pfa_InputData->ConfidenceLevel;
        uint64_t density    = (pfa_InputData->DensityOfPhasePix == 0UL)
            ? 2304ULL
            : (uint64_t)pfa_InputData->DensityOfPhasePix;
        uint64_t thr        = (uint64_t)DefocusOkNgThr;
        /*
         * Exact rational `p/q` ("Counting Divided"):
         *   1024 * 2304 * ConfidenceLevel / (DensityOfPhasePix * DefocusOkNgThr)
         * where 1024 * 2304 == 2359296.
         */
        uint64_t num = 2359296ULL * conf_level;
        uint64_t den = density * thr;
        (*pfa_DefocusConfidenceLevel) = limit_confidence_u64(num / den);
    }
}

/* Zero-Float `p/q` drop-in replacement for PdLibGetDefocus */
extern signed long PdLibGetDefocusPq
(
    PdLibInputData_t    *pfa_PdLibInputData,
    PdLibOutputData_t   *pfa_PdLibOutputData
)
{
    signed long ret;
    PdLibOutputData_t out;

    pfa_PdLibOutputData->Defocus                = 0L;
    pfa_PdLibOutputData->DefocusConfidence      = D_PD_LIB_E_NG;
    pfa_PdLibOutputData->DefocusConfidenceLevel = 0UL;
    pfa_PdLibOutputData->PhaseDifference        = 0L;

    ret = check_input_pq(pfa_PdLibInputData);
    if (ret != D_PD_LIB_E_OK) {
        return ret;
    }

    out.Defocus                = 0L;
    out.DefocusConfidence      = D_PD_LIB_E_NG;
    out.DefocusConfidenceLevel = 0UL;
    out.PhaseDifference        = 0L;

    calc_defocus_pq(pfa_PdLibInputData, &out.Defocus);

    if ((pfa_PdLibInputData->XKnotNumDefocusOKNG != 0U) &&
        (pfa_PdLibInputData->YKnotNumDefocusOKNG != 0U)) {
        if (pfa_PdLibInputData->PhaseDifference != D_PD_ERROR_PHASE_SENTINEL) {
            calc_defocus_confidence_level_pq(pfa_PdLibInputData, &out.DefocusConfidenceLevel);
            if (out.DefocusConfidenceLevel >= 1024UL) {
                out.DefocusConfidence = D_PD_LIB_E_OK;
            } else {
                out.DefocusConfidence = -ELDCL;
            }
        } else {
            out.DefocusConfidenceLevel = 0UL;
            out.DefocusConfidence      = -EPDVALERR;
        }
    } else {
        out.DefocusConfidenceLevel = 0UL;
        out.DefocusConfidence      = -ENCWDDON;
    }

    out.PhaseDifference = pfa_PdLibInputData->PhaseDifference;
    (*pfa_PdLibOutputData) = out;

    return D_PD_LIB_E_OK;
}

/* Exact integer `p/q` Catmull-Rom cubic Hermite spline evaluation */
extern signed long PdPhoenixCatmullRomPq
(
    signed long p0,
    signed long p1,
    signed long p2,
    signed long p3,
    signed long t_num,
    signed long t_den
)
{
    int64_t d, d2, d3, n, n2, n3;
    int64_t q0, q1, q2, q3;
    int64_t c0, c1, c2, c3, num, den;

    if (t_den <= 0L || t_num <= 0L) {
        return p1;
    }
    if (t_num >= t_den) {
        return p2;
    }

    d  = (int64_t)t_den;
    d2 = d * d;
    d3 = d2 * d;
    n  = (int64_t)t_num;
    n2 = n * n;
    n3 = n2 * n;

    q0 = (int64_t)p0;
    q1 = (int64_t)p1;
    q2 = (int64_t)p2;
    q3 = (int64_t)p3;

    c0 = 2LL * q1 * d3;
    c1 = (-q0 + q2) * n * d2;
    c2 = (2LL * q0 - 5LL * q1 + 4LL * q2 - q3) * n2 * d;
    c3 = (-q0 + 3LL * q1 - 3LL * q2 + q3) * n3;

    num = c0 + c1 + c2 + c3;
    den = 2LL * d3;
    if (num >= 0LL) {
        return limit_defocus_i64((num + (den >> 1)) / den);
    }
    return limit_defocus_i64(-((-num + (den >> 1)) / den));
}

/* Restore sensor phase dropouts (`D_PD_ERROR_VALUE << 4`) via Catmull-Rom spline */
extern void PdPhoenixRestorePhaseDropoutsPq
(
    const signed long   *pfa_RawPhaseDiff,
    unsigned long       fa_Count,
    signed long         *pfa_RestoredPhaseDiff,
    signed char         *pfa_RestoredFlags
)
{
    const signed long err_sentinel = D_PD_ERROR_PHASE_SENTINEL;
    unsigned long i;
    unsigned long first_valid = fa_Count;
    unsigned long last_valid  = fa_Count;

    if (fa_Count == 0UL) {
        return;
    }

    for (i = 0UL; i < fa_Count; i++) {
        pfa_RestoredPhaseDiff[i] = pfa_RawPhaseDiff[i];
        pfa_RestoredFlags[i]     = (pfa_RawPhaseDiff[i] == err_sentinel) ? 1 : 0;
        if (pfa_RawPhaseDiff[i] != err_sentinel) {
            if (first_valid == fa_Count) {
                first_valid = i;
            }
            last_valid = i;
        }
    }

    if (first_valid == fa_Count) {
        /* All windows dropped out: clamp to 0 phase difference */
        for (i = 0UL; i < fa_Count; i++) {
            pfa_RestoredPhaseDiff[i] = 0L;
        }
        return;
    }

    /* Leading dropout boundary before first valid window */
    for (i = 0UL; i < first_valid; i++) {
        pfa_RestoredPhaseDiff[i] = pfa_RestoredPhaseDiff[first_valid];
    }

    /* Trailing dropout boundary after last valid window */
    for (i = last_valid + 1UL; i < fa_Count; i++) {
        pfa_RestoredPhaseDiff[i] = pfa_RestoredPhaseDiff[last_valid];
    }

    /* Interior dropout spans between valid anchors */
    i = first_valid;
    while (i < last_valid) {
        unsigned long idx1, idx2, idx0, idx3, k, span;
        int64_t p1, p2, y0, y3, m1, m2, p0, p3;

        if (pfa_RawPhaseDiff[i] != err_sentinel) {
            i++;
            continue;
        }

        idx1 = i - 1UL;
        while (i < last_valid && pfa_RawPhaseDiff[i] == err_sentinel) {
            i++;
        }
        idx2 = i;
        span = idx2 - idx1;

        /* Locate outer valid neighbors idx0 <= idx1 and idx3 >= idx2 */
        idx0 = idx1;
        if (idx1 > first_valid) {
            unsigned long scan = idx1 - 1UL;
            while (scan > first_valid && pfa_RawPhaseDiff[scan] == err_sentinel) {
                scan--;
            }
            if (pfa_RawPhaseDiff[scan] != err_sentinel) {
                idx0 = scan;
            }
        }

        idx3 = idx2;
        if (idx2 < last_valid) {
            unsigned long scan = idx2 + 1UL;
            while (scan < last_valid && pfa_RawPhaseDiff[scan] == err_sentinel) {
                scan++;
            }
            if (pfa_RawPhaseDiff[scan] != err_sentinel) {
                idx3 = scan;
            }
        }

        p1 = (int64_t)pfa_RestoredPhaseDiff[idx1];
        p2 = (int64_t)pfa_RestoredPhaseDiff[idx2];
        y0 = (int64_t)pfa_RestoredPhaseDiff[idx0];
        y3 = (int64_t)pfa_RestoredPhaseDiff[idx3];

        /* Non-uniform Bessel-Overhauser chord tangents scaled to span */
        m1 = ((p2 - y0) * (int64_t)span) / (int64_t)((idx2 > idx0) ? (idx2 - idx0) : 1UL);
        m2 = ((y3 - p1) * (int64_t)span) / (int64_t)((idx3 > idx1) ? (idx3 - idx1) : 1UL);
        p0 = p2 - 2LL * m1;
        p3 = p1 + 2LL * m2;

        for (k = 1UL; k < span; k++) {
            pfa_RestoredPhaseDiff[idx1 + k] = PdPhoenixCatmullRomPq(
                (signed long)p0,
                (signed long)p1,
                (signed long)p2,
                (signed long)p3,
                (signed long)k,
                (signed long)span
            );
        }
    }
}

/* Map signed PDAF phase difference (in 1/16 px Q4 units, range [-512..+512]) into Z_{2^32} */
static uint32_t phase_diff_to_u32(signed long fa_PhaseDiff)
{
    int64_t clamped = (int64_t)fa_PhaseDiff;
    if (clamped < -512LL) { clamped = -512LL; }
    if (clamped >  512LL) { clamped =  512LL; }
    return (uint32_t)((clamped * 4194304LL) & 0xFFFFFFFFLL);
}

static signed long u32_to_phase_diff(uint32_t fa_PhaseU32)
{
    int32_t signed_phase = (int32_t)fa_PhaseU32;
    if (signed_phase >= 0) {
        return (signed long)((signed_phase + 2097152) / 4194304);
    }
    return -(signed long)(((-signed_phase) + 2097152) / 4194304);
}

static PdPhoenixRelation_t classify_phoenix_relation(uint32_t fa_PhaseU32)
{
    int32_t sp = (int32_t)fa_PhaseU32;
    uint32_t abs_p = (sp < 0) ? (uint32_t)(-sp) : (uint32_t)sp;

    if (abs_p < 0x10000000U) {
        return E_PHOENIX_REL_BOUND_0_DEG;
    } else if (abs_p < 0x30000000U) {
        return E_PHOENIX_REL_RELATED_45_DEG;
    } else if (abs_p < 0x50000000U) {
        return E_PHOENIX_REL_INDEPENDENT_90;
    } else if (abs_p < 0x70000000U) {
        return E_PHOENIX_REL_CONTRASTING_135;
    }
    return E_PHOENIX_REL_CONFLICTING_180;
}

/* Laser Phoenix Lattice Phase-Locked Autofocus over a sequence of PDAF windows */
extern signed long PdLibPhoenixLatticeLockSequence
(
    PdLibInputData_t        *pfa_InputSequence,
    unsigned long           fa_WindowCount,
    PdLibOutputData_t       *pfa_OutputSequence,
    PdPhoenixLatticeState_t *pfa_LatticeStates
)
{
    unsigned long i;
    uint64_t total_jitter = 0ULL;
    unsigned long coherence_q16 = (unsigned long)D_PHOENIX_Q16_ONE;
    uint32_t locked_phase_u32 = 0U;
    int32_t  phase_vel_i32 = 0;

    if (pfa_InputSequence == NULL || pfa_OutputSequence == NULL ||
        pfa_LatticeStates == NULL || fa_WindowCount == 0UL) {
        return D_PD_LIB_E_NG;
    }

    /* Pass 1: Extract raw PhaseDifference and restore D_PD_ERROR_VALUE << 4 dropouts */
    {
        const signed long err_sentinel = D_PD_ERROR_PHASE_SENTINEL;
        unsigned long first_valid = fa_WindowCount;
        unsigned long last_valid  = fa_WindowCount;

        for (i = 0UL; i < fa_WindowCount; i++) {
            signed long raw_pd = pfa_InputSequence[i].PhaseDifference;
            pfa_LatticeStates[i].RestoredPhaseDiff = raw_pd;
            pfa_LatticeStates[i].DropoutRestored   = (raw_pd == err_sentinel) ? 1 : 0;
            if (raw_pd != err_sentinel) {
                if (first_valid == fa_WindowCount) {
                    first_valid = i;
                }
                last_valid = i;
            }
        }

        if (first_valid == fa_WindowCount) {
            for (i = 0UL; i < fa_WindowCount; i++) {
                pfa_LatticeStates[i].RestoredPhaseDiff = 0L;
            }
        } else {
            for (i = 0UL; i < first_valid; i++) {
                pfa_LatticeStates[i].RestoredPhaseDiff = pfa_LatticeStates[first_valid].RestoredPhaseDiff;
            }
            for (i = last_valid + 1UL; i < fa_WindowCount; i++) {
                pfa_LatticeStates[i].RestoredPhaseDiff = pfa_LatticeStates[last_valid].RestoredPhaseDiff;
            }

            i = first_valid;
            while (i < last_valid) {
                unsigned long idx1, idx2, idx0, idx3, k, span;
                int64_t p1, p2, y0, y3, m1, m2, p0, p3;

                if (pfa_InputSequence[i].PhaseDifference != err_sentinel) {
                    i++;
                    continue;
                }
                idx1 = i - 1UL;
                while (i < last_valid && pfa_InputSequence[i].PhaseDifference == err_sentinel) {
                    i++;
                }
                idx2 = i;
                span = idx2 - idx1;

                idx0 = idx1;
                if (idx1 > first_valid) {
                    unsigned long scan = idx1 - 1UL;
                    while (scan > first_valid && pfa_InputSequence[scan].PhaseDifference == err_sentinel) {
                        scan--;
                    }
                    if (pfa_InputSequence[scan].PhaseDifference != err_sentinel) {
                        idx0 = scan;
                    }
                }
                idx3 = idx2;
                if (idx2 < last_valid) {
                    unsigned long scan = idx2 + 1UL;
                    while (scan < last_valid && pfa_InputSequence[scan].PhaseDifference == err_sentinel) {
                        scan++;
                    }
                    if (pfa_InputSequence[scan].PhaseDifference != err_sentinel) {
                        idx3 = scan;
                    }
                }

                p1 = (int64_t)pfa_LatticeStates[idx1].RestoredPhaseDiff;
                p2 = (int64_t)pfa_LatticeStates[idx2].RestoredPhaseDiff;
                y0 = (int64_t)pfa_LatticeStates[idx0].RestoredPhaseDiff;
                y3 = (int64_t)pfa_LatticeStates[idx3].RestoredPhaseDiff;

                m1 = ((p2 - y0) * (int64_t)span) / (int64_t)((idx2 > idx0) ? (idx2 - idx0) : 1UL);
                m2 = ((y3 - p1) * (int64_t)span) / (int64_t)((idx3 > idx1) ? (idx3 - idx1) : 1UL);
                p0 = p2 - 2LL * m1;
                p3 = p1 + 2LL * m2;

                for (k = 1UL; k < span; k++) {
                    pfa_LatticeStates[idx1 + k].RestoredPhaseDiff = PdPhoenixCatmullRomPq(
                        (signed long)p0,
                        (signed long)p1,
                        (signed long)p2,
                        (signed long)p3,
                        (signed long)k,
                        (signed long)span
                    );
                }
            }
        }
    }

    /* Pass 2: Marine Salience Phase-Jitter Gate in Z_{2^32} */
    if (fa_WindowCount > 1UL) {
        for (i = 1UL; i < fa_WindowCount; i++) {
            uint32_t pa = phase_diff_to_u32(pfa_LatticeStates[i].RestoredPhaseDiff);
            uint32_t pb = phase_diff_to_u32(pfa_LatticeStates[i - 1UL].RestoredPhaseDiff);
            int32_t diff = (int32_t)(pa - pb);
            uint32_t abs_diff = (diff < 0) ? (uint32_t)(-diff) : (uint32_t)diff;
            total_jitter += (uint64_t)abs_diff;
        }
        {
            uint64_t mean_jitter = total_jitter / (uint64_t)(fa_WindowCount - 1UL);
            uint64_t ref_scale   = 0x10000000ULL;
            coherence_q16 = (unsigned long)((ref_scale * (uint64_t)D_PHOENIX_Q16_ONE) / (ref_scale + mean_jitter));
        }
    }

    /* Pass 3: Laser Phoenix Z_{2^32} PLL Lock + Zero-Float `p/q` Defocus Calculation */
    locked_phase_u32 = phase_diff_to_u32(pfa_LatticeStates[0].RestoredPhaseDiff);
    phase_vel_i32    = 0;

    for (i = 0UL; i < fa_WindowCount; i++) {
        PdLibInputData_t local_in = pfa_InputSequence[i];
        uint32_t meas_u32 = phase_diff_to_u32(pfa_LatticeStates[i].RestoredPhaseDiff);
        int32_t  err_i32  = (int32_t)(meas_u32 - locked_phase_u32);
        int32_t  prop_step;
        int32_t  int_step;
        signed long status;

        if (i == 0UL) {
            locked_phase_u32 = meas_u32;
        } else {
            /* Golden-ratio phi = 4181/2584 rational PLL damping */
            prop_step = (int32_t)(((int64_t)err_i32 * D_PHOENIX_PHI_DEN) / D_PHOENIX_PHI_NUM);
            int_step  = err_i32 / 8;
            if (coherence_q16 < 22938UL) {
                prop_step /= 2;
                int_step  /= 2;
            }
            phase_vel_i32    = phase_vel_i32 + int_step;
            locked_phase_u32 = locked_phase_u32 + (uint32_t)prop_step + (uint32_t)phase_vel_i32;
        }

        /* For dropout-restored windows, feed the Catmull-Rom + PLL locked phase difference */
        if (pfa_LatticeStates[i].DropoutRestored) {
            local_in.PhaseDifference = pfa_LatticeStates[i].RestoredPhaseDiff;
        } else {
            local_in.PhaseDifference = u32_to_phase_diff(locked_phase_u32);
        }

        pfa_LatticeStates[i].LockedPhaseU32     = locked_phase_u32;
        pfa_LatticeStates[i].PhaseVelocityI32   = phase_vel_i32;
        pfa_LatticeStates[i].MarineCoherenceQ16 = coherence_q16;
        pfa_LatticeStates[i].LatticeRelation    = classify_phoenix_relation(locked_phase_u32);

        status = PdLibGetDefocusPq(&local_in, &pfa_OutputSequence[i]);
        if (status != D_PD_LIB_E_OK) {
            return status;
        }
    }

    return D_PD_LIB_E_OK;
}
