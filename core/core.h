/*****************************************************************************
 * core.h: h264 encoder
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: core.h,v 1.1 2004/06/03 19:27:06 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _CORE_H
#define _CORE_H 1

#define HAVE_STDINT_H 1 //lsp051226
#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdarg.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#define P264_VERSION "" // no configure script for msvc
#endif

/****************************************************************************
 * Macros
 ****************************************************************************/
#define P264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define P264_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define P264_MIN3(a,b,c) P264_MIN((a),P264_MIN((b),(c)))
#define P264_MAX3(a,b,c) P264_MAX((a),P264_MAX((b),(c)))
#define P264_MIN4(a,b,c,d) P264_MIN((a),P264_MIN3((b),(c),(d)))
#define P264_MAX4(a,b,c,d) P264_MAX((a),P264_MAX3((b),(c),(d)))
#define XCHG(type,a,b) { type t = a; a = b; b = t; }
#define FIX8(f) ((int)(f*(1<<8)+.5))

/****************************************************************************
 * Includes
 ****************************************************************************/
#include "p264.h"
#include "bs.h"
#include "set.h"
#include "predict.h"
#include "pixel.h"
#include "mc.h"
#include "frame.h"
#include "dct.h"
#include "cabac.h"
#include "csp.h"
#include "quant.h"

/****************************************************************************
 * Generals functions
 ****************************************************************************/
/* p264_malloc : will do or emulate a memalign
 * XXX you HAVE TO use p264_free for buffer allocated
 * with p264_malloc
 */
void *p264_malloc( int );
void *p264_realloc( void *p, int i_size );
void  p264_free( void * );

/* p264_slurp_file: malloc space for the whole file and read it */
char *p264_slurp_file( const char *filename );

/* mdate: return the current date in microsecond */
int64_t p264_mdate( void );

/* p264_param2string: return a (malloced) string containing most of
 * the encoding options */
char *p264_param2string( p264_param_t *p, int b_res );

/* log */
void p264_log( p264_t *h, int i_level, const char *psz_fmt, ... );

static inline int p264_clip3( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}

static inline float p264_clip3f( float v, float f_min, float f_max )
{
    return ( (v < f_min) ? f_min : (v > f_max) ? f_max : v );
}

static inline int p264_median( int a, int b, int c )
{
    int min = a, max =a;
    if( b < min )
        min = b;
    else
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */

    if( c < min )
        min = c;
    else if( c > max )
        max = c;

    return a + b + c - min - max;
}


/****************************************************************************
 *
 ****************************************************************************/
enum slice_type_e
{
    SLICE_TYPE_P  = 0,
    SLICE_TYPE_B  = 1,
    SLICE_TYPE_I  = 2,
    SLICE_TYPE_SP = 3,
    SLICE_TYPE_SI = 4
};

static const char slice_type_to_char[] = { 'P', 'B', 'I', 'S', 'S' };

typedef struct
{
    p264_sps_t *sps;
    p264_pps_t *pps;

    int i_type;
    int i_first_mb;
    int i_last_mb;

    int i_pps_id;

    int i_frame_num;

    int b_field_pic;
    int b_bottom_field;

    int i_idr_pic_id;   /* -1 if nal_type != 5 */

    int i_poc_lsb;
    int i_delta_poc_bottom;

    int i_delta_poc[2];
    int i_redundant_pic_cnt;

    int b_direct_spatial_mv_pred;

    int b_num_ref_idx_override;
    int i_num_ref_idx_l0_active;
    int i_num_ref_idx_l1_active;

    int b_ref_pic_list_reordering_l0;
    int b_ref_pic_list_reordering_l1;
    struct {
        int idc;
        int arg;
    } ref_pic_list_order[2][16];

    //predict weight //lsptodo

    //dec ref pic marking //lsp
    int b_no_output_of_prior_pics;
    int b_long_term_reference_flag;
    int b_adaptive_ref_pic_marking_mode;
    struct {
      int i_control_operation; //memory mangement control operation
      int arg;
    } ref_pic_marking[16]; //is 16 enough?


    int i_cabac_init_idc;

    int i_qp;
    int i_qp_delta;
    int b_sp_for_swidth;
    int i_qs_delta;

    /* deblocking filter */
    int i_disable_deblocking_filter_idc;
    int i_alpha_c0_offset;
    int i_beta_offset;

} p264_slice_header_t;

/* From ffmpeg
 */
#define P264_SCAN8_SIZE (6*8)
#define P264_SCAN8_0 (4+1*8)

static const int p264_scan8[16+2*4] =
{
    /* Luma */
    4+1*8, 5+1*8, 4+2*8, 5+2*8,
    6+1*8, 7+1*8, 6+2*8, 7+2*8,
    4+3*8, 5+3*8, 4+4*8, 5+4*8,
    6+3*8, 7+3*8, 6+4*8, 7+4*8,

    /* Cb */
    1+1*8, 2+1*8,
    1+2*8, 2+2*8,

    /* Cr */
    1+4*8, 2+4*8,
    1+5*8, 2+5*8,
};
/*
   0 1 2 3 4 5 6 7
 0
 1   B B   L L L L
 2   B B   L L L L
 3         L L L L
 4   R R   L L L L
 5   R R
*/


#define P264_BFRAME_MAX 16
#define P264_SLICE_MAX 4
#define P264_NAL_MAX (4 + P264_SLICE_MAX)

typedef struct p264_ratecontrol_t   p264_ratecontrol_t;
typedef struct p264_vlc_table_t     p264_vlc_table_t;

struct p264_t
{
    /* encoder parameters */
    p264_param_t    param;

    p264_t *thread[P264_SLICE_MAX];

    /* bitstream output */
    struct
    {
        int         i_nal;
        p264_nal_t  nal[P264_NAL_MAX];
        int         i_bitstream;    /* size of p_bitstream */
        uint8_t     *p_bitstream;   /* will hold data for all nal */
        bs_t        bs;
    } out;

    /* frame number/poc */
    int             i_frame;

    int             i_frame_offset; /* decoding only */
    int             i_frame_num;    /* decoding only */
    int             i_poc_msb;      /* decoding only */
    int             i_poc_lsb;      /* decoding only */
    int             i_poc;          /* decoding only */
    int i_skip_run; //lsp decoding only

    int             i_thread_num;   /* threads only */
    int             i_nal_type;     /* threads only */
    int             i_nal_ref_idc;  /* threads only */

    /* We use only one SPS and one PPS */
    p264_sps_t      sps_array[32]; //lsp
    p264_sps_t      *sps;
    p264_pps_t      pps_array[256]; //lsp
    p264_pps_t      *pps;
    int             i_idr_pic_id;

    int b_pic_output; //decoder only
    p264_picture_t *pic_dec; //lsp

    int             dequant4_mf[4][6][4][4];
    int             dequant8_mf[2][6][8][8];
    int             quant4_mf[4][6][4][4];
    int             quant8_mf[2][6][8][8];
    int             unquant4_mf[4][52][16];
    int             unquant8_mf[2][52][64];

    /* Slice header */
    p264_slice_header_t sh;

    /* cabac context */
    p264_cabac_t    cabac;

    struct
    {
        /* Frames to be encoded (whose types have been decided) */
        p264_frame_t *current[P264_BFRAME_MAX+3];
        /* Temporary buffer (frames types not yet decided) */
        p264_frame_t *next[P264_BFRAME_MAX+3];
        /* Unused frames */
        p264_frame_t *unused[P264_BFRAME_MAX+3];
        /* For adaptive B decision */
        p264_frame_t *last_nonb;

        /* frames used for reference +1 for decoding + sentinels */
        p264_frame_t *reference[16+2+1+2];
        int i_used_size;

        int i_last_idr; /* Frame number of the last IDR */

        int i_input;    /* Number of input frames already accepted */

        int i_max_dpb;  /* Number of frames allocated in the decoded picture buffer */
        int i_max_ref0;
        int i_max_ref1;
        int i_delay;    /* Number of frames buffered for B reordering */
        int b_have_lowres;  /* Whether 1/2 resolution luma planes are being used */
    } frames;

    /* current frame being encoded */
    p264_frame_t    *fenc;

    /* frame being reconstructed */
    p264_frame_t    *fdec;

    /* references lists */
    int             i_ref0;
    p264_frame_t    *fref0[16+3];     /* ref list 0 */
    int             i_ref1;
    p264_frame_t    *fref1[16+3];     /* ref list 1 */
    int             b_ref_reorder[2];



    /* Current MB DCT coeffs */
    struct dct_t //lsp
    {
        DECLARE_ALIGNED( int, luma16x16_dc[16], 16 );
        DECLARE_ALIGNED( int, chroma_dc[2][4], 16 );
        // FIXME merge with union
        DECLARE_ALIGNED( int, luma8x8[4][64], 16 );
        union
        {
            DECLARE_ALIGNED( int, residual_ac[15], 16 );
            DECLARE_ALIGNED( int, luma4x4[16], 16 );
        } block[16+8];
    } dct;

    /* MB table and cache for current frame/mb */
    struct
    {
        int     i_mb_count;                 /* number of mbs in a frame */

        /* Strides */
        int     i_mb_stride;
        int     i_b8_stride;
        int     i_b4_stride;

        /* Current index */
        int     i_mb_x;
        int     i_mb_y;
        int     i_mb_xy;
        int     i_b8_xy;
        int     i_b4_xy;
        
        /* Search parameters */
        int     i_me_method;
        int     i_subpel_refine;
        int     b_chroma_me;
        int     b_trellis;
        /* Allowed qpel MV range to stay within the picture + emulated edge pixels */
        int     mv_min[2];
        int     mv_max[2];
        /* Fullpel MV range for motion search */
        int     mv_min_fpel[2];
        int     mv_max_fpel[2];

        /* neighboring MBs */
        unsigned int i_neighbour;
        unsigned int i_neighbour8[4];       /* neighbours of each 8x8 or 4x4 block that are available */
        unsigned int i_neighbour4[16];      /* at the time the block is coded */
        int     i_mb_type_top; 
        int     i_mb_type_left; 
        int     i_mb_type_topleft; 
        int     i_mb_type_topright; 

        /* mb table */
        int8_t  *type;                      /* mb type */
        int8_t  *qp;                        /* mb qp */
        int16_t *cbp;                       /* mb cbp: 0x0?: luma, 0x?0: chroma, 0x100: luma dc, 0x0200 and 0x0400: chroma dc  (all set for PCM)*/
        int8_t  (*intra4x4_pred_mode)[7];   /* intra4x4 pred mode. for non I4x4 set to I_PRED_4x4_DC(2) */
        uint8_t (*non_zero_count)[16+4+4];  /* nzc. for I_PCM set to 16 */
        int8_t  *chroma_pred_mode;          /* chroma_pred_mode. cabac only. for non intra I_PRED_CHROMA_DC(0) */
        int16_t (*mv[2])[2];                /* mb mv. set to 0 for intra mb */
        int16_t (*mvd[2])[2];               /* mb mv difference with predict. set to 0 if intra. cabac only */
        int8_t   *ref[2];                   /* mb ref. set to -1 if non used (intra or Lx only) */
        int16_t (*mvr[2][16])[2];           /* 16x16 mv for each possible ref */
        int8_t  *skipbp;                    /* block pattern for SKIP or DIRECT (sub)mbs. B-frames + cabac only */
        int8_t  *mb_transform_size;         /* transform_size_8x8_flag of each mb */

        /* current value */
        int     i_type;
        int     i_partition;
        int     i_sub_partition[4];
        int     b_transform_8x8;

        int     i_cbp_luma;
        int     i_cbp_chroma;

        int     i_intra16x16_pred_mode;
        int     i_chroma_pred_mode;

        struct
        {
            /* pointer over mb of the frame to be compressed */
            uint8_t *p_fenc[3];

            /* pointer over mb of the frame to be reconstrucated  */
            uint8_t *p_fdec[3];

            /* pointer over mb of the references */
            uint8_t *p_fref[2][16][4+2]; /* last: lN, lH, lV, lHV, cU, cV */
            uint16_t *p_integral[2][16];

            /* core stride */
            int     i_stride[3];
        } pic;

        /* cache */
        struct
        {
            /* real intra4x4_pred_mode if I_4X4 or I_8X8, I_PRED_4x4_DC if mb available, -1 if not */
            int     intra4x4_pred_mode[P264_SCAN8_SIZE];

            /* i_non_zero_count if availble else 0x80 */
            int     non_zero_count[P264_SCAN8_SIZE];

            /* -1 if unused, -2 if unavaible */
            int8_t  ref[2][P264_SCAN8_SIZE];

            /* 0 if non avaible */
            int16_t mv[2][P264_SCAN8_SIZE][2];
            int16_t mvd[2][P264_SCAN8_SIZE][2];

            /* 1 if SKIP or DIRECT. set only for B-frames + CABAC */
            int8_t  skip[P264_SCAN8_SIZE];

            int16_t direct_mv[2][P264_SCAN8_SIZE][2];
            int8_t  direct_ref[2][P264_SCAN8_SIZE];

            /* number of neighbors (top and left) that used 8x8 dct */
            int     i_neighbour_transform_size;
            int     b_transform_8x8_allowed;
        } cache;

        /* */
        int     i_qp;       /* current qp */
        int     i_last_qp;  /* last qp */
        int     i_last_dqp; /* last delta qp */
        int     b_variable_qp; /* whether qp is allowed to vary per macroblock */
        int     b_lossless;

        /* B_direct and weighted prediction */
        int     dist_scale_factor[16][16];
        int     bipred_weight[16][16];
        /* maps fref1[0]'s ref indices into the current list0 */
        int     map_col_to_list0_buf[2]; // for negative indices
        int     map_col_to_list0[16];
    } mb;

    /* rate control encoding only */
    p264_ratecontrol_t *rc;

    int i_last_inter_size;
    int i_last_intra_size;
    int i_last_intra_qp;

    /* stats */
    struct
    {
        /* Current frame stats */
        struct
        {
            /* Headers bits (MV+Ref+MB Block Type */
            int i_hdr_bits;
            /* Texture bits (Intra/Predicted) */
            int i_itex_bits;
            int i_ptex_bits;
            /* ? */
            int i_misc_bits;
            /* MB type counts */
            int i_mb_count[19];
            int i_mb_count_i;
            int i_mb_count_p;
            int i_mb_count_skip;
            int i_mb_count_8x8dct[2];
            int i_mb_count_size[7];
            int i_mb_count_ref[16];
            /* Estimated (SATD) cost as Intra/Predicted frame */
            /* XXX: both omit the cost of MBs coded as P_SKIP */
            int i_intra_cost;
            int i_inter_cost;
        } frame;

        /* Cummulated stats */

        /* per slice info */
        int   i_slice_count[5];
        int64_t i_slice_size[5];
        int     i_slice_qp[5];
        /* */
        int64_t i_sqe_global[5];
        float   f_psnr_average[5];
        float   f_psnr_mean_y[5];
        float   f_psnr_mean_u[5];
        float   f_psnr_mean_v[5];
        /* */
        int64_t i_mb_count[5][19];
        int64_t i_mb_count_8x8dct[2];
        int64_t i_mb_count_size[2][7];
        int64_t i_mb_count_ref[2][16];

    } stat;

    /* CPU functions dependants */
    p264_predict_t      predict_16x16[4+3];
    p264_predict_t      predict_8x8c[4+3];
    p264_predict8x8_t   predict_8x8[9+3];
    p264_predict_t      predict_4x4[9+3];

    p264_pixel_function_t pixf;
    p264_mc_functions_t   mc;
    p264_dct_function_t   dctf;
    p264_csp_function_t   csp;
    p264_quant_function_t quantf;
    p264_deblock_function_t loopf;

    /* vlc table for decoding purpose only */
    p264_vlc_table_t *p264_coeff_token_lookup[5];
    p264_vlc_table_t *p264_level_prefix_lookup;
    p264_vlc_table_t *p264_total_zeros_lookup[15];
    p264_vlc_table_t *p264_total_zeros_dc_lookup[3];
    p264_vlc_table_t *p264_run_before_lookup[7];

#if VISUALIZE
    struct visualize_t *visualize;
#endif
};

// included at the end because it needs p264_t
#include "macroblock.h"

#endif

