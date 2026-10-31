/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * PdafPhoenixLatticePq.h
 * Zero-Float `p/q` ("Counting Divided") Defocus & Laser Phoenix Lattice
 * Phase-Locked Autofocus Extension for Sony `pdaf-library`.
 *
 * Authored by 8b-is (https://8b.is) — Visiting Sony R&D in Tokyo
 * (through October 24th; WhatsApp: +1 757-614-7542 — Chris).
 */

#ifndef __PDAF_PHOENIX_LATTICE_PQ_H__
#define __PDAF_PHOENIX_LATTICE_PQ_H__

#include <stdint.h>
#include "PdafMathFunc.h"
#include "PdafLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exact Fibonacci rational convergent for Golden Ratio phi = 4181 / 2584 */
#define D_PHOENIX_PHI_NUM                       (4181L)
#define D_PHOENIX_PHI_DEN                       (2584L)
#define D_PHOENIX_Q16_ONE                       (65536L)

/* UB-safe Q4 phase difference error sentinel (replaces negative left-shift D_PD_ERROR_VALUE << 4) */
#define D_PD_ERROR_PHASE_SENTINEL               ((signed long)(D_PD_ERROR_VALUE) * 16L)

/* 5-state Phoenix Interference Lattice Phase Relations */
typedef enum
{
    E_PHOENIX_REL_BOUND_0_DEG       = 0,    /*   0 deg : In-phase locked focus */
    E_PHOENIX_REL_RELATED_45_DEG    = 1,    /*  45 deg : Near-focus harmonic */
    E_PHOENIX_REL_INDEPENDENT_90    = 2,    /*  90 deg : Quadrature slewing */
    E_PHOENIX_REL_CONTRASTING_135   = 3,    /* 135 deg : Large defocus slew */
    E_PHOENIX_REL_CONFLICTING_180   = 4     /* 180 deg : Phase inversion / front-back flip */
} PdPhoenixRelation_t;

/* State & telemetry for Laser Phoenix Lattice Phase-Locked Loop (PLL) */
typedef struct
{
    uint32_t            LockedPhaseU32;         /* Phase locked in Z_{2^32} modular ring */
    int32_t             PhaseVelocityI32;       /* Loop integrator velocity in Z_{2^32} */
    signed long         RestoredPhaseDiff;      /* PhaseDifference after Catmull-Rom / PLL lock */
    unsigned long       MarineCoherenceQ16;     /* Marine jitter coherence in [0, 65536] */
    PdPhoenixRelation_t LatticeRelation;        /* 5-state Phoenix interference classification */
    signed char         DropoutRestored;        /* 1 if D_PD_ERROR_VALUE<<4 was spline-restored */
} PdPhoenixLatticeState_t;

/* Zero-float exact 64-bit integer `p/q` ("Counting Divided") line interpolation */
extern void CalcAddressOnLinePq_slXslY
(
    signed long *pf_x,
    signed long *pf_y,
    signed long f_xx,
    signed long *fp_yy
);

/* Zero-float exact 64-bit integer `p/q` broken-line interpolation */
extern signed char CalcAddressOnBrokenLinePq_ulXulY
(
    unsigned long *pf_x,
    unsigned long *pf_y,
    unsigned long f_PointNum,
    unsigned long f_xx,
    unsigned long *pf_yy
);

/* Zero-float exact 64-bit integer `p/q` bilinear plane interpolation */
extern signed char CalcAddressOnPlanePq_slXslYslZ
(
    signed long *pf_x,
    signed long *pf_y,
    signed long *pf_z,
    signed long f_xx,
    signed long f_yy,
    signed long *pf_zz
);

/*
 * Zero-Float `p/q` ("Counting Divided") drop-in replacement for PdLibGetDefocus.
 * Eliminates all `double` soft-float operations on ISP/MCU targets while
 * matching PdLibGetDefocus across all 9 knot zones (AreaIndex 0..8).
 */
extern signed long PdLibGetDefocusPq
(
    PdLibInputData_t    *pfa_PdLibInputData,
    PdLibOutputData_t   *pfa_PdLibOutputData
);

/*
 * Exact integer `p/q` Catmull-Rom cubic Hermite spline evaluation at
 * rational parameter t = t_num / t_den in [0, 1].
 */
extern signed long PdPhoenixCatmullRomPq
(
    signed long p0,
    signed long p1,
    signed long p2,
    signed long p3,
    signed long t_num,
    signed long t_den
);

/*
 * Restore sensor phase dropouts (`PhaseDifference == (D_PD_ERROR_VALUE << 4)`)
 * across a 1D array of PDAF windows or temporal frames using non-uniform
 * rational `p/q` Catmull-Rom cubic Hermite spline continuation.
 */
extern void PdPhoenixRestorePhaseDropoutsPq
(
    const signed long   *pfa_RawPhaseDiff,
    unsigned long       fa_Count,
    signed long         *pfa_RestoredPhaseDiff,
    signed char         *pfa_RestoredFlags
);

/*
 * Laser Phoenix Lattice Phase-Locked Autofocus over a sequence/grid of PDAF windows:
 *  1. Restores `D_PD_ERROR_VALUE << 4` dropouts via Catmull-Rom spline continuation.
 *  2. Evaluates O(1) per-window Marine structural phase-jitter coherence.
 *  3. Locks phase trajectories onto the Z_{2^32} Laser Phoenix Interference Lattice.
 *  4. Computes zero-float `p/q` defocus and confidence for each window.
 */
extern signed long PdLibPhoenixLatticeLockSequence
(
    PdLibInputData_t        *pfa_InputSequence,
    unsigned long           fa_WindowCount,
    PdLibOutputData_t       *pfa_OutputSequence,
    PdPhoenixLatticeState_t *pfa_LatticeStates
);

#ifdef __cplusplus
}
#endif

#endif /* __PDAF_PHOENIX_LATTICE_PQ_H__ */
