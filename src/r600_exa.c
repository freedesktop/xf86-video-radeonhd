/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Alex Deucher <alexander.deucher@amd.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"

#include "exa.h"

#include "rhd.h"
#include "rhd_cs.h"
#include "r6xx_accel.h"
#include "r600_shader.h"
#include "r600_reg.h"
#include "r600_state.h"

//#define SHOW_VERTEXES

#       define RADEON_ROP3_ZERO             0x00000000
#       define RADEON_ROP3_DSa              0x00880000
#       define RADEON_ROP3_SDna             0x00440000
#       define RADEON_ROP3_S                0x00cc0000
#       define RADEON_ROP3_DSna             0x00220000
#       define RADEON_ROP3_D                0x00aa0000
#       define RADEON_ROP3_DSx              0x00660000
#       define RADEON_ROP3_DSo              0x00ee0000
#       define RADEON_ROP3_DSon             0x00110000
#       define RADEON_ROP3_DSxn             0x00990000
#       define RADEON_ROP3_Dn               0x00550000
#       define RADEON_ROP3_SDno             0x00dd0000
#       define RADEON_ROP3_Sn               0x00330000
#       define RADEON_ROP3_DSno             0x00bb0000
#       define RADEON_ROP3_DSan             0x00770000
#       define RADEON_ROP3_ONE              0x00ff0000

uint32_t RADEON_ROP[16] = {
    RADEON_ROP3_ZERO, /* GXclear        */
    RADEON_ROP3_DSa,  /* Gxand          */
    RADEON_ROP3_SDna, /* GXandReverse   */
    RADEON_ROP3_S,    /* GXcopy         */
    RADEON_ROP3_DSna, /* GXandInverted  */
    RADEON_ROP3_D,    /* GXnoop         */
    RADEON_ROP3_DSx,  /* GXxor          */
    RADEON_ROP3_DSo,  /* GXor           */
    RADEON_ROP3_DSon, /* GXnor          */
    RADEON_ROP3_DSxn, /* GXequiv        */
    RADEON_ROP3_Dn,   /* GXinvert       */
    RADEON_ROP3_SDno, /* GXorReverse    */
    RADEON_ROP3_Sn,   /* GXcopyInverted */
    RADEON_ROP3_DSno, /* GXorInverted   */
    RADEON_ROP3_DSan, /* GXnand         */
    RADEON_ROP3_ONE,  /* GXset          */
};

static Bool
R600PrepareSolid(PixmapPtr pPix, int alu, Pixel pm, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPix->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    cb_config_t     cb_conf;
    shader_config_t vs_conf, ps_conf;
    uint64_t vs_addr, ps_addr;
    int i = 0;
    int pmask = 0;
    int pix_pitch;
    uint32_t pix_offset;
    uint32_t vs[16];
    uint32_t ps[2];

    //0
    vs[i++] = CF_DWORD0(ADDR(4));
    vs[i++] = CF_DWORD1(POP_COUNT(0),
			CF_CONST(0),
			COND(SQ_CF_COND_ACTIVE),
			I_COUNT(2),
			CALL_COUNT(0),
			END_OF_PROGRAM(0),
			VALID_PIXEL_MODE(0),
			CF_INST(SQ_CF_INST_VTX),
			WHOLE_QUAD_MODE(0),
			BARRIER(1));
    //1
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_POS0),
				      TYPE(SQ_EXPORT_POS),
				      RW_GPR(1),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(0));
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(0),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(1));
    //2
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(0),
				      TYPE(SQ_EXPORT_PARAM),
				      RW_GPR(2),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(0));
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(1),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(0));
    //3
    vs[i++] = 0x00000000;
    vs[i++] = 0x00000000;
    //4/5
    vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			 FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			 FETCH_WHOLE_QUAD(0),
			 BUFFER_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 SRC_SEL_X(SQ_SEL_X),
			 MEGA_FETCH_COUNT(12));
    vs[i++] = VTX_DWORD1_GPR(DST_GPR(1),
			     DST_REL(0),
			     DST_SEL_X(SQ_SEL_X),
			     DST_SEL_Y(SQ_SEL_Y),
			     DST_SEL_Z(SQ_SEL_0),
			     DST_SEL_W(SQ_SEL_1),
			     USE_CONST_FIELDS(0),
			     DATA_FORMAT(FMT_32_32_FLOAT), //xxx
			     NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
			     FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
			     SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
    vs[i++] = VTX_DWORD2(OFFSET(0),
			 ENDIAN_SWAP(ENDIAN_NONE),
			 CONST_BUF_NO_STRIDE(0),
			 MEGA_FETCH(1));
    vs[i++] = VTX_DWORD_PAD;

    //6/7
    if (pPix->drawable.bitsPerPixel == 8) {
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(1));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(2),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_0),
				 DST_SEL_Z(SQ_SEL_0),
				 DST_SEL_W(SQ_SEL_0),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_8),
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM),
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_UNSIGNED),
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(11),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(0));
	vs[i++] = VTX_DWORD_PAD;
    } else if (pPix->drawable.bitsPerPixel == 16) {
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(2));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(2),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_Z),
				 DST_SEL_W(SQ_SEL_0),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_5_6_5),
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM),
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_UNSIGNED),
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(10),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(0));
	vs[i++] = VTX_DWORD_PAD;
    } else {
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(4));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(2),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_Z),
				 DST_SEL_W(SQ_SEL_W),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_8_8_8_8),
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM),
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_UNSIGNED),
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(8),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(0));
	vs[i++] = VTX_DWORD_PAD;
    }

    i = 0;
    ps[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_PIXEL_MRT0),
				      TYPE(SQ_EXPORT_PIXEL),
				      RW_GPR(0),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(1));
    ps[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(1),
					   END_OF_PROGRAM(1),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(1));

    pix_pitch = exaGetPixmapPitch(pPix) / (pPix->drawable.bitsPerPixel / 8);
    pix_offset = exaGetPixmapOffset(pPix) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    // bad pitch
    if (pix_pitch & 63)
	return FALSE;

    // bad offset
    if (pix_offset & 0xff)
	return FALSE;

    if (pPix->drawable.bitsPerPixel == 24)
	return FALSE;

    // XXX: FIX ME
    if (pPix->drawable.bitsPerPixel == 8)
	return FALSE;

    CLEAR (cb_conf);
    CLEAR (vs_conf);
    CLEAR (ps_conf);

    //return FALSE;

#ifdef SHOW_VERTEXES
    ErrorF("%dx%d @ %dbpp, 0x%08x\n", pPix->drawable.width, pPix->drawable.height,
	   pPix->drawable.bitsPerPixel, exaGetPixmapPitch(pPix));
#endif

    accel_state->ib = RHDDRMCPBuffer(pScrn->scrnIndex);

    /* Init */
    start_3d(pScrn, accel_state->ib);

    cp_set_surface_sync(pScrn, accel_state->ib);

    set_default_state(pScrn, accel_state->ib);

    /* Scissor / viewport */
    ereg  (accel_state->ib, PA_CL_VTE_CNTL,                      VTX_XY_FMT_bit);
    ereg  (accel_state->ib, PA_CL_CLIP_CNTL,                     CLIP_DISABLE_bit);

    //return FALSE;

    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 512, vs, sizeof(vs));
    vs_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 512;
    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 256, ps, sizeof(ps));
    ps_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 256;

    /* Shader */
    vs_conf.shader_addr         = vs_addr;
    vs_conf.num_gprs            = 3;
    vs_conf.stack_size          = 0;
    vs_setup                    (pScrn, accel_state->ib, &vs_conf);

    ps_conf.shader_addr         = ps_addr;
    ps_conf.num_gprs            = 1;
    ps_conf.stack_size          = 0;
    ps_conf.uncached_first_inst = 1;
    ps_conf.clamp_consts        = 0;
    ps_conf.export_mode         = 2;
    ps_setup                    (pScrn, accel_state->ib, &ps_conf);

    /* Render setup */
    if (pm & 0x000000ff)
	pmask |= 1;
    if (pm & 0x0000ff00)
	pmask |= 2;
    if (pm & 0x00ff0000)
	pmask |= 4;
    if (pm & 0xff000000)
	pmask |= 8;
    ereg  (accel_state->ib, CB_SHADER_MASK,                      (pmask << OUTPUT0_ENABLE_shift));
    ereg  (accel_state->ib, R7xx_CB_SHADER_CONTROL,              (RT0_ENABLE_bit));
    ereg  (accel_state->ib, CB_COLOR_CONTROL,                    RADEON_ROP[alu]);

    cb_conf.id = 0;
    cb_conf.w = pix_pitch;
    cb_conf.h = pPix->drawable.height;
    cb_conf.base = exaGetPixmapOffset(pPix) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    if (pPix->drawable.bitsPerPixel == 8)
	cb_conf.format = COLOR_8;
    else if (pPix->drawable.bitsPerPixel == 16)
	cb_conf.format = COLOR_5_6_5;
    else
	cb_conf.format = COLOR_8_8_8_8;
    cb_conf.comp_swap = 0;
    cb_conf.source_format = 1;
    cb_conf.blend_clamp = 1;
    set_render_target(pScrn, accel_state->ib, &cb_conf);

    ereg  (accel_state->ib, PA_SU_SC_MODE_CNTL,                  (FACE_bit			|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_FRONT_PTYPE_shift)	|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_BACK_PTYPE_shift)));
    ereg  (accel_state->ib, DB_SHADER_CONTROL,                   ((1 << Z_ORDER_shift)		| /* EARLY_Z_THEN_LATE_Z */
						 DUAL_EXPORT_ENABLE_bit)); /* Only useful if no depth export */

    /* Interpolator setup */
    // export pixel color from VS
    ereg  (accel_state->ib, SPI_VS_OUT_CONFIG, ((1 - 1) << VS_EXPORT_COUNT_shift));
    ereg  (accel_state->ib, SPI_VS_OUT_ID_0, (0 << SEMANTIC_0_shift));

    /* Enabling flat shading needs both FLAT_SHADE_bit in SPI_PS_INPUT_CNTL_x
     * *and* FLAT_SHADE_ENA_bit in SPI_INTERP_CONTROL_0 */
    // input color from VS
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_0,                 (1 << NUM_INTERP_shift));
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_1,                 0);
    // color semantic id 0 -> GPR[0]
    ereg  (accel_state->ib, SPI_PS_INPUT_CNTL_0 + (0 <<2),       ((0    << SEMANTIC_shift)	|
								  (0x03 << DEFAULT_VAL_shift)	|
								  //FLAT_SHADE_bit		|
								  SEL_CENTROID_bit));
    ereg  (accel_state->ib, SPI_INTERP_CONTROL_0,                /* FLAT_SHADE_ENA_bit | */0);

    accel_state->solid_fg = (uint32_t)fg;

    accel_state->vb_index = 0;

#ifdef SHOW_VERTEXES
    ErrorF("PM: 0x%08x\n", pm);
#endif

    return TRUE;
}


static void
R600Solid(PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
    ScrnInfoPtr pScrn = xf86Screens[pPix->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    struct r6xx_solid_vertex vertex[3];
    struct r6xx_solid_vertex *solid_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));

    vertex[0].x = (float)x1;
    vertex[0].y = (float)y1;
    vertex[0].color = accel_state->solid_fg;

    vertex[1].x = (float)x1;
    vertex[1].y = (float)y2;
    vertex[1].color = accel_state->solid_fg;

    vertex[2].x = (float)x2;
    vertex[2].y = (float)y2;
    vertex[2].color = accel_state->solid_fg;

#ifdef SHOW_VERTEXES
    ErrorF("vertex 0: %f, %f, 0x%08x\n", vertex[0].x, vertex[0].y, vertex[0].color);
    ErrorF("vertex 1: %f, %f, 0x%08x\n", vertex[1].x, vertex[1].y, vertex[1].color);
    ErrorF("vertex 2: %f, %f, 0x%08x\n", vertex[2].x, vertex[2].y, vertex[2].color);
#endif

    // append to vertex buffer
    solid_vb[accel_state->vb_index++] = vertex[0];
    solid_vb[accel_state->vb_index++] = vertex[1];
    solid_vb[accel_state->vb_index++] = vertex[2];
}

static void
R600DoneSolid(PixmapPtr pPix)
{
    ScrnInfoPtr pScrn = xf86Screens[pPix->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    uint64_t vb_addr;
    draw_config_t   draw_conf;
    vtx_resource_t  vtx_res;

    vb_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2);

    CLEAR (draw_conf);
    CLEAR (vtx_res);

    if (accel_state->vb_index == 0) {
	R600CPFlushIndirect(pScrn, accel_state->ib);
	return;
    }

    /* Vertex buffer setup */
    vtx_res.id              = SQ_VTX_RESOURCE_vs;
    vtx_res.vtx_size_dw     = 12 / 4;
    vtx_res.vtx_num_entries = accel_state->vb_index * 12 / 4;
    vtx_res.mem_req_size    = 1;
    vtx_res.vb_addr         = vb_addr;
    set_vtx_resource        (pScrn, accel_state->ib, &vtx_res);

    /* Draw */
    draw_conf.prim_type          = DI_PT_RECTLIST;
    draw_conf.vgt_draw_initiator = DI_SRC_SEL_AUTO_INDEX;
    draw_conf.num_instances      = 1;
    draw_conf.num_indices        = vtx_res.vtx_num_entries / vtx_res.vtx_size_dw;
    draw_conf.index_type         = DI_INDEX_SIZE_16_BIT;

    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_0,            0);	/* ? */
    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_1,            0);

    ereg  (accel_state->ib, VGT_MAX_VTX_INDX,                    draw_conf.num_indices);
    ereg  (accel_state->ib, VGT_MIN_VTX_INDX,                    0);
    ereg  (accel_state->ib, VGT_INDX_OFFSET,                     0);

    draw_auto(pScrn, accel_state->ib, &draw_conf);

    wait_3d_idle_clean(pScrn, accel_state->ib);

    R600CPFlushIndirect(pScrn, accel_state->ib);
}

static Bool
R600DoPrepareCopy(ScrnInfoPtr pScrn,
		  int src_pitch, int src_width, int src_height, uint32_t src_offset, int src_bpp,
		  int dst_pitch, int dst_height, uint32_t dst_offset, int dst_bpp,
		  int rop, Pixel planemask)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    int pmask = 0;
    int i = 0;
    uint32_t vs[16];
    uint32_t ps[8];
    cb_config_t     cb_conf;
    tex_resource_t  tex_res;
    tex_sampler_t   tex_samp;
    shader_config_t vs_conf, ps_conf;
    uint64_t vs_addr, ps_addr;

    //0
    vs[i++] = CF_DWORD0(ADDR(4));
    vs[i++] = CF_DWORD1(POP_COUNT(0),
			CF_CONST(0),
			COND(SQ_CF_COND_ACTIVE),
			I_COUNT(2),
			CALL_COUNT(0),
			END_OF_PROGRAM(0),
			VALID_PIXEL_MODE(0),
			CF_INST(SQ_CF_INST_VTX),
			WHOLE_QUAD_MODE(0),
			BARRIER(1));
    //1
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_POS0),
				      TYPE(SQ_EXPORT_POS),
				      RW_GPR(1),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(0));
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(0),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(1));
    //2
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(0),
				      TYPE(SQ_EXPORT_PARAM),
				      RW_GPR(0),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(0));
    vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(0),
					   END_OF_PROGRAM(1),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(0));
    //3
    vs[i++] = 0x00000000;
    vs[i++] = 0x00000000;
    //4/5
    vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			 FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			 FETCH_WHOLE_QUAD(0),
			 BUFFER_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 SRC_SEL_X(SQ_SEL_X),
			 MEGA_FETCH_COUNT(16));
    vs[i++] = VTX_DWORD1_GPR(DST_GPR(1),
			     DST_REL(0),
			     DST_SEL_X(SQ_SEL_X),
			     DST_SEL_Y(SQ_SEL_Y),
			     DST_SEL_Z(SQ_SEL_0),
			     DST_SEL_W(SQ_SEL_1),
			     USE_CONST_FIELDS(0),
			     DATA_FORMAT(FMT_32_32_FLOAT), //xxx
			     NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
			     FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
			     SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
    vs[i++] = VTX_DWORD2(OFFSET(0),
			 ENDIAN_SWAP(ENDIAN_NONE),
			 CONST_BUF_NO_STRIDE(0),
			 MEGA_FETCH(1));
    vs[i++] = VTX_DWORD_PAD;
    //6/7
    vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			 FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			 FETCH_WHOLE_QUAD(0),
			 BUFFER_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 SRC_SEL_X(SQ_SEL_X),
			 MEGA_FETCH_COUNT(8));
    vs[i++] = VTX_DWORD1_GPR(DST_GPR(0),
			     DST_REL(0),
			     DST_SEL_X(SQ_SEL_X),
			     DST_SEL_Y(SQ_SEL_Y),
			     DST_SEL_Z(SQ_SEL_0),
			     DST_SEL_W(SQ_SEL_1),
			     USE_CONST_FIELDS(0),
			     DATA_FORMAT(FMT_32_32_FLOAT), //xxx
			     NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
			     FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
			     SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
    vs[i++] = VTX_DWORD2(OFFSET(8),
			 ENDIAN_SWAP(ENDIAN_NONE),
			 CONST_BUF_NO_STRIDE(0),
			 MEGA_FETCH(0));
    vs[i++] = VTX_DWORD_PAD;

    i = 0;

    // CF INST 0
    ps[i++] = CF_DWORD0(ADDR(2));
    ps[i++] = CF_DWORD1(POP_COUNT(0),
			CF_CONST(0),
			COND(SQ_CF_COND_ACTIVE),
			I_COUNT(1),
			CALL_COUNT(0),
			END_OF_PROGRAM(0),
			VALID_PIXEL_MODE(0),
			CF_INST(SQ_CF_INST_TEX),
			WHOLE_QUAD_MODE(0),
			BARRIER(1));
    // CF INST 1
    ps[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_PIXEL_MRT0),
				      TYPE(SQ_EXPORT_PIXEL),
				      RW_GPR(0),
				      RW_REL(ABSOLUTE),
				      INDEX_GPR(0),
				      ELEM_SIZE(1));
    ps[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					   SRC_SEL_Y(SQ_SEL_Y),
					   SRC_SEL_Z(SQ_SEL_Z),
					   SRC_SEL_W(SQ_SEL_W),
					   R6xx_ELEM_LOOP(0),
					   BURST_COUNT(1),
					   END_OF_PROGRAM(1),
					   VALID_PIXEL_MODE(0),
					   CF_INST(SQ_CF_INST_EXPORT_DONE),
					   WHOLE_QUAD_MODE(0),
					   BARRIER(1));
    // TEX INST 0
    ps[i++] = TEX_DWORD0(TEX_INST(SQ_TEX_INST_SAMPLE),
			 BC_FRAC_MODE(0),
			 FETCH_WHOLE_QUAD(0),
			 RESOURCE_ID(0),
			 SRC_GPR(0),
			 SRC_REL(ABSOLUTE),
			 R7xx_ALT_CONST(0));
    ps[i++] = TEX_DWORD1(DST_GPR(0),
			 DST_REL(ABSOLUTE),
			 DST_SEL_X(SQ_SEL_X),
			 DST_SEL_Y(SQ_SEL_Y),
			 DST_SEL_Z(SQ_SEL_Z),
			 DST_SEL_W(SQ_SEL_W),
			 LOD_BIAS(0),
			 COORD_TYPE_X(TEX_UNNORMALIZED),
			 COORD_TYPE_Y(TEX_UNNORMALIZED),
			 COORD_TYPE_Z(TEX_UNNORMALIZED),
			 COORD_TYPE_W(TEX_UNNORMALIZED));
    ps[i++] = TEX_DWORD2(OFFSET_X(0),
			 OFFSET_Y(0),
			 OFFSET_Z(0),
			 SAMPLER_ID(0),
			 SRC_SEL_X(SQ_SEL_X),
			 SRC_SEL_Y(SQ_SEL_Y),
			 SRC_SEL_Z(SQ_SEL_0),
			 SRC_SEL_W(SQ_SEL_1));

    CLEAR (cb_conf);
    CLEAR (tex_res);
    CLEAR (tex_samp);
    CLEAR (vs_conf);
    CLEAR (ps_conf);

    accel_state->ib = RHDDRMCPBuffer(pScrn->scrnIndex);

    /* Init */
    start_3d(pScrn, accel_state->ib);

    cp_set_surface_sync(pScrn, accel_state->ib);

    set_default_state(pScrn, accel_state->ib);

    /* Scissor / viewport */
    ereg  (accel_state->ib, PA_CL_VTE_CNTL,                      VTX_XY_FMT_bit);
    ereg  (accel_state->ib, PA_CL_CLIP_CNTL,                     CLIP_DISABLE_bit);

    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 512, vs, sizeof(vs));
    vs_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 512;
    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 256, ps, sizeof(ps));
    ps_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 256;

    /* Shader */
    vs_conf.shader_addr         = vs_addr;
    vs_conf.num_gprs            = 2;
    vs_conf.stack_size          = 0;
    vs_setup                    (pScrn, accel_state->ib, &vs_conf);

    ps_conf.shader_addr         = ps_addr;
    ps_conf.num_gprs            = 1;
    ps_conf.stack_size          = 0;
    ps_conf.uncached_first_inst = 1;
    ps_conf.clamp_consts        = 0;
    ps_conf.export_mode         = 2;
    ps_setup                    (pScrn, accel_state->ib, &ps_conf);


    /* Texture */
    tex_res.id                  = 0;
    tex_res.w                   = src_width;
    tex_res.h                   = src_height;
    tex_res.pitch               = src_pitch;
    tex_res.depth               = 0;
    tex_res.dim                 = SQ_TEX_DIM_2D;
    tex_res.base                = src_offset;
    tex_res.mip_base            = src_offset;
    if (src_bpp == 8) {
	tex_res.format              = FMT_8;
	tex_res.dst_sel_x           = SQ_SEL_X;
	tex_res.dst_sel_y           = SQ_SEL_0;
	tex_res.dst_sel_z           = SQ_SEL_0;
	tex_res.dst_sel_w           = SQ_SEL_0;
    } else if (src_bpp == 16) {
	tex_res.format              = FMT_5_6_5;
	tex_res.dst_sel_x           = SQ_SEL_X;
	tex_res.dst_sel_y           = SQ_SEL_Y;
	tex_res.dst_sel_z           = SQ_SEL_Z;
	tex_res.dst_sel_w           = SQ_SEL_W;
    } else {
	tex_res.format              = FMT_8_8_8_8;
	tex_res.dst_sel_x           = SQ_SEL_X;
	tex_res.dst_sel_y           = SQ_SEL_Y;
	tex_res.dst_sel_z           = SQ_SEL_Z;
	tex_res.dst_sel_w           = SQ_SEL_W;
    }

    tex_res.request_size        = 1;
    tex_res.base_level          = 0;
    tex_res.last_level          = 0;
    tex_res.perf_modulation     = 0;
    set_tex_resource            (pScrn, accel_state->ib, &tex_res);

    tex_samp.id                 = 0;
    tex_samp.clamp_x            = SQ_TEX_CLAMP_LAST_TEXEL;
    tex_samp.clamp_y            = SQ_TEX_CLAMP_LAST_TEXEL;
    tex_samp.clamp_z            = SQ_TEX_WRAP;
    tex_samp.xy_mag_filter      = SQ_TEX_XY_FILTER_POINT;
    tex_samp.xy_min_filter      = SQ_TEX_XY_FILTER_POINT;
    tex_samp.z_filter           = SQ_TEX_Z_FILTER_NONE;
    tex_samp.mip_filter         = 0;			/* no mipmap */
    set_tex_sampler             (pScrn, accel_state->ib, &tex_samp);


    /* Render setup */
    if (planemask & 0x000000ff)
	pmask |= 1;
    if (planemask & 0x0000ff00)
	pmask |= 2;
    if (planemask & 0x00ff0000)
	pmask |= 4;
    if (planemask & 0xff000000)
	pmask |= 8;
    ereg  (accel_state->ib, CB_SHADER_MASK,                      (pmask << OUTPUT0_ENABLE_shift));
    ereg  (accel_state->ib, R7xx_CB_SHADER_CONTROL,              (RT0_ENABLE_bit));
    ereg  (accel_state->ib, CB_COLOR_CONTROL,                    RADEON_ROP[rop]);

    cb_conf.id = 0;
    cb_conf.w = dst_pitch;
    cb_conf.h = dst_height;
    cb_conf.base = dst_offset;
    if (dst_bpp == 8)
	cb_conf.format = COLOR_8;
    else if (dst_bpp == 16)
	cb_conf.format = COLOR_5_6_5;
    else
	cb_conf.format = COLOR_8_8_8_8;
    cb_conf.comp_swap = 0;
    cb_conf.source_format = 1;
    cb_conf.blend_clamp = 1;
    set_render_target(pScrn, accel_state->ib, &cb_conf);

    ereg  (accel_state->ib, PA_SU_SC_MODE_CNTL,                  (FACE_bit			|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_FRONT_PTYPE_shift)	|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_BACK_PTYPE_shift)));
    ereg  (accel_state->ib, DB_SHADER_CONTROL,                   ((1 << Z_ORDER_shift)		| /* EARLY_Z_THEN_LATE_Z */
						 DUAL_EXPORT_ENABLE_bit)); /* Only useful if no depth export */

    /* Interpolator setup */
    // export tex coord from VS
    ereg  (accel_state->ib, SPI_VS_OUT_CONFIG, ((1 - 1) << VS_EXPORT_COUNT_shift));
    ereg  (accel_state->ib, SPI_VS_OUT_ID_0, (0 << SEMANTIC_0_shift));

    /* Enabling flat shading needs both FLAT_SHADE_bit in SPI_PS_INPUT_CNTL_x
     * *and* FLAT_SHADE_ENA_bit in SPI_INTERP_CONTROL_0 */
    // input tex coord from VS
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_0,                 ((1 << NUM_INTERP_shift)));
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_1,                 0);
    // color semantic id 0 -> GPR[0]
    ereg  (accel_state->ib, SPI_PS_INPUT_CNTL_0 + (0 <<2),       ((0    << SEMANTIC_shift)	|
								  (0x01 << DEFAULT_VAL_shift)	|
								  SEL_CENTROID_bit));
    ereg  (accel_state->ib, SPI_INTERP_CONTROL_0,                0);

    accel_state->vb_index = 0;

}

static void
R600DoCopy(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    draw_config_t   draw_conf;
    vtx_resource_t  vtx_res;
    uint64_t vb_addr;

    vb_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2);

    CLEAR (draw_conf);
    CLEAR (vtx_res);

    if (accel_state->vb_index == 0) {
	R600CPFlushIndirect(pScrn, accel_state->ib);
	return;
    }

    /* Vertex buffer setup */
    vtx_res.id              = SQ_VTX_RESOURCE_vs;
    vtx_res.vtx_size_dw     = 16 / 4;
    vtx_res.vtx_num_entries = accel_state->vb_index * 16 / 4;
    vtx_res.mem_req_size    = 1;
    vtx_res.vb_addr         = vb_addr;
    set_vtx_resource        (pScrn, accel_state->ib, &vtx_res);

    draw_conf.prim_type          = DI_PT_RECTLIST;
    draw_conf.vgt_draw_initiator = DI_SRC_SEL_AUTO_INDEX;
    draw_conf.num_instances      = 1;
    draw_conf.num_indices        = vtx_res.vtx_num_entries / vtx_res.vtx_size_dw;
    draw_conf.index_type         = DI_INDEX_SIZE_16_BIT;

    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_0,            0);	/* ? */
    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_1,            0);

    ereg  (accel_state->ib, VGT_MAX_VTX_INDX,                    draw_conf.num_indices);
    ereg  (accel_state->ib, VGT_MIN_VTX_INDX,                    0);
    ereg  (accel_state->ib, VGT_INDX_OFFSET,                     0);

    draw_auto(pScrn, accel_state->ib, &draw_conf);

    wait_3d_idle_clean(pScrn, accel_state->ib);

    R600CPFlushIndirect(pScrn, accel_state->ib);
}

static void
R600AppendCopyVertex(ScrnInfoPtr pScrn,
		     int srcX, int srcY,
		     int dstX, int dstY,
		     int w, int h)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    struct r6xx_copy_vertex *copy_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));
    struct r6xx_copy_vertex vertex[3];

    vertex[0].x = (float)dstX;
    vertex[0].y = (float)dstY;
    vertex[0].s = (float)srcX;
    vertex[0].t = (float)srcY;

    vertex[1].x = (float)dstX;
    vertex[1].y = (float)(dstY + h);
    vertex[1].s = (float)srcX;
    vertex[1].t = (float)(srcY + h);

    vertex[2].x = (float)(dstX + w);
    vertex[2].y = (float)(dstY + h);
    vertex[2].s = (float)(srcX + w);
    vertex[2].t = (float)(srcY + h);

#ifdef SHOW_VERTEXES
    ErrorF("vertex 0: %f, %f, %f, %d\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
    ErrorF("vertex 1: %f, %f, %f, %d\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
    ErrorF("vertex 2: %f, %f, %f, %d\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);
#endif

    // append to vertex buffer
    copy_vb[accel_state->vb_index++] = vertex[0];
    copy_vb[accel_state->vb_index++] = vertex[1];
    copy_vb[accel_state->vb_index++] = vertex[2];

}

static Bool
R600PrepareCopy(PixmapPtr pSrc,   PixmapPtr pDst,
		int xdir, int ydir,
		int rop,
		Pixel planemask)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    int src_pitch, dst_pitch;
    uint32_t src_offset, dst_offset;

    dst_pitch = exaGetPixmapPitch(pDst) / (pDst->drawable.bitsPerPixel / 8);
    src_pitch = exaGetPixmapPitch(pSrc) / (pSrc->drawable.bitsPerPixel / 8);

    src_offset = exaGetPixmapOffset(pSrc) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;
    dst_offset = exaGetPixmapOffset(pDst) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    // bad pitch
    if (src_pitch & 63)
	return FALSE;
    if (dst_pitch & 63)
	return FALSE;

    // bad offset
    if (src_offset & 0xff)
	return FALSE;
    if (dst_offset & 0xff)
	return FALSE;

    if (pSrc->drawable.bitsPerPixel == 24)
	return FALSE;
    if (pDst->drawable.bitsPerPixel == 24)
	return FALSE;

    //return FALSE;

#ifdef SHOW_VERTEXES
    ErrorF("src: %dx%d @ %dbpp, 0x%08x\n", pSrc->drawable.width, pSrc->drawable.height,
	   pSrc->drawable.bitsPerPixel, exaGetPixmapPitch(pSrc));
    ErrorF("dst: %dx%d @ %dbpp, 0x%08x\n", pDst->drawable.width, pDst->drawable.height,
	   pDst->drawable.bitsPerPixel, exaGetPixmapPitch(pDst));
#endif

    if (exaGetPixmapOffset(pSrc) == exaGetPixmapOffset(pDst)) {
	accel_state->same_surface = TRUE;
	accel_state->rop = rop;
	accel_state->planemask = planemask;

#ifdef SHOW_VERTEXES
	ErrorF("same surface!\n");
#endif
    } else {

	accel_state->same_surface = FALSE;

	R600DoPrepareCopy(pScrn,
			  src_pitch, pSrc->drawable.width, pSrc->drawable.height, src_offset, pSrc->drawable.bitsPerPixel,
			  dst_pitch, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
			  rop, planemask);

    }

    return TRUE;
}

static Bool
is_overlap(int sx1, int sx2, int sy1, int sy2, int dx1, int dx2, int dy1, int dy2)
{
    if (((sx1 >= dx1) && (sx1 <= dx2) && (sy1 >= dy1) && (sy1 <= dy2)) || // TL x1, y1
	((sx2 >= dx1) && (sx2 <= dx2) && (sy1 >= dy1) && (sy1 <= dy2)) || // TR x2, y1
	((sx1 >= dx1) && (sx1 <= dx2) && (sy2 >= dy1) && (sy2 <= dy2)) || // BL x1, y2
	((sx2 >= dx1) && (sx2 <= dx2) && (sy2 >= dy1) && (sy2 <= dy2)))   // BR x2, y2
	return TRUE;
    else
	return FALSE;
}

static void
R600OverlapCopy(PixmapPtr pDst,
		int srcX, int srcY,
		int dstX, int dstY,
		int w, int h)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    uint32_t dst_pitch = exaGetPixmapPitch(pDst) / (pDst->drawable.bitsPerPixel / 8);
    uint32_t dst_offset = exaGetPixmapOffset(pDst) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;
    struct r6xx_copy_vertex *copy_vb;
    struct r6xx_copy_vertex vertex[3];
    int i;

    if (is_overlap(srcX, srcX + w, srcY, srcY + h,
		   dstX, dstX + w, dstY, dstY + h)) {
	if (srcY == dstY) { // left/right
	    if (srcX < dstX) { // right
		// copy right to left
		for (i = w; i > 0; i--) {
		    R600DoPrepareCopy(pScrn,
				      dst_pitch, pDst->drawable.width, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      dst_pitch, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      accel_state->rop, accel_state->planemask);

		    copy_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));

		    vertex[0].x = (float)(dstX + i - 1);
		    vertex[0].y = (float)dstY;
		    vertex[0].s = (float)(srcX + i - 1);
		    vertex[0].t = (float)srcY;

		    vertex[1].x = (float)(dstX + i - 1);
		    vertex[1].y = (float)(dstY + h);
		    vertex[1].s = (float)(srcX + i - 1);
		    vertex[1].t = (float)(srcY + h);

		    vertex[2].x = (float)(dstX + i);
		    vertex[2].y = (float)(dstY + h);
		    vertex[2].s = (float)(srcX + i);
		    vertex[2].t = (float)(srcY + h);

#ifdef SHOW_VERTEXES
		    ErrorF("vertex 0: %f, %f, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
		    ErrorF("vertex 1: %f, %f, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
		    ErrorF("vertex 2: %f, %f, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);
#endif

		    // append to vertex buffer
		    copy_vb[accel_state->vb_index++] = vertex[0];
		    copy_vb[accel_state->vb_index++] = vertex[1];
		    copy_vb[accel_state->vb_index++] = vertex[2];

		    // do the blit
		    R600DoCopy(pScrn);
		}
	    } else { //left
		// copy left to right
		for (i = 0; i < w; i++) {
		    R600DoPrepareCopy(pScrn,
				      dst_pitch, pDst->drawable.width, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      dst_pitch, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      accel_state->rop, accel_state->planemask);

		    copy_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));

		    vertex[0].x = (float)(dstX + i);
		    vertex[0].y = (float)(dstY);
		    vertex[0].s = (float)(srcX + i);
		    vertex[0].t = (float)srcY;

		    vertex[1].x = (float)(dstX + i);
		    vertex[1].y = (float)(dstY + h);
		    vertex[1].s = (float)(srcX + i);
		    vertex[1].t = (float)(srcY + h);

		    vertex[2].x = (float)(dstX + i + 1);
		    vertex[2].y = (float)(dstY + h);
		    vertex[2].s = (float)(srcX + i + 1);
		    vertex[2].t = (float)(srcY + h);

#ifdef SHOW_VERTEXES
		    ErrorF("vertex 0: %f, %f, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
		    ErrorF("vertex 1: %f, %f, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
		    ErrorF("vertex 2: %f, %f, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);
#endif

		    // append to vertex buffer
		    copy_vb[accel_state->vb_index++] = vertex[0];
		    copy_vb[accel_state->vb_index++] = vertex[1];
		    copy_vb[accel_state->vb_index++] = vertex[2];

		    // do the blit
		    R600DoCopy(pScrn);
		}
	    }
	} else { //up/down
	    if (srcY > dstY) { // up
		// copy top to bottom
		for (i = 0; i < h; i++) {
		    R600DoPrepareCopy(pScrn,
				      dst_pitch, pDst->drawable.width, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      dst_pitch, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      accel_state->rop, accel_state->planemask);

		    copy_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));

		    vertex[0].x = (float)dstX;
		    vertex[0].y = (float)(dstY + i);
		    vertex[0].s = (float)srcX;
		    vertex[0].t = (float)(srcY + i);

		    vertex[1].x = (float)dstX;
		    vertex[1].y = (float)(dstY + i + 1);
		    vertex[1].s = (float)srcX;
		    vertex[1].t = (float)(srcY + i + 1);

		    vertex[2].x = (float)(dstX + w);
		    vertex[2].y = (float)(dstY + i + 1);
		    vertex[2].s = (float)(srcX + w);
		    vertex[2].t = (float)(srcY + i + 1);

#ifdef SHOW_VERTEXES
		    ErrorF("vertex 0: %f, %f, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
		    ErrorF("vertex 1: %f, %f, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
		    ErrorF("vertex 2: %f, %f, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);
#endif

		    // append to vertex buffer
		    copy_vb[accel_state->vb_index++] = vertex[0];
		    copy_vb[accel_state->vb_index++] = vertex[1];
		    copy_vb[accel_state->vb_index++] = vertex[2];

		    // do the blit
		    R600DoCopy(pScrn);
		}
	    } else { // down
		// copy bottom to top
		for (i = h; i > 0; i--) {
		    R600DoPrepareCopy(pScrn,
				      dst_pitch, pDst->drawable.width, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      dst_pitch, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
				      accel_state->rop, accel_state->planemask);

		    copy_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));

		    vertex[0].x = (float)dstX;
		    vertex[0].y = (float)(dstY + i - 1);
		    vertex[0].s = (float)(srcX);
		    vertex[0].t = (float)(srcY + i - 1);

		    vertex[1].x = (float)dstX;
		    vertex[1].y = (float)(dstY + i);
		    vertex[1].s = (float)srcX;
		    vertex[1].t = (float)srcY + i;

		    vertex[2].x = (float)(dstX + w);
		    vertex[2].y = (float)(dstY + i);
		    vertex[2].s = (float)(srcX + w);
		    vertex[2].t = (float)(srcY + i);

#ifdef SHOW_VERTEXES
		    ErrorF("vertex 0: %f, %f, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
		    ErrorF("vertex 1: %f, %f, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
		    ErrorF("vertex 2: %f, %f, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);
#endif

		    // append to vertex buffer
		    copy_vb[accel_state->vb_index++] = vertex[0];
		    copy_vb[accel_state->vb_index++] = vertex[1];
		    copy_vb[accel_state->vb_index++] = vertex[2];

		    // do the blit
		    R600DoCopy(pScrn);
		}
	    }
	}
    } else {
	R600DoPrepareCopy(pScrn,
			  dst_pitch, pDst->drawable.width, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
			  dst_pitch, pDst->drawable.height, dst_offset, pDst->drawable.bitsPerPixel,
			  accel_state->rop, accel_state->planemask);

	copy_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));

	vertex[0].x = (float)dstX;
	vertex[0].y = (float)dstY;
	vertex[0].s = (float)srcX;
	vertex[0].t = (float)srcY;

	vertex[1].x = (float)dstX;
	vertex[1].y = (float)(dstY + h);
	vertex[1].s = (float)srcX;
	vertex[1].t = (float)(srcY + h);

	vertex[2].x = (float)(dstX + w);
	vertex[2].y = (float)(dstY + h);
	vertex[2].s = (float)(srcX + w);
	vertex[2].t = (float)(srcY + h);

#ifdef SHOW_VERTEXES
	ErrorF("vertex 0: %f, %f, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].s, vertex[0].t);
	ErrorF("vertex 1: %f, %f, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].s, vertex[1].t);
	ErrorF("vertex 2: %f, %f, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].s, vertex[2].t);
#endif

	// append to vertex buffer
	copy_vb[accel_state->vb_index++] = vertex[0];
	copy_vb[accel_state->vb_index++] = vertex[1];
	copy_vb[accel_state->vb_index++] = vertex[2];

	// do the blit
	R600DoCopy(pScrn);
    }
}

static void
R600Copy(PixmapPtr pDst,
	 int srcX, int srcY,
	 int dstX, int dstY,
	 int w, int h)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;

    //blit to/from same surfacce
    if (accel_state->same_surface)
	R600OverlapCopy(pDst, srcX, srcY, dstX, dstY, w, h);
    else
	R600AppendCopyVertex(pScrn, srcX, srcY, dstX, dstY, w, h);
}

static void
R600DoneCopy(PixmapPtr pDst)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;

    if (accel_state->same_surface)
	return;
    else
	R600DoCopy(pScrn);
}

#define RADEON_TRACE_FALL 0
#define RADEON_TRACE_DRAW 0

#if RADEON_TRACE_FALL
#define RADEON_FALLBACK(x)     		\
do {					\
	ErrorF("%s: ", __FUNCTION__);	\
	ErrorF x;			\
	return FALSE;			\
} while (0)
#else
#define RADEON_FALLBACK(x) return FALSE
#endif

#define xFixedToFloat(f) (((float) (f)) / 65536)

static inline void transformPoint(PictTransform *transform, xPointFixed *point)
{
    PictVector v;
    v.vector[0] = point->x;
    v.vector[1] = point->y;
    v.vector[2] = xFixed1;
    PictureTransformPoint(transform, &v);
    point->x = v.vector[0];
    point->y = v.vector[1];
}

struct blendinfo {
    Bool dst_alpha;
    Bool src_alpha;
    uint32_t blend_cntl;
};

static struct blendinfo R600BlendOp[] = {
    /* Clear */
    {0, 0, (BLEND_ZERO << COLOR_SRCBLEND_shift) | (BLEND_ZERO << COLOR_DESTBLEND_shift)},
    /* Src */
    {0, 0, (BLEND_ONE << COLOR_SRCBLEND_shift) | (BLEND_ZERO << COLOR_DESTBLEND_shift)},
    /* Dst */
    {0, 0, (BLEND_ZERO << COLOR_SRCBLEND_shift) | (BLEND_ONE << COLOR_DESTBLEND_shift)},
    /* Over */
    {0, 1, (BLEND_ONE << COLOR_SRCBLEND_shift) | (BLEND_ONE_MINUS_SRC_ALPHA << COLOR_DESTBLEND_shift)},
    /* OverReverse */
    {1, 0, (BLEND_ONE_MINUS_DST_ALPHA << COLOR_SRCBLEND_shift) | (BLEND_ONE << COLOR_DESTBLEND_shift)},
    /* In */
    {1, 0, (BLEND_DST_ALPHA << COLOR_SRCBLEND_shift) | (BLEND_ZERO << COLOR_DESTBLEND_shift)},
    /* InReverse */
    {0, 1, (BLEND_ZERO << COLOR_SRCBLEND_shift) | (BLEND_SRC_ALPHA << COLOR_DESTBLEND_shift)},
    /* Out */
    {1, 0, (BLEND_ONE_MINUS_DST_ALPHA << COLOR_SRCBLEND_shift) | (BLEND_ZERO << COLOR_DESTBLEND_shift)},
    /* OutReverse */
    {0, 1, (BLEND_ZERO << COLOR_SRCBLEND_shift) | (BLEND_ONE_MINUS_SRC_ALPHA << COLOR_DESTBLEND_shift)},
    /* Atop */
    {1, 1, (BLEND_DST_ALPHA << COLOR_SRCBLEND_shift) | (BLEND_ONE_MINUS_SRC_ALPHA << COLOR_DESTBLEND_shift)},
    /* AtopReverse */
    {1, 1, (BLEND_ONE_MINUS_DST_ALPHA << COLOR_SRCBLEND_shift) | (BLEND_SRC_ALPHA << COLOR_DESTBLEND_shift)},
    /* Xor */
    {1, 1, (BLEND_ONE_MINUS_DST_ALPHA << COLOR_SRCBLEND_shift) | (BLEND_ONE_MINUS_SRC_ALPHA << COLOR_DESTBLEND_shift)},
    /* Add */
    {0, 0, (BLEND_ONE << COLOR_SRCBLEND_shift) | (BLEND_ONE << COLOR_DESTBLEND_shift)},
};

struct formatinfo {
    unsigned int fmt;
    uint32_t card_fmt;
};

static struct formatinfo R600TexFormats[] = {
    {PICT_a8r8g8b8,	FMT_8_8_8_8},
    {PICT_x8r8g8b8,	FMT_8_8_8_8},
    {PICT_a8b8g8r8,	FMT_8_8_8_8},
    {PICT_x8b8g8r8,	FMT_8_8_8_8},
    {PICT_r5g6b5,	FMT_5_6_5},
    {PICT_a1r5g5b5,	FMT_1_5_5_5},
    {PICT_x1r5g5b5,     FMT_1_5_5_5},
    {PICT_a8,		FMT_8},
};

static uint32_t R600GetBlendCntl(int op, PicturePtr pMask, uint32_t dst_format)
{
    uint32_t sblend, dblend;

    sblend = R600BlendOp[op].blend_cntl & COLOR_SRCBLEND_mask;
    dblend = R600BlendOp[op].blend_cntl & COLOR_DESTBLEND_mask;

    /* If there's no dst alpha channel, adjust the blend op so that we'll treat
     * it as always 1.
     */
    if (PICT_FORMAT_A(dst_format) == 0 && R600BlendOp[op].dst_alpha) {
	if (sblend == (BLEND_DST_ALPHA << COLOR_SRCBLEND_shift))
	    sblend = (BLEND_ONE << COLOR_SRCBLEND_shift);
	else if (sblend == (BLEND_ONE_MINUS_DST_ALPHA << COLOR_SRCBLEND_shift))
	    sblend = (BLEND_ZERO << COLOR_SRCBLEND_shift);
    }

    /* If the source alpha is being used, then we should only be in a case where
     * the source blend factor is 0, and the source blend value is the mask
     * channels multiplied by the source picture's alpha.
     */
    if (pMask && pMask->componentAlpha && R600BlendOp[op].src_alpha) {
	if (dblend == (BLEND_SRC_ALPHA << COLOR_DESTBLEND_shift)) {
	    dblend = (BLEND_SRC_COLOR << COLOR_DESTBLEND_shift);
	} else if (dblend == (BLEND_ONE_MINUS_SRC_ALPHA << COLOR_DESTBLEND_shift)) {
	    dblend = (BLEND_ONE_MINUS_SRC_COLOR << COLOR_DESTBLEND_shift);
	}
    }

    return sblend | dblend;
}

static Bool R600GetDestFormat(PicturePtr pDstPicture, uint32_t *dst_format)
{
    switch (pDstPicture->format) {
    case PICT_a8r8g8b8:
    case PICT_x8r8g8b8:
	*dst_format = COLOR_8_8_8_8;
	break;
    case PICT_r5g6b5:
	*dst_format = COLOR_5_6_5;
	break;
    case PICT_a1r5g5b5:
    case PICT_x1r5g5b5:
	*dst_format = COLOR_1_5_5_5;
	break;
    case PICT_a8:
	*dst_format = COLOR_8;
	break;
    default:
	RADEON_FALLBACK(("Unsupported dest format 0x%x\n",
	       (int)pDstPicture->format));
    }
    return TRUE;
}

static Bool R600CheckCompositeTexture(PicturePtr pPict,
				      PicturePtr pDstPict,
				      int op,
				      int unit)
{
    int w = pPict->pDrawable->width;
    int h = pPict->pDrawable->height;
    unsigned int i;
    int max_tex_w, max_tex_h;

    max_tex_w = 8192;
    max_tex_h = 8192;

    if ((w > max_tex_w) || (h > max_tex_h))
	RADEON_FALLBACK(("Picture w/h too large (%dx%d)\n", w, h));

    for (i = 0; i < sizeof(R600TexFormats) / sizeof(R600TexFormats[0]); i++) {
	if (R600TexFormats[i].fmt == pPict->format)
	    break;
    }
    if (i == sizeof(R600TexFormats) / sizeof(R600TexFormats[0]))
	RADEON_FALLBACK(("Unsupported picture format 0x%x\n",
			 (int)pPict->format));

    //fixme
    //if (!RADEONCheckTexturePOT(pPict, unit == 0))
    //return FALSE;

    if (pPict->filter != PictFilterNearest &&
	pPict->filter != PictFilterBilinear)
	RADEON_FALLBACK(("Unsupported filter 0x%x\n", pPict->filter));

    /* for REPEAT_NONE, Render semantics are that sampling outside the source
     * picture results in alpha=0 pixels. We can implement this with a border color
     * *if* our source texture has an alpha channel, otherwise we need to fall
     * back. If we're not transformed then we hope that upper layers have clipped
     * rendering to the bounds of the source drawable, in which case it doesn't
     * matter. I have not, however, verified that the X server always does such
     * clipping.
     */
    //FIXME R6xx
    if (pPict->transform != 0 && !pPict->repeat && PICT_FORMAT_A(pPict->format) == 0) {
	if (!(((op == PictOpSrc) || (op == PictOpClear)) && (PICT_FORMAT_A(pDstPict->format) == 0)))
	    RADEON_FALLBACK(("REPEAT_NONE unsupported for transformed xRGB source\n"));
    }

    return TRUE;
}

static Bool R600TextureSetup(PicturePtr pPict, PixmapPtr pPix,
					int unit)
{
    ScrnInfoPtr pScrn = xf86Screens[pPix->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    int w = pPict->pDrawable->width;
    int h = pPict->pDrawable->height;
    unsigned int i;
    tex_resource_t  tex_res;
    tex_sampler_t   tex_samp;

    CLEAR (tex_res);
    CLEAR (tex_samp);

    for (i = 0; i < sizeof(R600TexFormats) / sizeof(R600TexFormats[0]); i++) {
	if (R600TexFormats[i].fmt == pPict->format)
	    break;
    }

    accel_state->texW[unit] = w;
    accel_state->texH[unit] = h;

    ErrorF("Tex %d setup %dx%d\n", unit, w, h);

    /* Texture */
    tex_res.id                  = unit;
    tex_res.w                   = w;
    tex_res.h                   = h;
    tex_res.pitch               = exaGetPixmapPitch(pPix) / (pPix->drawable.bitsPerPixel / 8);
    tex_res.depth               = 0;
    tex_res.dim                 = SQ_TEX_DIM_2D;
    tex_res.base                = exaGetPixmapOffset(pPix) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;
    tex_res.mip_base            = exaGetPixmapOffset(pPix) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;
    tex_res.format              = R600TexFormats[i].card_fmt;
    tex_res.request_size        = 1;

    /* component swizzles */
    // XXX double check these
    switch (pPict->format) {
    case PICT_a1r5g5b5:
    case PICT_a8r8g8b8:
	ErrorF("%s: PICT_a8r8g8b8\n", unit ? "mask" : "src");
	tex_res.dst_sel_x           = SQ_SEL_X;
	tex_res.dst_sel_y           = SQ_SEL_Y;
	tex_res.dst_sel_z           = SQ_SEL_Z;
	tex_res.dst_sel_w           = SQ_SEL_W;
	break;
    case PICT_a8b8g8r8:
	ErrorF("%s: PICT_a8b8g8r8\n", unit ? "mask" : "src");
	tex_res.dst_sel_x           = SQ_SEL_X;
	tex_res.dst_sel_y           = SQ_SEL_W;
	tex_res.dst_sel_z           = SQ_SEL_Z;
	tex_res.dst_sel_w           = SQ_SEL_Y;
	break;
    case PICT_x8b8g8r8:
	ErrorF("%s: PICT_x8b8g8r8\n", unit ? "mask" : "src");
	tex_res.dst_sel_x           = SQ_SEL_1;
	tex_res.dst_sel_y           = SQ_SEL_W;
	tex_res.dst_sel_z           = SQ_SEL_Z;
	tex_res.dst_sel_w           = SQ_SEL_Y;
	break;
    case PICT_x1r5g5b5:
    case PICT_x8r8g8b8:
	ErrorF("%s: PICT_x8r8g8b8\n", unit ? "mask" : "src");
	tex_res.dst_sel_x           = SQ_SEL_1;
	tex_res.dst_sel_y           = SQ_SEL_Y;
	tex_res.dst_sel_z           = SQ_SEL_Z;
	tex_res.dst_sel_w           = SQ_SEL_W;
	break;
    case PICT_r5g6b5:
	ErrorF("%s: PICT_r5g6b5\n", unit ? "mask" : "src");
	tex_res.dst_sel_x           = SQ_SEL_1;
	tex_res.dst_sel_y           = SQ_SEL_X;
	tex_res.dst_sel_z           = SQ_SEL_Y;
	tex_res.dst_sel_w           = SQ_SEL_Z;
	break;
    case PICT_a8:
	ErrorF("%s: PICT_a8\n", unit ? "mask" : "src");
	tex_res.dst_sel_x           = SQ_SEL_X;
	tex_res.dst_sel_y           = SQ_SEL_0;
	tex_res.dst_sel_z           = SQ_SEL_0;
	tex_res.dst_sel_w           = SQ_SEL_0;
	break;
    default:
	break;
    }

    tex_res.base_level          = 0;
    tex_res.last_level          = 0;
    tex_res.perf_modulation     = 0;
    set_tex_resource            (pScrn, accel_state->ib, &tex_res);

    tex_samp.id                 = unit;
    tex_samp.clamp_z            = SQ_TEX_WRAP;

    //fixme
    /*if (pPict->repeat && !(unit == 0 && accel_state->need_src_tile_x))
	tex_samp.clamp_x            = SQ_TEX_WRAP;
    else*/
	tex_samp.clamp_x            = SQ_TEX_CLAMP_LAST_TEXEL;

    /*if (pPict->repeat && !(unit == 0 && accel_state->need_src_tile_y))
	tex_samp.clamp_y            = SQ_TEX_WRAP;
    else*/
	tex_samp.clamp_y            = SQ_TEX_CLAMP_LAST_TEXEL;

    switch (pPict->filter) {
    case PictFilterNearest:
	tex_samp.xy_mag_filter      = SQ_TEX_XY_FILTER_POINT;
	tex_samp.xy_min_filter      = SQ_TEX_XY_FILTER_POINT;
	break;
    case PictFilterBilinear:
	tex_samp.xy_mag_filter      = SQ_TEX_XY_FILTER_BILINEAR;
	tex_samp.xy_min_filter      = SQ_TEX_XY_FILTER_BILINEAR;
	break;
    default:
	RADEON_FALLBACK(("Bad filter 0x%x\n", pPict->filter));
    }

    tex_samp.z_filter           = SQ_TEX_Z_FILTER_NONE;
    tex_samp.mip_filter         = 0;			/* no mipmap */
    set_tex_sampler             (pScrn, accel_state->ib, &tex_samp);

    // border color?

    if (pPict->transform != 0) {
	accel_state->is_transform[unit] = TRUE;
	accel_state->transform[unit] = pPict->transform;
    } else
	accel_state->is_transform[unit] = FALSE;

    return TRUE;
}

static Bool R600CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
			       PicturePtr pDstPicture)
{
    uint32_t tmp1;
//    ScreenPtr pScreen = pDstPicture->pDrawable->pScreen;
    PixmapPtr pSrcPixmap, pDstPixmap;
//    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
//    RHDPtr rhdPtr = RHDPTR(pScrn);
    int max_tex_w, max_tex_h, max_dst_w, max_dst_h;

    /* Check for unsupported compositing operations. */
    if (op >= (int) (sizeof(R600BlendOp) / sizeof(R600BlendOp[0])))
	RADEON_FALLBACK(("Unsupported Composite op 0x%x\n", op));

    pSrcPixmap = RADEONGetDrawablePixmap(pSrcPicture->pDrawable);

    max_tex_w = 8192;
    max_tex_h = 8192;
    max_dst_w = 8192;
    max_dst_h = 8192;

    if (pSrcPixmap->drawable.width >= max_tex_w ||
	pSrcPixmap->drawable.height >= max_tex_h) {
	RADEON_FALLBACK(("Source w/h too large (%d,%d).\n",
			 pSrcPixmap->drawable.width,
			 pSrcPixmap->drawable.height));
    }

    pDstPixmap = RADEONGetDrawablePixmap(pDstPicture->pDrawable);

    if (pDstPixmap->drawable.width >= max_dst_w ||
	pDstPixmap->drawable.height >= max_dst_h) {
	RADEON_FALLBACK(("Dest w/h too large (%d,%d).\n",
			 pDstPixmap->drawable.width,
			 pDstPixmap->drawable.height));
    }

    if (pMaskPicture) {
	PixmapPtr pMaskPixmap = RADEONGetDrawablePixmap(pMaskPicture->pDrawable);

	if (pMaskPixmap->drawable.width >= max_tex_w ||
	    pMaskPixmap->drawable.height >= max_tex_h) {
	    RADEON_FALLBACK(("Mask w/h too large (%d,%d).\n",
			     pMaskPixmap->drawable.width,
			     pMaskPixmap->drawable.height));
	}

	if (pMaskPicture->componentAlpha) {
	    /* Check if it's component alpha that relies on a source alpha and
	     * on the source value.  We can only get one of those into the
	     * single source value that we get to blend with.
	     */
	    if (R600BlendOp[op].src_alpha &&
		(R600BlendOp[op].blend_cntl & COLOR_SRCBLEND_mask) !=
		(BLEND_ZERO << COLOR_SRCBLEND_shift)) {
		RADEON_FALLBACK(("Component alpha not supported with source "
				 "alpha and source value blending.\n"));
	    }
	}

	if (!R600CheckCompositeTexture(pMaskPicture, pDstPicture, op, 1))
	    return FALSE;
    }

    if (!R600CheckCompositeTexture(pSrcPicture, pDstPicture, op, 0))
	return FALSE;

    if (!R600GetDestFormat(pDstPicture, &tmp1))
	return FALSE;

    return TRUE;

}

static Bool R600PrepareComposite(int op, PicturePtr pSrcPicture,
				 PicturePtr pMaskPicture, PicturePtr pDstPicture,
				 PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
    ScrnInfoPtr pScrn = xf86Screens[pSrc->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    uint32_t blendcntl, dst_format;
//    int src_color, src_alpha;
//    int mask_color, mask_alpha;
    uint32_t src_pitch, dst_pitch, src_offset, dst_offset;
    cb_config_t cb_conf;
    shader_config_t vs_conf, ps_conf;
    uint64_t vs_addr, ps_addr;
    int i = 0;
    uint32_t vs[20];
    uint32_t ps[24];

    //return FALSE;

    // no mask for now
    // XXX: FIX ME
    if (pMask)
	RADEON_FALLBACK(("Mask!\n"));

    if (pMask) {
	//0
	vs[i++] = CF_DWORD0(ADDR(4));
	vs[i++] = CF_DWORD1(POP_COUNT(0),
			    CF_CONST(0),
			    COND(SQ_CF_COND_ACTIVE),
			    I_COUNT(3),
			    CALL_COUNT(0),
			    END_OF_PROGRAM(0),
			    VALID_PIXEL_MODE(0),
			    CF_INST(SQ_CF_INST_VTX),
			    WHOLE_QUAD_MODE(0),
			    BARRIER(1));
	//1 - dst
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_POS0),
					  TYPE(SQ_EXPORT_POS),
					  RW_GPR(2),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(0));
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(0),
					       END_OF_PROGRAM(0),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT_DONE),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(1));
	//2 - src
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(0),
					  TYPE(SQ_EXPORT_PARAM),
					  RW_GPR(1),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(0));
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(0),
					       END_OF_PROGRAM(0),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(0));
	//3 - mask
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(0),
					  TYPE(SQ_EXPORT_PARAM),
					  RW_GPR(0),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(0));
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(0),
					       END_OF_PROGRAM(1),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT_DONE),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(0));
	//4/5 - dst
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(24));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(2),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_0),
				 DST_SEL_W(SQ_SEL_1),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_32_32_FLOAT), //xxx
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(0),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(1));
	vs[i++] = VTX_DWORD_PAD;
	//6/7 - src
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(8));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(1),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_0),
				 DST_SEL_W(SQ_SEL_1),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_32_32_FLOAT), //xxx
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(8),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(0));
	vs[i++] = VTX_DWORD_PAD;
	//8/9 - mask
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(8));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(0),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_0),
				 DST_SEL_W(SQ_SEL_1),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_32_32_FLOAT), //xxx
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(8),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(0));
	vs[i++] = VTX_DWORD_PAD;
    } else {
	//0
	vs[i++] = CF_DWORD0(ADDR(4));
	vs[i++] = CF_DWORD1(POP_COUNT(0),
			    CF_CONST(0),
			    COND(SQ_CF_COND_ACTIVE),
			    I_COUNT(2),
			    CALL_COUNT(0),
			    END_OF_PROGRAM(0),
			    VALID_PIXEL_MODE(0),
			    CF_INST(SQ_CF_INST_VTX),
			    WHOLE_QUAD_MODE(0),
			    BARRIER(1));
	//1 - dst
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_POS0),
					  TYPE(SQ_EXPORT_POS),
					  RW_GPR(1),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(0));
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(0),
					       END_OF_PROGRAM(0),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT_DONE),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(1));
	//2 - src
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(0),
					  TYPE(SQ_EXPORT_PARAM),
					  RW_GPR(0),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(0));
	vs[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(0),
					       END_OF_PROGRAM(1),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT_DONE),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(0));
	//3
	vs[i++] = 0x00000000;
	vs[i++] = 0x00000000;
	//4/5 - dst
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(16));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(1),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_0),
				 DST_SEL_W(SQ_SEL_1),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_32_32_FLOAT), //xxx
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(0),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(1));
	vs[i++] = VTX_DWORD_PAD;
	//6/7 - src
	vs[i++] = VTX_DWORD0(VTX_INST(SQ_VTX_INST_FETCH),
			     FETCH_TYPE(SQ_VTX_FETCH_VERTEX_DATA),
			     FETCH_WHOLE_QUAD(0),
			     BUFFER_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     SRC_SEL_X(SQ_SEL_X),
			     MEGA_FETCH_COUNT(8));
	vs[i++] = VTX_DWORD1_GPR(DST_GPR(0),
				 DST_REL(0),
				 DST_SEL_X(SQ_SEL_X),
				 DST_SEL_Y(SQ_SEL_Y),
				 DST_SEL_Z(SQ_SEL_0),
				 DST_SEL_W(SQ_SEL_1),
				 USE_CONST_FIELDS(0),
				 DATA_FORMAT(FMT_32_32_FLOAT), //xxx
				 NUM_FORMAT_ALL(SQ_NUM_FORMAT_NORM), //xxx
				 FORMAT_COMP_ALL(SQ_FORMAT_COMP_SIGNED), //xxx
				 SRF_MODE_ALL(SRF_MODE_ZERO_CLAMP_MINUS_ONE));
	vs[i++] = VTX_DWORD2(OFFSET(8),
			     ENDIAN_SWAP(ENDIAN_NONE),
			     CONST_BUF_NO_STRIDE(0),
			     MEGA_FETCH(0));
	vs[i++] = VTX_DWORD_PAD;
    }

    i = 0;

    if (pMask) {
	int src_a, src_r, src_g, src_b;
	int mask_a, mask_r, mask_g, mask_b;

	/* setup pixel shader */
	if (PICT_FORMAT_RGB(pSrcPicture->format) == 0) {
	    //src_color = R300_ALU_RGB_0_0;
	    src_r = SQ_SEL_0;
	    src_g = SQ_SEL_0;
	    src_b = SQ_SEL_0;
	} else {
	    //src_color = R300_ALU_RGB_SRC0_RGB;
	    src_r = SQ_SEL_Y;
	    src_g = SQ_SEL_Z;
	    src_b = SQ_SEL_W;
	}

	if (PICT_FORMAT_A(pSrcPicture->format) == 0) {
	    //src_alpha = R300_ALU_ALPHA_1_0;
	    src_a = SQ_SEL_1;
	} else {
	    //src_alpha = R300_ALU_ALPHA_SRC0_A;
	    src_a = SQ_SEL_X;
	}

	if (pMaskPicture->componentAlpha) {
	    if (R600BlendOp[op].src_alpha) {
		if (PICT_FORMAT_A(pSrcPicture->format) == 0) {
		    //src_color = R300_ALU_RGB_1_0;
		    //src_alpha = R300_ALU_ALPHA_1_0;
		    src_r = SQ_SEL_1;
		    src_g = SQ_SEL_1;
		    src_b = SQ_SEL_1;
		    src_a = SQ_SEL_1;
		} else {
		    //src_color = R300_ALU_RGB_SRC0_AAA;
		    //src_alpha = R300_ALU_ALPHA_SRC0_A;
		    src_r = SQ_SEL_X;
		    src_g = SQ_SEL_X;
		    src_b = SQ_SEL_X;
		    src_a = SQ_SEL_X;
		}

		//mask_color = R300_ALU_RGB_SRC1_RGB;
		mask_r = SQ_SEL_Y;
		mask_g = SQ_SEL_Z;
		mask_b = SQ_SEL_W;

		if (PICT_FORMAT_A(pMaskPicture->format) == 0) {
		    //mask_alpha = R300_ALU_ALPHA_1_0;
		    mask_a = SQ_SEL_1;
		} else {
		    //mask_alpha = R300_ALU_ALPHA_SRC1_A;
		    mask_a = SQ_SEL_X;
		}
	    } else {
		//src_color = R300_ALU_RGB_SRC0_RGB;
		src_r = SQ_SEL_Y;
		src_g = SQ_SEL_Z;
		src_b = SQ_SEL_W;

		if (PICT_FORMAT_A(pSrcPicture->format) == 0) {
		    //src_alpha = R300_ALU_ALPHA_1_0;
		    src_a = SQ_SEL_1;
		} else {
		    //src_alpha = R300_ALU_ALPHA_SRC0_A;
		    src_a = SQ_SEL_X;
		}

		//mask_color = R300_ALU_RGB_SRC1_RGB;
		mask_r = SQ_SEL_Y;
		mask_g = SQ_SEL_Z;
		mask_b = SQ_SEL_W;

		if (PICT_FORMAT_A(pMaskPicture->format) == 0) {
		    //mask_alpha = R300_ALU_ALPHA_1_0;
		    mask_a = SQ_SEL_1;
		} else {
		    //mask_alpha = R300_ALU_ALPHA_SRC1_A;
		    mask_a = SQ_SEL_X;
		}
	    }
	} else {
	    if (PICT_FORMAT_A(pMaskPicture->format) == 0) {
		//mask_color = R300_ALU_RGB_1_0;
		mask_r = SQ_SEL_1;
		mask_g = SQ_SEL_1;
		mask_b = SQ_SEL_1;
	    } else {
		//mask_color = R300_ALU_RGB_SRC1_AAA;
		mask_r = SQ_SEL_X;
		mask_g = SQ_SEL_X;
		mask_b = SQ_SEL_X;
	    }
	    if (PICT_FORMAT_A(pMaskPicture->format) == 0) {
		//mask_alpha = R300_ALU_ALPHA_1_0;
		mask_a = SQ_SEL_1;
	    } else {
		//mask_alpha = R300_ALU_ALPHA_SRC1_A;
		mask_a = SQ_SEL_X;
	    }
	}

	//0
	ps[i++] = CF_DWORD0(ADDR(8));
	ps[i++] = CF_DWORD1(POP_COUNT(0),
			    CF_CONST(0),
			    COND(SQ_CF_COND_ACTIVE),
			    I_COUNT(2),
			    CALL_COUNT(0),
			    END_OF_PROGRAM(0),
			    VALID_PIXEL_MODE(0),
			    CF_INST(SQ_CF_INST_TEX),
			    WHOLE_QUAD_MODE(0),
			    BARRIER(1));

	// 1
	ps[i++] = CF_ALU_DWORD0(ADDR(3),
				KCACHE_BANK0(0),
				KCACHE_BANK1(0),
				KCACHE_MODE0(SQ_CF_KCACHE_NOP));
	ps[i++] = CF_ALU_DWORD1(KCACHE_MODE1(SQ_CF_KCACHE_NOP),
				KCACHE_ADDR0(0),
				KCACHE_ADDR1(0),
				I_COUNT(4),
				USES_WATERFALL(0),
				CF_INST(SQ_CF_INST_ALU),
				WHOLE_QUAD_MODE(0),
				BARRIER(1));

	//2
	ps[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_PIXEL_MRT0),
					  TYPE(SQ_EXPORT_PIXEL),
					  RW_GPR(2),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(1));

	ps[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(1),
					       END_OF_PROGRAM(1),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT_DONE),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(1));

	// 3 - alu 0
	// MUL gpr[2].x gpr[1].x gpr[0].x
	ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			     SRC0_REL(ABSOLUTE),
			     SRC0_ELEM(ELEM_X),
			     SRC0_NEG(0),
			     SRC1_SEL(0),
			     SRC1_REL(ABSOLUTE),
			     SRC1_ELEM(ELEM_X),
			     SRC1_NEG(0),
			     INDEX_MODE(SQ_INDEX_LOOP),
			     PRED_SEL(SQ_PRED_SEL_OFF),
			     LAST(0));
	ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
				 SRC0_ABS(0),
				 SRC1_ABS(0),
				 UPDATE_EXECUTE_MASK(0),
				 UPDATE_PRED(0),
				 WRITE_MASK(1),
				 FOG_MERGE(0),
				 OMOD(SQ_ALU_OMOD_OFF),
				 ALU_INST(SQ_OP2_INST_MUL),
				 BANK_SWIZZLE(SQ_ALU_VEC_012),
				 DST_GPR(2),
				 DST_REL(ABSOLUTE),
				 DST_ELEM(ELEM_X),
				 CLAMP(0));
	// 4 - alu 1
	// MUL gpr[2].y gpr[1].y gpr[0].y
	ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			     SRC0_REL(ABSOLUTE),
			     SRC0_ELEM(ELEM_Y),
			     SRC0_NEG(0),
			     SRC1_SEL(0),
			     SRC1_REL(ABSOLUTE),
			     SRC1_ELEM(ELEM_Y),
			     SRC1_NEG(0),
			     INDEX_MODE(SQ_INDEX_LOOP),
			     PRED_SEL(SQ_PRED_SEL_OFF),
			     LAST(0));
	ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
				 SRC0_ABS(0),
				 SRC1_ABS(0),
				 UPDATE_EXECUTE_MASK(0),
				 UPDATE_PRED(0),
				 WRITE_MASK(1),
				 FOG_MERGE(0),
				 OMOD(SQ_ALU_OMOD_OFF),
				 ALU_INST(SQ_OP2_INST_MUL),
				 BANK_SWIZZLE(SQ_ALU_VEC_012),
				 DST_GPR(2),
				 DST_REL(ABSOLUTE),
				 DST_ELEM(ELEM_Y),
				 CLAMP(0));
	// 5 - alu 2
	// MUL gpr[2].z gpr[1].z gpr[0].z
	ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			     SRC0_REL(ABSOLUTE),
			     SRC0_ELEM(ELEM_Z),
			     SRC0_NEG(0),
			     SRC1_SEL(0),
			     SRC1_REL(ABSOLUTE),
			     SRC1_ELEM(ELEM_Z),
			     SRC1_NEG(0),
			     INDEX_MODE(SQ_INDEX_LOOP),
			     PRED_SEL(SQ_PRED_SEL_OFF),
			     LAST(0));
	ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
				 SRC0_ABS(0),
				 SRC1_ABS(0),
				 UPDATE_EXECUTE_MASK(0),
				 UPDATE_PRED(0),
				 WRITE_MASK(1),
				 FOG_MERGE(0),
				 OMOD(SQ_ALU_OMOD_OFF),
				 ALU_INST(SQ_OP2_INST_MUL),
				 BANK_SWIZZLE(SQ_ALU_VEC_012),
				 DST_GPR(2),
				 DST_REL(ABSOLUTE),
				 DST_ELEM(ELEM_Z),
				 CLAMP(0));
	// 6 - alu 3
	// MUL gpr[2].w gpr[1].w gpr[0].w
	ps[i++] = ALU_DWORD0(SRC0_SEL(1),
			     SRC0_REL(ABSOLUTE),
			     SRC0_ELEM(ELEM_W),
			     SRC0_NEG(0),
			     SRC1_SEL(0),
			     SRC1_REL(ABSOLUTE),
			     SRC1_ELEM(ELEM_W),
			     SRC1_NEG(0),
			     INDEX_MODE(SQ_INDEX_LOOP),
			     PRED_SEL(SQ_PRED_SEL_OFF),
			     LAST(0));
	ps[i++] = ALU_DWORD1_OP2(rhdPtr->ChipSet,
				 SRC0_ABS(0),
				 SRC1_ABS(0),
				 UPDATE_EXECUTE_MASK(0),
				 UPDATE_PRED(0),
				 WRITE_MASK(1),
				 FOG_MERGE(0),
				 OMOD(SQ_ALU_OMOD_OFF),
				 ALU_INST(SQ_OP2_INST_MUL),
				 BANK_SWIZZLE(SQ_ALU_VEC_012),
				 DST_GPR(2),
				 DST_REL(ABSOLUTE),
				 DST_ELEM(ELEM_W),
				 CLAMP(0));
	// 7
	ps[i++] = 0x00000000;
	ps[i++] = 0x00000000;

	//8/9 - src
	ps[i++] = TEX_DWORD0(TEX_INST(SQ_TEX_INST_SAMPLE),
			     BC_FRAC_MODE(0),
			     FETCH_WHOLE_QUAD(0),
			     RESOURCE_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     R7xx_ALT_CONST(0));
	ps[i++] = TEX_DWORD1(DST_GPR(1),
			     DST_REL(ABSOLUTE),
			     DST_SEL_X(src_a),
			     DST_SEL_Y(src_r),
			     DST_SEL_Z(src_g),
			     DST_SEL_W(src_b),
			     LOD_BIAS(0),
			     COORD_TYPE_X(TEX_NORMALIZED),
			     COORD_TYPE_Y(TEX_NORMALIZED),
			     COORD_TYPE_Z(TEX_NORMALIZED),
			     COORD_TYPE_W(TEX_NORMALIZED));
	ps[i++] = TEX_DWORD2(OFFSET_X(0),
			     OFFSET_Y(0),
			     OFFSET_Z(0),
			     SAMPLER_ID(0),
			     SRC_SEL_X(SQ_SEL_X),
			     SRC_SEL_Y(SQ_SEL_Y),
			     SRC_SEL_Z(SQ_SEL_0),
			     SRC_SEL_W(SQ_SEL_1));
	//10/11 - mask
	ps[i++] = TEX_DWORD0(TEX_INST(SQ_TEX_INST_SAMPLE),
			     BC_FRAC_MODE(0),
			     FETCH_WHOLE_QUAD(0),
			     RESOURCE_ID(0),
			     SRC_GPR(1),
			     SRC_REL(ABSOLUTE),
			     R7xx_ALT_CONST(0));
	ps[i++] = TEX_DWORD1(DST_GPR(0),
			     DST_REL(ABSOLUTE),
			     DST_SEL_X(mask_a),
			     DST_SEL_Y(mask_r),
			     DST_SEL_Z(mask_g),
			     DST_SEL_W(mask_b),
			     LOD_BIAS(0),
			     COORD_TYPE_X(TEX_NORMALIZED),
			     COORD_TYPE_Y(TEX_NORMALIZED),
			     COORD_TYPE_Z(TEX_NORMALIZED),
			     COORD_TYPE_W(TEX_NORMALIZED));
	ps[i++] = TEX_DWORD2(OFFSET_X(0),
			     OFFSET_Y(0),
			     OFFSET_Z(0),
			     SAMPLER_ID(0),
			     SRC_SEL_X(SQ_SEL_X),
			     SRC_SEL_Y(SQ_SEL_Y),
			     SRC_SEL_Z(SQ_SEL_0),
			     SRC_SEL_W(SQ_SEL_1));
    } else {
	int src_a, src_r, src_g, src_b;
	/* setup pixel shader */
	if (PICT_FORMAT_RGB(pSrcPicture->format) == 0) {
	    //src_color = R300_ALU_RGB_0_0;
	    src_r = SQ_SEL_0;
	    src_g = SQ_SEL_0;
	    src_b = SQ_SEL_0;
	} else {
	    //src_color = R300_ALU_RGB_SRC0_RGB;
	    src_r = SQ_SEL_Y;
	    src_g = SQ_SEL_Z;
	    src_b = SQ_SEL_W;
	}

	if (PICT_FORMAT_A(pSrcPicture->format) == 0) {
	    //src_alpha = R300_ALU_ALPHA_1_0;
	    src_a = SQ_SEL_1;
	} else {
	    //src_alpha = R300_ALU_ALPHA_SRC0_A;
	    src_a = SQ_SEL_X;
	}

	//0
	ps[i++] = CF_DWORD0(ADDR(2));
	ps[i++] = CF_DWORD1(POP_COUNT(0),
			    CF_CONST(0),
			    COND(SQ_CF_COND_ACTIVE),
			    I_COUNT(1),
			    CALL_COUNT(0),
			    END_OF_PROGRAM(0),
			    VALID_PIXEL_MODE(0),
			    CF_INST(SQ_CF_INST_TEX),
			    WHOLE_QUAD_MODE(0),
			    BARRIER(1));
	//1
	ps[i++] = CF_ALLOC_IMP_EXP_DWORD0(ARRAY_BASE(CF_PIXEL_MRT0),
					  TYPE(SQ_EXPORT_PIXEL),
					  RW_GPR(0),
					  RW_REL(ABSOLUTE),
					  INDEX_GPR(0),
					  ELEM_SIZE(1));

	ps[i++] = CF_ALLOC_IMP_EXP_DWORD1_SWIZ(SRC_SEL_X(SQ_SEL_X),
					       SRC_SEL_Y(SQ_SEL_Y),
					       SRC_SEL_Z(SQ_SEL_Z),
					       SRC_SEL_W(SQ_SEL_W),
					       R6xx_ELEM_LOOP(0),
					       BURST_COUNT(1),
					       END_OF_PROGRAM(1),
					       VALID_PIXEL_MODE(0),
					       CF_INST(SQ_CF_INST_EXPORT_DONE),
					       WHOLE_QUAD_MODE(0),
					       BARRIER(1));


	//2/3 - src
	ps[i++] = TEX_DWORD0(TEX_INST(SQ_TEX_INST_SAMPLE),
			     BC_FRAC_MODE(0),
			     FETCH_WHOLE_QUAD(0),
			     RESOURCE_ID(0),
			     SRC_GPR(0),
			     SRC_REL(ABSOLUTE),
			     R7xx_ALT_CONST(0));
	ps[i++] = TEX_DWORD1(DST_GPR(0),
			     DST_REL(ABSOLUTE),
			     DST_SEL_X(src_a),
			     DST_SEL_Y(src_r),
			     DST_SEL_Z(src_g),
			     DST_SEL_W(src_b),
			     LOD_BIAS(0),
			     COORD_TYPE_X(TEX_NORMALIZED),
			     COORD_TYPE_Y(TEX_NORMALIZED),
			     COORD_TYPE_Z(TEX_NORMALIZED),
			     COORD_TYPE_W(TEX_NORMALIZED));
	ps[i++] = TEX_DWORD2(OFFSET_X(0),
			     OFFSET_Y(0),
			     OFFSET_Z(0),
			     SAMPLER_ID(0),
			     SRC_SEL_X(SQ_SEL_X),
			     SRC_SEL_Y(SQ_SEL_Y),
			     SRC_SEL_Z(SQ_SEL_0),
			     SRC_SEL_W(SQ_SEL_1));

    }

    CLEAR (cb_conf);
    CLEAR (vs_conf);
    CLEAR (ps_conf);

    if (!R600GetDestFormat(pDstPicture, &dst_format))
	return FALSE;

    switch (pDstPicture->format) {
    case PICT_a8r8g8b8:
	ErrorF("dst: PICT_a8r8g8b8\n");
	break;
    case PICT_x8r8g8b8:
	ErrorF("dst: PICT_x8r8g8b8\n");
	break;
    case PICT_r5g6b5:
	ErrorF("dst: PICT_r5g6b5\n");
	break;
    case PICT_a1r5g5b5:
	ErrorF("dst: PICT_a1r5g5b5\n");
	break;
    case PICT_x1r5g5b5:
	ErrorF("dst: PICT_x1r5g5b5\n");
	break;
    case PICT_a8:
	ErrorF("dst: PICT_a8\n");
	break;
    default:
	break;
    }

    if (pMask)
	accel_state->has_mask = TRUE;
    else
	accel_state->has_mask = FALSE;

    dst_pitch = exaGetPixmapPitch(pDst) / (pDst->drawable.bitsPerPixel / 8);
    dst_offset = exaGetPixmapOffset(pDst) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    src_pitch = exaGetPixmapPitch(pSrc) / (pSrc->drawable.bitsPerPixel / 8);
    src_offset = exaGetPixmapOffset(pSrc) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

    if (dst_pitch & 63)
	RADEON_FALLBACK(("Bad dst pitch 0x%x\n", dst_pitch));

    if (dst_offset & 0xff)
	RADEON_FALLBACK(("Bad destination offset 0x%x\n", (int)dst_offset));

    if (src_pitch & 63)
	RADEON_FALLBACK(("Bad src pitch 0x%x\n", dst_pitch));

    if (src_offset & 0xff)
	RADEON_FALLBACK(("Bad src offset 0x%x\n", (int)src_offset));

    if (pMask) {
	uint32_t mask_pitch = exaGetPixmapPitch(pSrc) / (pSrc->drawable.bitsPerPixel / 8);
	uint32_t mask_offset = exaGetPixmapOffset(pSrc) + rhdPtr->FbIntAddress + rhdPtr->FbScanoutStart;

	if (mask_pitch & 63)
	    RADEON_FALLBACK(("Bad mask pitch 0x%x\n", dst_pitch));

	if (mask_offset & 0xff)
	    RADEON_FALLBACK(("Bad mask offset 0x%x\n", (int)mask_offset));

    }

    // fixme
    //if (!RADEONSetupSourceTile(pSrcPicture, pSrc, TRUE, FALSE))
    //return FALSE;

    accel_state->ib = RHDDRMCPBuffer(pScrn->scrnIndex);

    /* Init */
    start_3d(pScrn, accel_state->ib);

    cp_set_surface_sync(pScrn, accel_state->ib);

    set_default_state(pScrn, accel_state->ib);

    // fix me if false discard buffer!
    if (!R600TextureSetup(pSrcPicture, pSrc, 0))
	return FALSE;

    if (pMask != NULL) {
	// fix me if false discard buffer!
	if (!R600TextureSetup(pMaskPicture, pMask, 1))
	    return FALSE;
    } else {
	accel_state->is_transform[1] = FALSE;
    }

    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 512, vs, sizeof(vs));
    vs_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 512;
    memcpy ((char *)accel_state->ib->address + (accel_state->ib->total / 2) - 256, ps, sizeof(ps));
    ps_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2) - 256;

    /* Shader */
    vs_conf.shader_addr         = vs_addr;
    vs_conf.num_gprs            = 3;
    vs_conf.stack_size          = 0;
    vs_setup                    (pScrn, accel_state->ib, &vs_conf);

    ps_conf.shader_addr         = ps_addr;
    ps_conf.num_gprs            = 3;
    ps_conf.stack_size          = 0;
    ps_conf.uncached_first_inst = 1;
    ps_conf.clamp_consts        = 0;
    ps_conf.export_mode         = 2;
    ps_setup                    (pScrn, accel_state->ib, &ps_conf);

    ereg  (accel_state->ib, CB_SHADER_MASK,                      (0xf << OUTPUT0_ENABLE_shift));
    ereg  (accel_state->ib, R7xx_CB_SHADER_CONTROL,              (RT0_ENABLE_bit));

    blendcntl = R600GetBlendCntl(op, pMaskPicture, pDstPicture->format);
    if (rhdPtr->ChipSet == RHD_R600) {
	// no per-MRT blend on R600
	ereg  (accel_state->ib, CB_COLOR_CONTROL,                    RADEON_ROP[3] | (1 << TARGET_BLEND_ENABLE_shift));
	ereg  (accel_state->ib, CB_BLEND_CONTROL,                    blendcntl);
    } else {
	ereg  (accel_state->ib, CB_COLOR_CONTROL,                    (RADEON_ROP[3] |
								      (1 << TARGET_BLEND_ENABLE_shift) |
								      PER_MRT_BLEND_bit));
	ereg  (accel_state->ib, CB_BLEND0_CONTROL,                   blendcntl);
    }

    cb_conf.id = 0;
    cb_conf.w = dst_pitch;
    cb_conf.h = pDst->drawable.height;
    cb_conf.base = dst_offset;
    cb_conf.format = dst_format;
    cb_conf.comp_swap = 0;
    cb_conf.source_format = 1;
    cb_conf.blend_clamp = 1;
    set_render_target(pScrn, accel_state->ib, &cb_conf);

    ereg  (accel_state->ib, PA_SU_SC_MODE_CNTL,                  (FACE_bit			|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_FRONT_PTYPE_shift)	|
						 (POLYMODE_PTYPE__TRIANGLES << POLYMODE_BACK_PTYPE_shift)));
    ereg  (accel_state->ib, DB_SHADER_CONTROL,                   ((1 << Z_ORDER_shift)		| /* EARLY_Z_THEN_LATE_Z */
						 DUAL_EXPORT_ENABLE_bit)); /* Only useful if no depth export */

    /* Interpolator setup */
    if (pMask) {
	// export 2 tex coords from VS
	ereg  (accel_state->ib, SPI_VS_OUT_CONFIG, ((2 - 1) << VS_EXPORT_COUNT_shift));
	// src = semantic id 0; mask = semantic id 1
	ereg  (accel_state->ib, SPI_VS_OUT_ID_0, ((0 << SEMANTIC_0_shift) |
						  (1 << SEMANTIC_1_shift)));
	// input 2 tex coords from VS
	ereg  (accel_state->ib, SPI_PS_IN_CONTROL_0, (2 << NUM_INTERP_shift));
    } else {
	// export 1 tex coords from VS
	ereg  (accel_state->ib, SPI_VS_OUT_CONFIG, ((1 - 1) << VS_EXPORT_COUNT_shift));
	// src = semantic id 0
	ereg  (accel_state->ib, SPI_VS_OUT_ID_0,   (0 << SEMANTIC_0_shift));
	// input 1 tex coords from VS
	ereg  (accel_state->ib, SPI_PS_IN_CONTROL_0, (1 << NUM_INTERP_shift));
    }
    ereg  (accel_state->ib, SPI_PS_IN_CONTROL_1,                 0);
    // SPI_PS_INPUT_CNTL_0 maps to GPR[0] - load with semantic id 0
    ereg  (accel_state->ib, SPI_PS_INPUT_CNTL_0 + (0 <<2),       ((0    << SEMANTIC_shift)	|
								  (0x01 << DEFAULT_VAL_shift)	|
								  SEL_CENTROID_bit));
    // SPI_PS_INPUT_CNTL_1 maps to GPR[1] - load with semantic id 1
    ereg  (accel_state->ib, SPI_PS_INPUT_CNTL_0 + (1 <<2),       ((1    << SEMANTIC_shift)	|
								  (0x01 << DEFAULT_VAL_shift)	|
								  SEL_CENTROID_bit));
    ereg  (accel_state->ib, SPI_INTERP_CONTROL_0,                0);

    accel_state->vb_index = 0;

    return TRUE;
}

static void R600Composite(PixmapPtr pDst,
			  int srcX, int srcY,
			  int maskX, int maskY,
			  int dstX, int dstY,
			  int w, int h)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    struct r6xx_comp_vertex *comp_vb = (pointer)((char*)accel_state->ib->address + (accel_state->ib->total / 2));
    struct r6xx_comp_vertex vertex[3];
    xPointFixed srcTopLeft, srcTopRight, srcBottomLeft, srcBottomRight;
    xPointFixed maskTopLeft, maskTopRight, maskBottomLeft, maskBottomRight;

    /* ErrorF("R600Composite (%d,%d) (%d,%d) (%d,%d) (%d,%d)\n",
       srcX, srcY, maskX, maskY,dstX, dstY, w, h); */

    srcTopLeft.x     = IntToxFixed(srcX);
    srcTopLeft.y     = IntToxFixed(srcY);
    srcTopRight.x    = IntToxFixed(srcX + w);
    srcTopRight.y    = IntToxFixed(srcY);
    srcBottomLeft.x  = IntToxFixed(srcX);
    srcBottomLeft.y  = IntToxFixed(srcY + h);
    srcBottomRight.x = IntToxFixed(srcX + w);
    srcBottomRight.y = IntToxFixed(srcY + h);

    maskTopLeft.x     = IntToxFixed(maskX);
    maskTopLeft.y     = IntToxFixed(maskY);
    maskTopRight.x    = IntToxFixed(maskX + w);
    maskTopRight.y    = IntToxFixed(maskY);
    maskBottomLeft.x  = IntToxFixed(maskX);
    maskBottomLeft.y  = IntToxFixed(maskY + h);
    maskBottomRight.x = IntToxFixed(maskX + w);
    maskBottomRight.y = IntToxFixed(maskY + h);

    //XXX do transform in vertex shader
    if (accel_state->is_transform[0]) {
	transformPoint(accel_state->transform[0], &srcTopLeft);
	transformPoint(accel_state->transform[0], &srcTopRight);
	transformPoint(accel_state->transform[0], &srcBottomLeft);
	transformPoint(accel_state->transform[0], &srcBottomRight);
    }
    if (accel_state->is_transform[1]) {
	transformPoint(accel_state->transform[1], &maskTopLeft);
	transformPoint(accel_state->transform[1], &maskTopRight);
	transformPoint(accel_state->transform[1], &maskBottomLeft);
	transformPoint(accel_state->transform[1], &maskBottomRight);
    }

    vertex[0].x = dstX;
    vertex[0].y = dstY;
    vertex[0].src_s = xFixedToFloat(srcTopLeft.x) / accel_state->texW[0];
    vertex[0].src_t = xFixedToFloat(srcTopLeft.y) / accel_state->texH[0];

    vertex[1].x = dstX;
    vertex[1].y = dstY + h;
    vertex[1].src_s = xFixedToFloat(srcBottomLeft.x) / accel_state->texW[0];
    vertex[1].src_t = xFixedToFloat(srcBottomLeft.y) / accel_state->texH[0];

    vertex[2].x = dstX + w;
    vertex[2].y = dstY + h;
    vertex[2].src_s = xFixedToFloat(srcBottomRight.x) / accel_state->texW[0];
    vertex[2].src_t = xFixedToFloat(srcBottomRight.y) / accel_state->texH[0];

    if (accel_state->has_mask) {
	vertex[0].mask_s = xFixedToFloat(maskTopLeft.x) / accel_state->texW[1];
	vertex[0].mask_t = xFixedToFloat(maskTopLeft.y) / accel_state->texH[1];

	vertex[1].mask_s = xFixedToFloat(maskBottomLeft.x) / accel_state->texW[1];
	vertex[1].mask_t = xFixedToFloat(maskBottomLeft.y) / accel_state->texH[1];

	vertex[2].mask_s = xFixedToFloat(maskBottomRight.x) / accel_state->texW[1];
	vertex[2].mask_t = xFixedToFloat(maskBottomRight.y) / accel_state->texH[1];

#ifdef SHOW_VERTEXES
	ErrorF("vertex 0: %d, %d, %f, %f, %f, %f\n", vertex[0].x, vertex[0].y,
	       vertex[0].src_s, vertex[0].src_t, vertex[0].mask_s, vertex[0].mask_t);
	ErrorF("vertex 1: %d, %d, %f, %f, %f, %f\n", vertex[1].x, vertex[1].y,
	       vertex[1].src_s, vertex[1].src_t, vertex[1].mask_s, vertex[1].mask_t);
	ErrorF("vertex 2: %d, %d, %f, %f, %f, %f\n", vertex[2].x, vertex[2].y,
	       vertex[2].src_s, vertex[2].src_t,  vertex[2].mask_s, vertex[2].mask_t);
#endif
    } else {
	vertex[0].mask_s = 0;
	vertex[0].mask_t = 0;

	vertex[1].mask_s = 0;
	vertex[1].mask_t = 0;

	vertex[2].mask_s = 0;
	vertex[2].mask_t = 0;

#ifdef SHOW_VERTEXES
	ErrorF("vertex 0: %d, %d, %f, %f\n", vertex[0].x, vertex[0].y, vertex[0].src_s, vertex[0].src_t);
	ErrorF("vertex 1: %d, %d, %f, %f\n", vertex[1].x, vertex[1].y, vertex[1].src_s, vertex[1].src_t);
	ErrorF("vertex 2: %d, %d, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].src_s, vertex[2].src_t);
	ErrorF("vertex 3: %d, %d, %f, %f\n", vertex[2].x, vertex[2].y, vertex[2].src_s, vertex[2].src_t);
#endif
    }

    // append to vertex buffer
    comp_vb[accel_state->vb_index++] = vertex[0];
    comp_vb[accel_state->vb_index++] = vertex[1];
    comp_vb[accel_state->vb_index++] = vertex[2];

}

static void R600DoneComposite(PixmapPtr pDst)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    draw_config_t   draw_conf;
    vtx_resource_t  vtx_res;
    uint64_t vb_addr;

    CLEAR (draw_conf);
    CLEAR (vtx_res);

    if (accel_state->vb_index == 0) {
	R600CPFlushIndirect(pScrn, accel_state->ib);
	return;
    }

    vb_addr = RHDDRIGetIntGARTLocation(pScrn) +
	(accel_state->ib->idx * accel_state->ib->total) + (accel_state->ib->total / 2);

    /* Vertex buffer setup */
    vtx_res.id              = SQ_VTX_RESOURCE_vs;
    vtx_res.vtx_size_dw     = 24 / 4;
    vtx_res.vtx_num_entries = accel_state->vb_index * 24 / 4;
    vtx_res.mem_req_size    = 1;
    vtx_res.vb_addr         = vb_addr;
    set_vtx_resource        (pScrn, accel_state->ib, &vtx_res);

    draw_conf.prim_type          = DI_PT_RECTLIST;
    draw_conf.vgt_draw_initiator = DI_SRC_SEL_AUTO_INDEX;
    draw_conf.num_instances      = 1;
    draw_conf.num_indices        = vtx_res.vtx_num_entries / vtx_res.vtx_size_dw;
    draw_conf.index_type         = DI_INDEX_SIZE_16_BIT;

    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_0,            0);	/* ? */
    ereg  (accel_state->ib, VGT_INSTANCE_STEP_RATE_1,            0);

    ereg  (accel_state->ib, VGT_MAX_VTX_INDX,                    draw_conf.num_indices);
    ereg  (accel_state->ib, VGT_MIN_VTX_INDX,                    0);
    ereg  (accel_state->ib, VGT_INDX_OFFSET,                     0);

    draw_auto(pScrn, accel_state->ib, &draw_conf);

    wait_3d_idle_clean(pScrn, accel_state->ib);

    R600CPFlushIndirect(pScrn, accel_state->ib);
}

static Bool
R600UploadToScreen(PixmapPtr pDst, int x, int y, int w, int h,
		   char *src, int src_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
//    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    uint8_t *dst = (pointer)((char *)rhdPtr->FbBase + rhdPtr->FbScanoutStart + exaGetPixmapOffset(pDst));
    int dst_pitch = exaGetPixmapPitch(pDst);
    int bpp = pDst->drawable.bitsPerPixel;


    //return FALSE;

    dst += (x * bpp / 8) + (y * dst_pitch);
    w *= bpp / 8;

    while (h--) {
	memcpy(dst, src, w);
	src += src_pitch;
	dst += dst_pitch;
    }

    return TRUE;
}

static Bool
R600DownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
		       char *dst, int dst_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pSrc->drawable.pScreen->myNum];
    RHDPtr rhdPtr = RHDPTR(pScrn);
//    struct r6xx_accel_state *accel_state = rhdPtr->TwoDPrivate;
    uint8_t *src = (pointer)((char *)rhdPtr->FbBase + rhdPtr->FbScanoutStart + exaGetPixmapOffset(pSrc));
    int	src_pitch = exaGetPixmapPitch(pSrc);
    int	bpp = pSrc->drawable.bitsPerPixel;

    //return FALSE;

    src += (x * bpp / 8) + (y * src_pitch);
    w *= bpp / 8;

    while (h--) {
	memcpy(dst, src, w);
	src += src_pitch;
	dst += dst_pitch;
    }

    return TRUE;
}

void
R6xxEXACloseScreen(ScreenPtr pScreen)
{
    exaDriverFini(pScreen);
}

void
R6xxEXADestroy(ScrnInfoPtr pScrn)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);

    if (rhdPtr->EXAInfo) {
	xfree(rhdPtr->EXAInfo);
	rhdPtr->EXAInfo = NULL;
    }

    if (rhdPtr->TwoDPrivate) {
	xfree(rhdPtr->TwoDPrivate);
	rhdPtr->TwoDPrivate = NULL;
    }
}

/* no need to needlessly flush the caches/wait for idle
 * the drawing code does this already (and mesa code should be designed to do so as well)
 * excessive idling/flushing seems to cause stability problems on
 * r7xx and drawing glitches on r6xx.
 */
void
R6xxCacheFlush(struct RhdCS *CS)
{
    CS = CS; // nop - avoid compiler warning
}

void
R6xxEngineWaitIdleFull(struct RhdCS *CS)
{
    CS = CS; // nop - avoid compiler warning
}

static int
R600EXAMarkSync(ScreenPtr pScreen)
{
    struct r6xx_accel_state *accel_state = RHDPTR(xf86Screens[pScreen->myNum])->TwoDPrivate;

    accel_state->exaSyncMarker++;

    return accel_state->exaSyncMarker;
}

static void
R600EXASync(ScreenPtr pScreen, int marker)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct r6xx_accel_state *accel_state = RHDPTR(pScrn)->TwoDPrivate;

    if (accel_state->exaMarkerSynced != marker) {
	struct RhdCS *CS = RHDPTR(pScrn)->CS;

	RHDCSFlush(CS);
	RHDCSIdle(CS);

	accel_state->exaMarkerSynced = marker;
    }
}

Bool
R6xxEXAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen)
{
    RHDPtr rhdPtr = RHDPTR(pScrn);
    struct RhdCS *CS = rhdPtr->CS;
    ExaDriverRec *EXAInfo;
    struct r6xx_accel_state *accel_state;

    RHDFUNC(pScrn);

    EXAInfo = exaDriverAlloc();
    if (EXAInfo == NULL || !CS)
	return FALSE;

    accel_state = xnfcalloc(1, sizeof(struct r6xx_accel_state));

    EXAInfo->exa_major = EXA_VERSION_MAJOR;
    EXAInfo->exa_minor = EXA_VERSION_MINOR;

    EXAInfo->flags = EXA_OFFSCREEN_PIXMAPS;
    EXAInfo->pixmapOffsetAlign = 0x1000;
    EXAInfo->pixmapPitchAlign = 64;

#if EXA_VERSION_MAJOR > 2 || (EXA_VERSION_MAJOR == 2 && EXA_VERSION_MINOR >= 3)
    EXAInfo->maxPitchBytes = 16320;
    EXAInfo->maxX = 8192;
#else
    EXAInfo->maxX = 16320 / 4;
#endif
    EXAInfo->maxY = 8192;

    EXAInfo->memoryBase = (CARD8 *) rhdPtr->FbBase + rhdPtr->FbScanoutStart;
    EXAInfo->offScreenBase = rhdPtr->FbOffscreenStart - rhdPtr->FbScanoutStart;
    EXAInfo->memorySize = rhdPtr->FbScanoutSize + rhdPtr->FbOffscreenSize;

    EXAInfo->PrepareSolid = R600PrepareSolid;
    EXAInfo->Solid = R600Solid;
    EXAInfo->DoneSolid = R600DoneSolid;

    EXAInfo->PrepareCopy = R600PrepareCopy;
    EXAInfo->Copy = R600Copy;
    EXAInfo->DoneCopy = R600DoneCopy;

    EXAInfo->CheckComposite = R600CheckComposite;
    EXAInfo->PrepareComposite = R600PrepareComposite;
    EXAInfo->Composite = R600Composite;
    EXAInfo->DoneComposite = R600DoneComposite;

    EXAInfo->UploadToScreen = R600UploadToScreen;
    EXAInfo->DownloadFromScreen = R600DownloadFromScreen;

    EXAInfo->MarkSync = R600EXAMarkSync;
    EXAInfo->WaitMarker = R600EXASync;

    if (!exaDriverInit(pScreen, EXAInfo)) {
	xfree(accel_state);
	xfree(EXAInfo);
	return FALSE;
    }

    RHDPTR(pScrn)->EXAInfo = EXAInfo;

    accel_state->XHas3DEngineState = FALSE;

    rhdPtr->TwoDPrivate = accel_state;

    exaMarkSync(pScreen);

    return TRUE;
}

