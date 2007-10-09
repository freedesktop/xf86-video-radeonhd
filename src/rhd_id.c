/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86Resources.h"

#include "rhd.h"
#include "rhd_output.h"
#include "rhd_connector.h"
#include "rhd_card.h"

SymTabRec RHDChipsets[] = {
    /* R500 */
    { RHD_RV505, "RV505" },
    { RHD_RV515, "RV515" },
    { RHD_RV516, "RV516" },
    { RHD_R520,  "R520" },
    { RHD_RV530, "RV530" },
    { RHD_RV535, "RV535" },
    { RHD_RV550, "RV550" },
    { RHD_RV560, "RV560" },
    { RHD_RV570, "RV570" },
    { RHD_R580,  "R580" },
    /* R500 Mobility */
    { RHD_M52,   "M52" },
    { RHD_M54,   "M54" },
    { RHD_M56,   "M56" },
    { RHD_M58,   "M58" },
    { RHD_M62,   "M62" },
    { RHD_M64,   "M64" },
    { RHD_M66,   "M66" },
    { RHD_M68,   "M68" },
    { RHD_M71,   "M71" },
    /* R500 integrated */
    { RHD_RS690, "RS690" },
    { RHD_RS740, "RS740" },
    /* R600 */
    { RHD_R600,  "R600" },
    { RHD_RV610, "RV610" },
    { RHD_RV630, "RV630" },
    /* R600 Mobility */
    { RHD_M72,   "M72" },
    { RHD_M74,   "M74" },
    { RHD_M76,   "M76" },
    /* R600 integrated */
    { -1,      NULL }
};

resRange res_none[] = { _END };

/*
 * This is what people would refer to as "Petite".
 */
PciChipsets RHDPCIchipsets[] = {
    { RHD_R520,  0x7100, res_none }, /* Radeon X1800 */
    { RHD_M58,   0x7101, res_none }, /* Mobility Radeon X1800 XT */
    { RHD_M58,   0x7102, res_none }, /* Mobility Radeon X1800 */
    { RHD_M58,   0x7103, res_none }, /* Mobility FireGL V7200 */
    { RHD_R520,  0x7104, res_none }, /* FireGL V7200 */
    { RHD_R520,  0x7105, res_none }, /* FireGL V5300 */
    { RHD_M58,   0x7106, res_none }, /* Mobility FireGL V7100 */
    { RHD_R520,  0x7108, res_none }, /* Radeon X1800 */
    { RHD_R520,  0x7109, res_none }, /* Radeon X1800 */
    { RHD_R520,  0x710A, res_none }, /* Radeon X1800 */
    { RHD_R520,  0x710B, res_none }, /* Radeon X1800 */
    { RHD_R520,  0x710C, res_none }, /* Radeon X1800 */
    { RHD_R520,  0x710E, res_none }, /* FireGL V7300 */
    { RHD_R520,  0x710F, res_none }, /* FireGL V7350 */
    { RHD_RV515, 0x7140, res_none }, /* Radeon X1600 */
    { RHD_RV505, 0x7141, res_none }, /* RV505 */
    { RHD_RV515, 0x7142, res_none }, /* Radeon X1300/X1550 */
    { RHD_RV505, 0x7143, res_none }, /* Radeon X1550 */
    { RHD_M54,   0x7144, res_none }, /* M54-GL */
    { RHD_M54,   0x7145, res_none }, /* Mobility Radeon X1400 */
    { RHD_RV515, 0x7146, res_none }, /* Radeon X1300/X1550 */
    { RHD_RV505, 0x7147, res_none }, /* Radeon X1550 64-bit */
    { RHD_M52,   0x7149, res_none }, /* Mobility Radeon X1300 */
    { RHD_M52,   0x714A, res_none }, /* Mobility Radeon X1300 */
    { RHD_M52,   0x714B, res_none }, /* Mobility Radeon X1300 */
    { RHD_M52,   0x714C, res_none }, /* Mobility Radeon X1300 */
    { RHD_RV515, 0x714D, res_none }, /* Radeon X1300 */
    { RHD_RV515, 0x714E, res_none }, /* Radeon X1300 */
    { RHD_RV505, 0x714F, res_none }, /* RV505 */
    { RHD_RV505, 0x7151, res_none }, /* RV505 */
    { RHD_RV515, 0x7152, res_none }, /* FireGL V3300 */
    { RHD_RV515, 0x7153, res_none }, /* FireGL V3350 */
    { RHD_RV515, 0x715E, res_none }, /* Radeon X1300 */
    { RHD_RV505, 0x715F, res_none }, /* Radeon X1550 64-bit */
    { RHD_RV516, 0x7180, res_none }, /* Radeon X1300/X1550 */
    { RHD_RV516, 0x7181, res_none }, /* Radeon X1600 */
    { RHD_RV516, 0x7183, res_none }, /* Radeon X1300/X1550 */
    { RHD_M64,   0x7186, res_none }, /* Mobility Radeon X1450 */
    { RHD_RV516, 0x7187, res_none }, /* Radeon X1300/X1550 */
    { RHD_M64,   0x7188, res_none }, /* Mobility Radeon X2300 */
    { RHD_M64,   0x718A, res_none }, /* Mobility Radeon X2300 */
    { RHD_M62,   0x718B, res_none }, /* Mobility Radeon X1350 */
    { RHD_M62,   0x718C, res_none }, /* Mobility Radeon X1350 */
    { RHD_M64,   0x718D, res_none }, /* Mobility Radeon X1450 */
    { RHD_RV516, 0x718F, res_none }, /* Radeon X1300 */
    { RHD_RV516, 0x7193, res_none }, /* Radeon X1550 */
    { RHD_M62,   0x7196, res_none }, /* Mobility Radeon X1350 */
    { RHD_RV516, 0x719B, res_none }, /* FireMV 2250 */
    { RHD_RV516, 0x719F, res_none }, /* Radeon X1550 64-bit */
    { RHD_RV530, 0x71C0, res_none }, /* Radeon X1600 */
    { RHD_RV535, 0x71C1, res_none }, /* Radeon X1650 */
    { RHD_RV530, 0x71C2, res_none }, /* Radeon X1600 */
    { RHD_RV535, 0x71C3, res_none }, /* Radeon X1600 */
    { RHD_M56,   0x71C4, res_none }, /* Mobility FireGL V5200 */
    { RHD_M56,   0x71C5, res_none }, /* Mobility Radeon X1600 */
    { RHD_RV530, 0x71C6, res_none }, /* Radeon X1650 */
    { RHD_RV535, 0x71C7, res_none }, /* Radeon X1650 */
    { RHD_RV530, 0x71CD, res_none }, /* Radeon X1600 */
    { RHD_RV530, 0x71CE, res_none }, /* Radeon X1300 XT/X1600 Pro */
    { RHD_RV530, 0x71D2, res_none }, /* FireGL V3400 */
    { RHD_M66,   0x71D4, res_none }, /* Mobility FireGL V5250 */
    { RHD_M66,   0x71D5, res_none }, /* Mobility Radeon X1700 */
    { RHD_M66,   0x71D6, res_none }, /* Mobility Radeon X1700 XT */
    { RHD_RV530, 0x71DA, res_none }, /* FireGL V5200 */
    { RHD_M66,   0x71DE, res_none }, /* Mobility Radeon X1700 */
    { RHD_RV550, 0x7200, res_none }, /*  Radeon X2300HD  */
    { RHD_M71,   0x7210, res_none }, /* Mobility Radeon HD 2300 */
    { RHD_M71,   0x7211, res_none }, /* Mobility Radeon HD 2300 */
    { RHD_R580,  0x7240, res_none }, /* Radeon X1950 */
    { RHD_R580,  0x7243, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x7244, res_none }, /* Radeon X1950 */
    { RHD_R580,  0x7245, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x7246, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x7247, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x7248, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x7249, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x724A, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x724B, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x724C, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x724D, res_none }, /* Radeon X1900 */
    { RHD_R580,  0x724E, res_none }, /* AMD Stream Processor */
    { RHD_R580,  0x724F, res_none }, /* Radeon X1900 */
    { RHD_RV570, 0x7280, res_none }, /* Radeon X1950 */
    { RHD_RV560, 0x7281, res_none }, /* RV560 */
    { RHD_RV560, 0x7283, res_none }, /* RV560 */
    { RHD_M68,   0x7284, res_none }, /* Mobility Radeon X1900 */
    { RHD_RV560, 0x7287, res_none }, /* RV560 */
    { RHD_RV570, 0x7288, res_none }, /* Radeon X1950 GT */
    { RHD_RV570, 0x7289, res_none }, /* RV570 */
    { RHD_RV570, 0x728B, res_none }, /* RV570 */
    { RHD_RV570, 0x728C, res_none }, /* ATI FireGL V7400  */
    { RHD_RV560, 0x7290, res_none }, /* RV560 */
    { RHD_RV560, 0x7291, res_none }, /* Radeon X1650 */
    { RHD_RV560, 0x7293, res_none }, /* Radeon X1650 */
    { RHD_RV560, 0x7297, res_none }, /* RV560 */
    { RHD_RS690, 0x791E, res_none }, /* Radeon X1200 */
    { RHD_RS690, 0x791F, res_none }, /* Radeon X1200 */
    { RHD_RS740, 0x796C, res_none }, /* RS740 */
    { RHD_RS740, 0x796D, res_none }, /* RS740M */
    { RHD_RS740, 0x796E, res_none }, /* RS740 */
    { RHD_RS740, 0x796F, res_none }, /* RS740M */
    { RHD_R600,  0x9400, res_none }, /* Radeon HD 2900 XT */
    { RHD_R600,  0x9401, res_none }, /* Radeon HD 2900 XT */
    { RHD_R600,  0x9402, res_none }, /* Radeon HD 2900 XT */
    { RHD_R600,  0x9403, res_none }, /* Radeon HD 2900 Pro */
    { RHD_R600,  0x9405, res_none }, /* Radeon HD 2900 GT */
    { RHD_R600,  0x940A, res_none }, /* FireGL V8650 */
    { RHD_R600,  0x940B, res_none }, /* FireGL V8600 */
    { RHD_R600,  0x940F, res_none }, /* FireGL V7600 */
    { RHD_RV610, 0x94C0, res_none }, /* RV610 */
    { RHD_RV610, 0x94C1, res_none }, /* Radeon HD 2400 XT */
    { RHD_RV610, 0x94C3, res_none }, /* Radeon HD 2400 Pro */
    { RHD_RV610, 0x94C4, res_none }, /* ATI Radeon HD 2400 PRO AGP */
    { RHD_RV610, 0x94C5, res_none }, /* FireGL V4000 */
    { RHD_RV610, 0x94C6, res_none }, /* RV610 */
    { RHD_RV610, 0x94C7, res_none }, /* ATI Radeon HD 2350 */
    { RHD_M74,   0x94C8, res_none }, /* Mobility Radeon HD 2400 XT */
    { RHD_M72,   0x94C9, res_none }, /* Mobility Radeon HD 2400 */
    { RHD_M72,   0x94CB, res_none }, /* ATI RADEON E2400 */
    { RHD_RV610, 0x94CC, res_none }, /* RV610 */
    { RHD_RV630, 0x9580, res_none }, /* RV630 */
    { RHD_M76,   0x9581, res_none }, /* Mobility Radeon HD 2600 */
    { RHD_M76,   0x9583, res_none }, /* Mobility Radeon HD 2600 XT */
    { RHD_RV630, 0x9586, res_none }, /* ATI Radeon HD 2600 XT AGP */
    { RHD_RV630, 0x9587, res_none }, /* ATI Radeon HD 2600 Pro AGP */
    { RHD_RV630, 0x9588, res_none }, /* Radeon HD 2600 XT */
    { RHD_RV630, 0x9589, res_none }, /* Radeon HD 2600 Pro */
    { RHD_RV630, 0x958A, res_none }, /* Gemini RV630 */
    { RHD_M76,   0x958B, res_none }, /* Gemini ATI Mobility Radeon HD 2600 XT */
    { RHD_RV630, 0x958C, res_none }, /* FireGL V5600 */
    { RHD_RV630, 0x958D, res_none }, /* FireGL V3600 */
    { -1,	 -1,     RES_UNDEFINED }
};

/*
 *
 */
void
RHDIdentify(int flags)
{
    xf86Msg(X_INFO, "%s: X driver for the following AMD GPG (ATI) graphics devices:\n", RHD_NAME);
    xf86Msg(X_NONE, "\t"
	    "RV505  : Radeon X1550, X1550 64bit.\n\t"
	    "RV515  : Radeon X1300, X1550, X1600; FireGL V3300, V3350.\n\t"
	    "RV516  : Radeon X1300, X1550, X1550 64-bit, X1600; FireMV 2250.\n\t"
	    "R520   : Radeon X1800; FireGL V5300, V7200, V7300, V7350.\n\t"
	    "RV530  : Radeon X1300 XT, X1600, X1600 Pro, X1650; FireGL V3400, V5200.\n\t"
	    "RV535  : Radeon X1300, X1650.\n\t"
	    "RV550  : Radeon X2300 HD.\n\t"
	    "RV560  : Radeon X1650.\n\t"
	    "RV570  : Radeon X1950, X1950 GT; FireGL V7400.\n\t"
	    "R580   : Radeon X1900, X1950; AMD Stream Processor.\n");
    xf86Msg(X_NONE, "\t"
	    "R600   : Radeon HD 2900 GT/Pro/XT; FireGL V7600/V8600/V8650.\n\t"
	    "RV610  : Radeon HD 2350, HD 2400 Pro/XT, HD 2400 Pro AGP; FireGL V4000.\n\t"
	    "RV630  : Radeon HD 2600 Pro/XT, HD 2600 Pro/XT AGP; Gemini RV630; FireGL V3600/V5600.\n\t"
	    "M52    : Mobility Radeon X1300.\n\t"
	    "M54    : Mobility Radeon X1400; M54-GL.\n\t"
	    "M56    : Mobility Radeon X1600; Mobility FireGL V5200.\n\t"
	    "M58    : Mobility Radeon X1800, X1800 XT; Mobility FireGL V7100, V7200.\n\t"
	    "M62    : Mobility Radeon X1350.\n\t"
	    "M64    : Mobility Radeon X1450, X2300.\n");
    xf86Msg(X_NONE, "\t"
	    "M66    : Mobility Radeon X1700, X1700 XT; FireGL V5250.\n\t"
	    "M68    : Mobility Radeon X1900.\n\t"
	    "M71    : Mobility Radeon HD 2300.\n\t"
	    "M72    : Mobility Radeon HD 2400; Radeon E2400.\n\t"
	    "M74    : Mobility Radeon HD 2400 XT.\n\t"
	    "M76    : Mobility Radeon HD 2600; (Gemini ATI) Mobility Radeon HD 2600 XT.\n\t"
	    "RS690  : Radeon X1200.\n\t"
	    "RS740  : RS740, RS740M\n");
    xf86Msg(X_NONE, "\n");
}

#if defined(USE_ID_CONNECTORS) || !defined(ATOM_BIOS)
/*
 * Some macros to help us make connector tables less messy.
 * There are, after all, a limited number of possibilities at the moment.
 */

#define VGA_A0_DVI_BA10 \
  { {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_1, RHD_HPD_0, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_TMDSA}}}

#define VGA_A0_DVI_BB10 \
  { {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_1, RHD_HPD_0, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_LVTMA}}}

#define VGA_A0_DVI_BB11 \
  { {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_1, RHD_HPD_1, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_LVTMA}}}

#define VGA_B1_DVI_AA00 \
  { {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_1, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_0, RHD_HPD_0, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_TMDSA}}}

#define VGA_B1_DVI_AB01 \
  { {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_1, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_0, RHD_HPD_1, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_LVTMA}}}

#define VGA_B1_DVI_AB00 \
  { {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_1, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_0, RHD_HPD_0, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_LVTMA}}}

#define DVI_AA00_DVI_BB11 \
  { {RHD_CONNECTOR_DVI, "DVI-I 1", RHD_DDC_0, RHD_HPD_0, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_TMDSA}}, \
    {RHD_CONNECTOR_DVI, "DVI-I 2", RHD_DDC_1, RHD_HPD_1, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_LVTMA}}}

#define DVI_BA10_DVI_AB01 \
  { {RHD_CONNECTOR_DVI, "DVI-I 1", RHD_DDC_1, RHD_HPD_0, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_TMDSA}}, \
    {RHD_CONNECTOR_DVI, "DVI-I 2", RHD_DDC_0, RHD_HPD_1, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_LVTMA}}}

#define DVI_BB11_DVI_AA00 \
  { {RHD_CONNECTOR_DVI, "DVI-I 1", RHD_DDC_1, RHD_HPD_1, \
       { RHD_OUTPUT_DACB, RHD_OUTPUT_LVTMA}}, \
    {RHD_CONNECTOR_DVI, "DVI-I 2", RHD_DDC_0, RHD_HPD_0, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_TMDSA}}}

#define PANEL_B_VGA_A0 \
  { {RHD_CONNECTOR_PANEL, "Panel", RHD_DDC_NONE, RHD_HPD_NONE, \
       { RHD_OUTPUT_LVTMA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}}

#define PANEL_B1_VGA_A0 \
  { {RHD_CONNECTOR_PANEL, "Panel", RHD_DDC_1, RHD_HPD_NONE, \
       { RHD_OUTPUT_LVTMA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}}

#define PANEL_B1_VGA_A2 \
  { {RHD_CONNECTOR_PANEL, "Panel", RHD_DDC_1, RHD_HPD_NONE, \
       { RHD_OUTPUT_LVTMA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_2, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}}

#define PANEL_B2_VGA_A0 \
  { {RHD_CONNECTOR_PANEL, "Panel", RHD_DDC_2, RHD_HPD_NONE, \
       { RHD_OUTPUT_LVTMA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}}

#define PANEL_B2_DVI_AA00 \
  { {RHD_CONNECTOR_PANEL, "Panel", RHD_DDC_2, RHD_HPD_NONE, \
       { RHD_OUTPUT_LVTMA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-I", RHD_DDC_0, RHD_HPD_0, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_TMDSA}}}

#define PANEL_B2_VGA_A0_DVI_A10 \
  { {RHD_CONNECTOR_PANEL, "Panel", RHD_DDC_2, RHD_HPD_NONE, \
       { RHD_OUTPUT_LVTMA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_VGA, "VGA", RHD_DDC_0, RHD_HPD_NONE, \
       { RHD_OUTPUT_DACA, RHD_OUTPUT_NONE}}, \
    {RHD_CONNECTOR_DVI, "DVI-D", RHD_DDC_1, RHD_HPD_0, \
       { RHD_OUTPUT_TMDSA, RHD_OUTPUT_NONE}}}

#else /* if !defined(USE_ID_CONNECTORS) && defined(ATOM_BIOS) */

#define ID_CONNECTORINFO_EMPTY \
     { {RHD_CONNECTOR_NONE, "NULL", RHD_DDC_NONE, RHD_HPD_NONE, \
       { RHD_OUTPUT_NONE, RHD_OUTPUT_NONE}}}

#define VGA_A0_DVI_BA10         ID_CONNECTORINFO_EMPTY
#define VGA_A0_DVI_BB10         ID_CONNECTORINFO_EMPTY
#define VGA_A0_DVI_BB11         ID_CONNECTORINFO_EMPTY
#define VGA_B1_DVI_AA00         ID_CONNECTORINFO_EMPTY
#define VGA_B1_DVI_AB01         ID_CONNECTORINFO_EMPTY
#define VGA_B1_DVI_AB00         ID_CONNECTORINFO_EMPTY
#define DVI_AA00_DVI_BB11       ID_CONNECTORINFO_EMPTY
#define DVI_BA10_DVI_AB01       ID_CONNECTORINFO_EMPTY
#define DVI_BB11_DVI_AA00       ID_CONNECTORINFO_EMPTY
#define PANEL_B_VGA_A0          ID_CONNECTORINFO_EMPTY
#define PANEL_B1_VGA_A0         ID_CONNECTORINFO_EMPTY
#define PANEL_B1_VGA_A2         ID_CONNECTORINFO_EMPTY
#define PANEL_B2_VGA_A0         ID_CONNECTORINFO_EMPTY
#define PANEL_B2_DVI_AA00       ID_CONNECTORINFO_EMPTY
#define PANEL_B2_VGA_A0_DVI_A10 ID_CONNECTORINFO_EMPTY

#endif /* if defined(USE_ID_CONNECTORS) || !defined(ATOM_BIOS) */

/*
 * List of pci subsystem / card ids.
 *
 * Used for:
 * - printing card name.
 * - connector mapping.
 *
 */
static struct rhdCard
rhdCards[] =
{
    /* 0x7100 : R520 : Radeon X1800 */
    { 0x7100, 0x1002, 0x0B12, "Powercolor X1800XT", DVI_BA10_DVI_AB01 },
    /* 0x7101 : M58 : Mobility Radeon X1800 XT */
    /* 0x7102 : M58 : Mobility Radeon X1800 */
    /* 0x7103 : M58 : Mobility FireGL V7200 */
    /* 0x7104 : R520 : FireGL V7200 */
    { 0x7104, 0x1002, 0x0B32, "ATI FireGL V7200 RH", DVI_BA10_DVI_AB01 },
    /* 0x7105 : R520 : FireGL V5300 */
    /* 0x7106 : M58 : Mobility FireGL V7100 */
    /* 0x7108 : R520 : Radeon X1800 */
    /* 0x7109 : R520 : Radeon X1800 */
    /* 0x710A : R520 : Radeon X1800 */
    /* 0x710B : R520 : Radeon X1800 */
    /* 0x710C : R520 : Radeon X1800 */
    /* 0x710E : R520 : FireGL V7300 */
    /* 0x710F : R520 : FireGL V7350 */
    /* 0x7140 : RV515 : Radeon X1600 */
    /* 0x7141 : RV505 : RV505 */
    /* 0x7142 : RV515 : Radeon X1300/X1550 */
    /* 0x7143 : RV505 : Radeon X1550 */
    /* 0x7144 : M54 : M54-GL */
    /* 0x7145 : M54 : Mobility Radeon X1400 */
    { 0x7145, 0x1028, 0x2002, "Dell Inspiron 9400", PANEL_B2_VGA_A0_DVI_A10 },
    { 0x7145, 0x1028, 0x2003, "Dell Inspiron 6400", PANEL_B_VGA_A0 },
    { 0x7145, 0x1179, 0xFF10, "Toshiba Satellite A100-773", PANEL_B1_VGA_A2 },
    { 0x7145, 0x17AA, 0x2006, "Lenovo Thinkpad T60 (2007)", PANEL_B2_VGA_A0_DVI_A10 },
    { 0x7145, 0x17AA, 0x202A, "Lenovo Thinkpad Z61m", PANEL_B2_VGA_A0 },
    /* 0x7146 : RV515 : Radeon X1300/X1550 */
    { 0x7146, 0x174B, 0x0470, "Sapphire X1300", VGA_B1_DVI_AB01 },
    /* 0x7147 : RV505 : Radeon X1550 64-bit */
    { 0x7147, 0x174B, 0x0840, "Sapphire X1550", VGA_A0_DVI_BB11 },
    /* 0x7149 : M52 : Mobility Radeon X1300 */
    { 0x7149, 0x1028, 0x2003, "Dell Inspiron E1505", PANEL_B_VGA_A0 },
    { 0x7149, 0x17AA, 0x2005, "Lenovo Thinkpad T60 (2008)", PANEL_B2_VGA_A0_DVI_A10 },
    /* 0x714A : M52 : Mobility Radeon X1300 */
    /* 0x714B : M52 : Mobility Radeon X1300 */
    /* 0x714C : M52 : Mobility Radeon X1300 */
    /* 0x714D : RV515 : Radeon X1300 */
    /* 0x714E : RV515 : Radeon X1300 */
    /* 0x714F : RV505 : RV505 */
    /* 0x7151 : RV505 : RV505 */
    /* 0x7152 : RV515 : FireGL V3300 */
    { 0x7152, 0x1002, 0x0B02, "ATI FireGL V3300", DVI_BB11_DVI_AA00 },
    /* 0x7153 : RV515 : FireGL V3350 */
    /* 0x715E : RV515 : Radeon X1300 */
    /* 0x715F : RV505 : Radeon X1550 64-bit */
    /* 0x7180 : RV516 : Radeon X1300/X1550 */
    /* 0x7181 : RV516 : Radeon X1600 */
    /* 0x7183 : RV516 : Radeon X1300/X1550 */
    /* 0x7186 : M64 : Mobility Radeon X1450 */
    /* 0x7187 : RV516 : Radeon X1300/X1550 */
    /* 0x7188 : M64 : Mobility Radeon X2300 */
    /* 0x718A : M64 : Mobility Radeon X2300 */
    /* 0x718B : M62 : Mobility Radeon X1350 */
    /* 0x718C : M62 : Mobility Radeon X1350 */
    /* 0x718D : M64 : Mobility Radeon X1450 */
    /* 0x718F : RV516 : Radeon X1300 */
    /* 0x7193 : RV516 : Radeon X1550 */
    /* 0x7196 : M62 : Mobility Radeon X1350 */
    /* 0x719B : RV516 : FireMV 2250 */
    /* 0x719F : RV516 : Radeon X1550 64-bit */
    /* 0x71C0 : RV530 : Radeon X1600 */
    /* 0x71C1 : RV535 : Radeon X1650 */
    { 0x71C1, 0x174B, 0x0840, "Sapphire X1650 Pro", DVI_AA00_DVI_BB11 },
    /* 0x71C2 : RV530 : Radeon X1600 */
    { 0x71C2, 0x17EE, 0x71C0, "Connect3D Radeon X1600 Pro", VGA_B1_DVI_AA00 },
    /* 0x71C3 : RV535 : Radeon X1600 */
    /* 0x71C4 : M56 : Mobility FireGL V5200 */
    { 0x71C4, 0x17AA, 0x2007, "Lenovo Thinkpad T60p", PANEL_B2_VGA_A0 },
    /* 0x71C5 : M56 : Mobility Radeon X1600 */
    { 0x71C5, 0x103C, 0x30A3, "HP/Compaq nc8430", PANEL_B1_VGA_A0 },
    { 0x71C5, 0x103C, 0x30B4, "HP/Compaq nw8440", PANEL_B1_VGA_A0 },
    { 0x71C5, 0x1043, 0x10B2, "Asus W3J/Z96", PANEL_B_VGA_A0 },
    { 0x71C5, 0x106B, 0x0080, "Macbook Pro", PANEL_B2_DVI_AA00 },
    { 0x71C5, 0x1179, 0xFF10, "Toshiba Satellite A100-237", PANEL_B1_VGA_A2 },
    /* 0x71C6 : RV530 : Radeon X1650 */
    { 0x71C6, 0x174B, 0x0850, "Sapphire X1650 Pro AGP", VGA_A0_DVI_BA10 },
    { 0x71C6, 0x1462, 0x0400, "MSI RX1650 Pro", DVI_BA10_DVI_AB01 },
    /* 0x71C7 : RV535 : Radeon X1650 */
    { 0x71C7, 0x1043, 0x01B6, "Asus EAX1650 Silent", VGA_A0_DVI_BB10 },
    /* 0x71CD : RV530 : Radeon X1600 */
    /* 0x71CE : RV530 : Radeon X1300 XT/X1600 Pro */
    /* 0x71D2 : RV530 : FireGL V3400 */
    { 0x71D2, 0x1002, 0x2B02, "ATI FireGL V3400", DVI_BB11_DVI_AA00 },
    /* 0x71D4 : M66 : Mobility FireGL V5250 */
    { 0x71D4, 0x17AA, 0x20A4, "Lenovo T60p", PANEL_B2_VGA_A0 },
    /* 0x71D5 : M66 : Mobility Radeon X1700 */
    /* 0x71D6 : M66 : Mobility Radeon X1700 XT */
    /* 0x71DA : RV530 : FireGL V5200 */
    /* 0x71DE : M66 : Mobility Radeon X1700 */
    /* 0x7200 : RV550 : Radeon X2300HD */
    /* 0x7210 : M71 : Mobility Radeon HD 2300 */
    /* 0x7211 : M71 : Mobility Radeon HD 2300 */
    /* 0x7240 : R580 : Radeon X1950 */
    /* 0x7243 : R580 : Radeon X1900 */
    /* 0x7244 : R580 : Radeon X1950 */
    /* 0x7245 : R580 : Radeon X1900 */
    /* 0x7246 : R580 : Radeon X1900 */
    /* 0x7247 : R580 : Radeon X1900 */
    /* 0x7248 : R580 : Radeon X1900 */
    /* 0x7249 : R580 : Radeon X1900 */
    { 0x7249, 0x1002, 0x0B12, "ATI Radeon X1900 XTX", DVI_BA10_DVI_AB01 },
    /* 0x724A : R580 : Radeon X1900 */
    /* 0x724B : R580 : Radeon X1900 */
    /* 0x724C : R580 : Radeon X1900 */
    /* 0x724D : R580 : Radeon X1900 */
    /* 0x724E : R580 : AMD Stream Processor */
    /* 0x724F : R580 : Radeon X1900 */
    /* 0x7280 : RV570 : Radeon X1950 */
    { 0x7280, 0x174B, 0xE190, "Sapphire X1950 Pro", DVI_BA10_DVI_AB01 },
    { 0x7280, 0x18BC, 0x2870, "GeCube X1950 Pro", DVI_BA10_DVI_AB01 },
    /* 0x7281 : RV560 : RV560 */
    /* 0x7283 : RV560 : RV560 */
    /* 0x7284 : M68 : Mobility Radeon X1900 */
    /* 0x7287 : RV560 : RV560 */
    /* 0x7288 : RV570 : Radeon X1950 GT */
    { 0x7288, 0x174B, 0xE190, "Sapphire X1950 GT", DVI_BA10_DVI_AB01 },
    /* 0x7289 : RV570 : RV570 */
    /* 0x728B : RV570 : RV570 */
    /* 0x728C : RV570 : ATI FireGL V7400 */
    /* 0x7290 : RV560 : RV560 */
    /* 0x7291 : RV560 : Radeon X1650 */
    /* 0x7293 : RV560 : Radeon X1650 */
    /* 0x7297 : RV560 : RV560 */
    /* 0x791E : RS690 : Radeon X1200 */
    /* 0x791F : RS690 : Radeon X1200 */
    /* 0x796C : RS740 : RS740 */
    /* 0x796D : RS740 : RS740M */
    /* 0x796E : RS740 : RS740 */
    /* 0x796F : RS740 : RS740M */
    /* 0x9400 : R600 : Radeon HD 2900 XT */
    { 0x9400, 0x1002, 0x3142, "Sapphire HD 2900 XT", DVI_BB11_DVI_AA00 },
    /* 0x9401 : R600 : Radeon HD 2900 XT */
    /* 0x9402 : R600 : Radeon HD 2900 XT */
    /* 0x9403 : R600 : Radeon HD 2900 Pro */
    /* 0x9405 : R600 : ATI Radeon HD 2900 GT  */
    /* 0x940A : R600 : ATI FireGL V8650  */
    /* 0x940B : R600 : ATI FireGL V8600  */
    /* 0x940F : R600 : ATI FireGL V7600  */
    /* 0x94C0 : RV610 : RV610 */
    /* 0x94C1 : RV610 : Radeon HD 2400 XT */
    { 0x94C1, 0x174B, 0xE390, "Sapphire HD 2400 XT", VGA_B1_DVI_AB00},
    /* 0x94C3 : RV610 : Radeon HD 2400 Pro */
    { 0x94C3, 0x174B, 0xE370, "Sapphire HD 2400 Pro", VGA_A0_DVI_BB10 },
    /* 0x94C4 : RV610 : ATI Radeon HD 2400 PRO AGP  */
    /* 0x94C5 : RV610 : ATI FireGL V4000  */
    /* 0x94C6 : RV610 : RV610  */
    /* 0x94C7 : RV610 : ATI Radeon HD 2350 */
    /* 0x94C8 : M74 : Mobility Radeon HD 2400 XT */
    /* 0x94C9 : M72 : Mobility Radeon HD 2400 */
    /* 0x94CB : M72 : ATI RADEON E2400 */
    /* 0x94CC : RV610 : RV610  */
    /* 0x9580 : RV630 : RV630 */
    /* 0x9581 : M76 : Mobility Radeon HD 2600 */
    /* 0x9583 : M76 : Mobility Radeon HD 2600 XT */
    /* 0x9586 : RV630 : ATI Radeon HD 2600 XT AGP */
    /* 0x9587 : RV630 : ATI Radeon HD 2600 Pro AGP */
    /* 0x9588 : RV630 : Radeon HD 2600 XT */
    { 0x9588, 0x1002, 0x2542, "ATI Radeon HD 2600XT DDR4", DVI_BA10_DVI_AB01 },
    { 0x9588, 0x174B, 0x2E42, "Sapphire HD 2600 XT", DVI_BA10_DVI_AB01 },
    /* 0x9589 : RV630 : Radeon HD 2600 Pro */
    { 0x9589, 0x174B, 0xE410, "Sapphire HD 2600 Pro", DVI_BA10_DVI_AB01 },
    /* 0x958A : RV630 : Gemini RV630 */
    /* 0x958B : M76 : Gemini ATI Mobility Radeon HD 2600 XT */
    /* 0x958C : RV630 : ATI FireGL V5600  */
    /* 0x958D : RV630 : ATI FireGL V3600  */
    { 0, 0, 0, NULL } /* KEEP THIS: End marker. */
};

/*
 *
 */
struct rhdCard *
RHDCardIdentify(ScrnInfoPtr pScrn)
{
    pciVideoPtr Pci = RHDPTR(pScrn)->PciInfo;
    int i;

    for (i = 0; rhdCards[i].name; i++)
	if ((rhdCards[i].device == Pci->chipType) &&
	    (rhdCards[i].card_vendor == Pci->subsysVendor) &&
	    (rhdCards[i].card_device == Pci->subsysCard))
	    return rhdCards + i;

    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "Unknown card detected: 0x%04X:0x%04X:0x%04X.\n",
	       Pci->chipType, Pci->subsysVendor, Pci->subsysCard);
    xf86Msg(X_NONE, "\t"
	    "Your card might not work or might not work optimally.\n\t"
	    "To rectify this, please contact radeonhd@opensuse.org.\n\t"
	    "Include your X log, the full name of the device\n\t"
	    "and the conntest (found in utils/conntest) output.\n");

    return NULL;
}
