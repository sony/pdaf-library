/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * test_pdaf_phoenix_pq.c
 * Verification suite for Zero-Float `p/q` ("Counting Divided") Defocus &
 * Laser Phoenix Lattice Phase-Locked Autofocus on Sony `pdaf-library`.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/PdafLibrary.h"
#include "../src/PdafMathFunc.h"
#include "../src/PdafPhoenixLatticePq.h"

static void populate_fixture_input
(
    PdLibInputData_t        *in,
    signed long             *slopes,
    signed long             *offsets,
    unsigned short          *x_knots,
    unsigned short          *y_knots,
    DefocusOKNGThrLine_t    *thr_lines,
    unsigned long           *ag_pts,
    unsigned long           *conf_pts
)
{
    int i;

    x_knots[0] = 500U;
    x_knots[1] = 1500U;
    x_knots[2] = 2500U;

    y_knots[0] = 400U;
    y_knots[1] = 1200U;
    y_knots[2] = 2000U;

    ag_pts[0] = 100UL;
    ag_pts[1] = 400UL;
    ag_pts[2] = 1600UL;

    conf_pts[0] = 80UL;
    conf_pts[1] = 160UL;
    conf_pts[2] = 320UL;

    for (i = 0; i < 9; i++) {
        slopes[i]  = 1100L + (signed long)(i * 35);
        offsets[i] = -120L + (signed long)(i * 25);
        thr_lines[i].PointNum     = 3UL;
        thr_lines[i].p_AnalogGain = ag_pts;
        thr_lines[i].p_Confidence = conf_pts;
    }

    memset(in, 0, sizeof(*in));
    in->PhaseDifference            = 96L;
    in->ConfidenceLevel            = 260UL;
    in->XSizeOfImage               = 3000U;
    in->YSizeOfImage               = 2400U;
    in->XAddressOfWindowStart      = 900U;
    in->XAddressOfWindowEnd        = 1100U;
    in->YAddressOfWindowStart      = 700U;
    in->YAddressOfWindowEnd        = 900U;
    in->XKnotNumSlopeOffset        = 3U;
    in->YKnotNumSlopeOffset        = 3U;
    in->p_SlopeData                = slopes;
    in->p_OffsetData               = offsets;
    in->p_XAddressKnotSlopeOffset  = x_knots;
    in->p_YAddressKnotSlopeOffset  = y_knots;
    in->AdjCoeffSlope              = D_PD_LIB_SLOPE_ADJ_COEFF_SENS_MODE0;
    in->ImagerAnalogGain           = 250UL;
    in->XKnotNumDefocusOKNG        = 3U;
    in->YKnotNumDefocusOKNG        = 3U;
    in->p_DefocusOKNGThrLine       = thr_lines;
    in->p_XAddressKnotDefocusOKNG  = x_knots;
    in->p_YAddressKnotDefocusOKNG  = y_knots;
    in->DensityOfPhasePix          = D_PD_LIB_DENSITY_SENS_MODE0;
}

static void test_bit_exact_parity_all_9_areas(void)
{
    /* Window centers spanning all 9 AreaIndex regions (0..8) */
    const unsigned short win_x_starts[9] = {
        100U,  900U, 2600U,
        100U, 1000U, 2600U,
        100U, 1700U, 2600U
    };
    const unsigned short win_y_starts[9] = {
        100U,  100U,  100U,
        700U,  800U, 1500U,
        2100U, 2100U, 2100U
    };
    const signed long phase_diffs[5] = { -240L, -45L, 0L, 73L, 315L };

    signed long slopes[9], offsets[9];
    unsigned short x_knots[3], y_knots[3];
    DefocusOKNGThrLine_t thr_lines[9];
    unsigned long ag_pts[3], conf_pts[3];
    PdLibInputData_t in;
    int area, p;

    populate_fixture_input(&in, slopes, offsets, x_knots, y_knots, thr_lines, ag_pts, conf_pts);

    for (area = 0; area < 9; area++) {
        for (p = 0; p < 5; p++) {
            PdLibOutputData_t out_ref, out_pq;
            signed long ret_ref, ret_pq;

            in.XAddressOfWindowStart = win_x_starts[area];
            in.XAddressOfWindowEnd   = (unsigned short)(win_x_starts[area] + 200U);
            in.YAddressOfWindowStart = win_y_starts[area];
            in.YAddressOfWindowEnd   = (unsigned short)(win_y_starts[area] + 200U);
            in.PhaseDifference       = phase_diffs[p];

            ret_ref = PdLibGetDefocus(&in, &out_ref);
            ret_pq  = PdLibGetDefocusPq(&in, &out_pq);

            assert(ret_ref == D_PD_LIB_E_OK);
            assert(ret_pq == ret_ref);
            assert(out_pq.Defocus == out_ref.Defocus);
            assert(out_pq.DefocusConfidence == out_ref.DefocusConfidence);
            assert(out_pq.DefocusConfidenceLevel == out_ref.DefocusConfidenceLevel);
            assert(out_pq.PhaseDifference == out_ref.PhaseDifference);
        }
    }
}

static void test_error_handling_parity(void)
{
    signed long slopes[9], offsets[9];
    unsigned short x_knots[3], y_knots[3];
    DefocusOKNGThrLine_t thr_lines[9];
    unsigned long ag_pts[3], conf_pts[3];
    PdLibInputData_t in;
    PdLibOutputData_t out_ref, out_pq;
    signed long ret_ref, ret_pq;

    populate_fixture_input(&in, slopes, offsets, x_knots, y_knots, thr_lines, ag_pts, conf_pts);

    /* Sensor error sentinel D_PD_ERROR_PHASE_SENTINEL */
    in.PhaseDifference = D_PD_ERROR_PHASE_SENTINEL;
    ret_ref = PdLibGetDefocus(&in, &out_ref);
    ret_pq  = PdLibGetDefocusPq(&in, &out_pq);
    assert(ret_ref == ret_pq);
    assert(out_ref.DefocusConfidence == -EPDVALERR);
    assert(out_pq.DefocusConfidence == -EPDVALERR);
    assert(out_pq.DefocusConfidenceLevel == 0UL);
}

static void test_laser_phoenix_lattice_lock_and_dropout_recovery(void)
{
    signed long slopes[9], offsets[9];
    unsigned short x_knots[3], y_knots[3];
    DefocusOKNGThrLine_t thr_lines[9];
    unsigned long ag_pts[3], conf_pts[3];
    PdLibInputData_t seq_in[12];
    PdLibOutputData_t seq_out[12];
    PdPhoenixLatticeState_t states[12];
    signed long gt_phase[12];
    unsigned long i;
    signed long status;

    for (i = 0UL; i < 12UL; i++) {
        populate_fixture_input(&seq_in[i], slopes, offsets, x_knots, y_knots, thr_lines, ag_pts, conf_pts);
        /* Smooth focus pull from -160 to +170 (step +30 per frame) */
        gt_phase[i] = -160L + (signed long)i * 30L;
        seq_in[i].PhaseDifference = gt_phase[i];
    }

    /* Inject sensor phase dropouts (D_PD_ERROR_PHASE_SENTINEL) at windows 4, 5 and 8 */
    seq_in[4].PhaseDifference = D_PD_ERROR_PHASE_SENTINEL;
    seq_in[5].PhaseDifference = D_PD_ERROR_PHASE_SENTINEL;
    seq_in[8].PhaseDifference = D_PD_ERROR_PHASE_SENTINEL;

    /* Standard PdLibGetDefocus fails with -EPDVALERR on window 4 */
    {
        PdLibOutputData_t raw_out;
        PdLibGetDefocus(&seq_in[4], &raw_out);
        assert(raw_out.DefocusConfidence == -EPDVALERR);
    }

    /* Laser Phoenix Lattice Lock recovers dropouts via Catmull-Rom spline + Z_{2^32} PLL */
    status = PdLibPhoenixLatticeLockSequence(seq_in, 12UL, seq_out, states);
    assert(status == D_PD_LIB_E_OK);

    for (i = 0UL; i < 12UL; i++) {
        signed long diff_err = labs(states[i].RestoredPhaseDiff - gt_phase[i]);
        assert(diff_err <= 2L);
        assert(seq_out[i].DefocusConfidence == D_PD_LIB_E_OK);
        assert(seq_out[i].DefocusConfidenceLevel >= 1024UL);
        assert(states[i].MarineCoherenceQ16 > 40000UL);
    }
    assert(states[4].DropoutRestored == 1);
    assert(states[5].DropoutRestored == 1);
    assert(states[8].DropoutRestored == 1);
    assert(states[0].DropoutRestored == 0);
}

int main(void)
{
    test_bit_exact_parity_all_9_areas();
    test_error_handling_parity();
    test_laser_phoenix_lattice_lock_and_dropout_recovery();
    printf("ALL SONY PDAF PHOENIX LATTICE PQ TESTS PASSED (0 DN error, 100%% dropout recovery)\n");
    return 0;
}
