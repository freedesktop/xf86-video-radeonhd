/*
 * Copyright 2007, 2008  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007, 2008  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007, 2008  Egbert Eich   <eich@novell.com>
 * Copyright 2007, 2008  Advanced Micro Devices, Inc.
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

/* for usleep */
#if HAVE_XF86_ANSIC_H
# include "xf86_ansic.h"
#else
# include <unistd.h>
# include <string.h>
# include <stdio.h>
#endif

#include "rhd.h"
#include "rhd_connector.h"
#include "rhd_output.h"
#include "rhd_crtc.h"
#include "rhd_atombios.h"
#include "rhd_biosscratch.h"

struct rhdAtomOutputPrivate {
    Bool Stored;

    struct atomCodeTableVersion EncoderVersion;
    struct atomCodeTableVersion CrtcSourceVersion;
    struct atomEncoderConfig EncoderConfig;
    enum atomEncoder EncoderId;

    struct atomTransmitterConfig TransmitterConfig;
    enum atomTransmitter TransmitterId;

    enum atomOutput ControlId;

    Bool   RunDualLink;
    int    PixelClock;

    CARD16 PowerDigToDE;
    CARD16 PowerDEToBL;
    CARD16 OffDelay;
    Bool   TemporalDither;
    Bool   SpatialDither;
    int    GreyLevel;
    Bool   DualLink;
    Bool   LVDS24Bit;
    Bool   FPDI;

    Bool   Coherent;
};

/*
 *
 */
static enum rhdSensedOutput
rhdAtomDACSense(struct rhdOutput *Output, enum rhdConnectorType Type)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    enum atomDAC DAC;

    switch (Output->Id) {
	case RHD_OUTPUT_DACA:
	    DAC = atomDACA;
	    break;
	case RHD_OUTPUT_DACB:
	    DAC = atomDACB;
	    break;
	default:
	    return FALSE;
    }

    if (! AtomDACLoadDetection(rhdPtr->atomBIOS, Output->OutputDriverPrivate->Device, DAC))
	return RHD_SENSED_NONE;
    return rhdAtomBIOSScratchDACSenseResults(Output, DAC);
}

static inline void
rhdSetEncoderTransmitterConfig(struct rhdOutput *Output, int PixelClock)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    struct rhdAtomOutputPrivate *Private = (struct rhdAtomOutputPrivate *) Output->Private;
    struct atomEncoderConfig *EncoderConfig = &Private->EncoderConfig;
    struct atomTransmitterConfig *TransmitterConfig = &Private->TransmitterConfig;

    RHDFUNC(Output);

    EncoderConfig->PixelClock = TransmitterConfig->PixelClock = PixelClock;

    switch (Output->Id) {
	case RHD_OUTPUT_NONE:
	case RHD_OUTPUT_DVO:
	    xf86DrvMsg(Output->scrnIndex, X_ERROR, "%s: unhandled output\n", __func__);
	    break; /* we don't handle these yet */
	case RHD_OUTPUT_DACA:
	case RHD_OUTPUT_DACB:
	    switch (Output->SensedType) {
		case RHD_SENSED_VGA:
		    EncoderConfig->u.dac.Standard = atomDAC_VGA;
		    break;
		case RHD_SENSED_TV_COMPONENT:
		    EncoderConfig->u.dac.Standard = atomDAC_CV;
		    break;
		case RHD_SENSED_TV_SVIDEO:
		case RHD_SENSED_TV_COMPOSITE:
		    switch (rhdPtr->tvMode) {
			case RHD_TV_NTSC:
			case RHD_TV_NTSCJ:
			    EncoderConfig->u.dac.Standard = atomDAC_NTSC;
			    /* NTSC */
			    break;
			case RHD_TV_PAL:
			case RHD_TV_PALN:
			case RHD_TV_PALCN:
			case RHD_TV_PAL60:
			default:
			    EncoderConfig->u.dac.Standard = atomDAC_PAL;
			    /* PAL */
			    break;
		    }
		    break;
		default:
		    xf86DrvMsg(Output->scrnIndex, X_ERROR, "Sensed incompatible output for DAC\n");
		    EncoderConfig->u.dac.Standard = atomDAC_VGA;
		    break;
	    }
	    break;

	case RHD_OUTPUT_TMDSA:
	case RHD_OUTPUT_LVTMA:
	    if (Output->Connector->Type == RHD_CONNECTOR_DVI) {
		Private->RunDualLink = (PixelClock > 165000) ? TRUE : FALSE;
	    }
	    switch (Private->EncoderVersion.cref) {
		case 1:
		    if (Private->RunDualLink)
			EncoderConfig->u.lvds.LinkCnt = atomDualLink;
		    else
			EncoderConfig->u.lvds.LinkCnt = atomSingleLink;
		    break;
		case 2:
		    if (Private->RunDualLink)
			EncoderConfig->u.lvds2.LinkCnt = atomDualLink;
		    else
			EncoderConfig->u.lvds2.LinkCnt = atomSingleLink;
		    if (Private->Coherent)
			EncoderConfig->u.lvds2.Coherent = TRUE;
		    else
			EncoderConfig->u.lvds2.Coherent = FALSE;
		    break;
	    }
	    break;

	case RHD_OUTPUT_KLDSKP_LVTMA:
	case RHD_OUTPUT_UNIPHYA:
	case RHD_OUTPUT_UNIPHYB:
	    if (Output->Connector->Type == RHD_CONNECTOR_DVI
#if 0
		|| Output->Connector->Type == RHD_CONNECTOR_DP_DUAL
		|| Output->Connector->Type == RHD_CONNECTOR_HDMI_B
#endif
		) {
		Private->RunDualLink = (PixelClock > 165000) ? TRUE : FALSE;
	    }

	    if (Private->RunDualLink) {
		TransmitterConfig->LinkCnt = EncoderConfig->u.dig.LinkCnt = atomDualLink;
		TransmitterConfig->Link = atomTransLinkAB;
	    } else
		TransmitterConfig->LinkCnt = EncoderConfig->u.dig.LinkCnt = atomSingleLink;
	    
	    TransmitterConfig->Coherent = Private->Coherent;
	    break;
    }
}

/*
 *
 */
static inline void
rhdAtomOutputSet(struct rhdOutput *Output, DisplayModePtr Mode)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    struct rhdAtomOutputPrivate *Private = (struct rhdAtomOutputPrivate *) Output->Private;
    struct atomEncoderConfig *EncoderConfig = &Private->EncoderConfig;
/*     struct atomTransmitterConfig *TransmitterConfig = &Private->TransmitterConfig; */
    struct atomCrtcSourceConfig CrtcSourceConfig;

    RHDFUNC(Output);

    Private->PixelClock = Mode->SynthClock;
    rhdSetEncoderTransmitterConfig(Output, Private->PixelClock);

    switch ( Private->CrtcSourceVersion.cref){
	case 1:
	    CrtcSourceConfig.u.Device = Output->OutputDriverPrivate->Device;
	    break;
	case 2:
	    CrtcSourceConfig.u.crtc2.Encoder = Private->EncoderId;
	    CrtcSourceConfig.u.crtc2.Mode = EncoderConfig->u.dig.EncoderMode;
	    break;
	default:
	    xf86DrvMsg(Output->scrnIndex, X_ERROR,
		       "Unknown version of SelectCrtcSource code table: %i\n",Private->CrtcSourceVersion.cref);
	    return;
    }
    rhdAtomEncoderControl(rhdPtr->atomBIOS,  Private->EncoderId, atomEncoderOn, EncoderConfig);
    RHDAtomUpdateBIOSScratchForOutput(Output);
    rhdAtomSelectCrtcSource(rhdPtr->atomBIOS, Output->Crtc->Id ? atomCrtc2 : atomCrtc1, &CrtcSourceConfig);
}

/*
 *
 */
static inline void
rhdAtomOutputPower(struct rhdOutput *Output, int Power)
{
    RHDPtr rhdPtr = RHDPTRI(Output);
    struct rhdAtomOutputPrivate *Private = (struct rhdAtomOutputPrivate *) Output->Private;

    RHDFUNC(Output);

    RHDAtomUpdateBIOSScratchForOutput(Output);
    rhdSetEncoderTransmitterConfig(Output, Private->PixelClock);

    switch (Power) {
	case RHD_POWER_ON:
	    switch (Output->Id) {
		case RHD_OUTPUT_KLDSKP_LVTMA:
		case RHD_OUTPUT_UNIPHYA:
		case RHD_OUTPUT_UNIPHYB:
		    rhdAtomDigTransmitterControl(rhdPtr->atomBIOS,  Private->TransmitterId,
					      atomTransEnable, &Private->TransmitterConfig);
		    rhdAtomDigTransmitterControl(rhdPtr->atomBIOS,  Private->TransmitterId,
					      atomTransEnableOutput, &Private->TransmitterConfig);
		    break;
		default:
		    rhdAtomOutputControl(rhdPtr->atomBIOS, Private->ControlId, atomOutputEnable);
		    break;
	    }
	    break;
	case RHD_POWER_RESET:
	    switch (Output->Id) {
		case RHD_OUTPUT_KLDSKP_LVTMA:
		case RHD_OUTPUT_UNIPHYA:
		case RHD_OUTPUT_UNIPHYB:
		    rhdAtomDigTransmitterControl(rhdPtr->atomBIOS,  Private->TransmitterId,
					      atomTransDisableOutput, &Private->TransmitterConfig);
		    break;
		default:
		    rhdAtomOutputControl(rhdPtr->atomBIOS, Private->ControlId, atomOutputDisable);
		    break;
	    }
	    break;
	case RHD_POWER_SHUTDOWN:
	    switch (Output->Id) {
		case RHD_OUTPUT_KLDSKP_LVTMA:
		case RHD_OUTPUT_UNIPHYA:
		case RHD_OUTPUT_UNIPHYB:
		    rhdAtomDigTransmitterControl(rhdPtr->atomBIOS,  Private->TransmitterId,
					      atomTransDisableOutput, &Private->TransmitterConfig);
		    rhdAtomDigTransmitterControl(rhdPtr->atomBIOS,  Private->TransmitterId,
					      atomTransDisable, &Private->TransmitterConfig);
		    break;
		default:
		    rhdAtomOutputControl(rhdPtr->atomBIOS, Private->ControlId, atomOutputDisable);
		    break;
	    }
	    rhdAtomEncoderControl(rhdPtr->atomBIOS,  Private->EncoderId, atomEncoderOff, &Private->EncoderConfig);
	    break;
    }
}

/*
 *
 */
static inline void
rhdAtomOutputSave(struct rhdOutput *Output)
{
}

/*
 *
 */
static void
rhdAtomOutputRestore(struct rhdOutput *Output)
{
/*     struct rhdAtomOutputPrivate *Private = (struct rhdAtomOutputPrivate *) Output->Private; */
}

/*
 *
 */
static ModeStatus
rhdAtomOutputModeValid(struct rhdOutput *Output, DisplayModePtr Mode)
{
    RHDFUNC(Output);

    if (Mode->Flags & V_INTERLACE)
	return MODE_NO_INTERLACE;

    if (Mode->Clock < 25000)
	return MODE_CLOCK_LOW;


    if (Output->Connector->Type == RHD_CONNECTOR_DVI_SINGLE
#if 0
		|| Output->Connector->Type == RHD_CONNECTOR_DP_DUAL
		|| Output->Connector->Type == RHD_CONNECTOR_HDMI_B
#endif
	) {
	if (Mode->Clock > 165000)
	    return MODE_CLOCK_HIGH;
    }
    else if (Output->Connector->Type == RHD_CONNECTOR_DVI) {
	if (Mode->Clock > 330000) /* could go higher still */
	    return MODE_CLOCK_HIGH;
    }

    return MODE_OK;
}


/*
 *
 */
static void
rhdAtomOutputDestroy(struct rhdOutput *Output)
{
    RHDFUNC(Output);

    if (!Output->Private)
	return;

    xfree(Output->Private);
    Output->Private = NULL;
}

/*
 *
 */
static Bool
LVDSInfoRetrieve(RHDPtr rhdPtr, struct rhdAtomOutputPrivate *Private)
{
    AtomBiosArgRec data;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_SEQ_DIG_ONTO_DE, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->PowerDigToDE = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_SEQ_DE_TO_BL, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->PowerDEToBL = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_OFF_DELAY, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->OffDelay = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_DUALLINK, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->DualLink = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_24BIT, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->LVDS24Bit = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_FPDI, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->FPDI = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_TEMPORAL_DITHER, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->TemporalDither = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_SPATIAL_DITHER, &data) != ATOM_SUCCESS)
	return FALSE;
    Private->SpatialDither = data.val;

    if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS,
			ATOM_LVDS_GREYLVL, &data) != ATOM_SUCCESS)
	return FALSE;
    {
	Private->GreyLevel = data.val;
	xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "AtomBIOS returned %i Grey Levels\n",
		   Private->GreyLevel);
    }
    Private->Coherent = FALSE;

    return TRUE;
}

/*
 * TMDSInfoRetrieve() - interface to set TMDS (DVI) parameters.
 */
static Bool
TMDSInfoRetrieve(RHDPtr rhdPtr, struct rhdAtomOutputPrivate *Private)
{
    Private->FPDI = FALSE;
    Private->TemporalDither = FALSE;
    Private->SpatialDither = FALSE;
    Private->GreyLevel = 0;
    Private->Coherent = FALSE;

    return TRUE;
}

/*
 *
 */
struct rhdOutput *
RHDAtomOutputInit(RHDPtr rhdPtr, rhdConnectorType ConnectorType,
		  rhdOutputType OutputType)
{
    struct rhdOutput *Output;
    struct rhdAtomOutputPrivate *Private;
    struct atomEncoderConfig *EncoderConfig;
    struct atomTransmitterConfig *TransmitterConfig;

    RHDFUNC(rhdPtr);

    Output = xnfcalloc(sizeof(struct rhdOutput), 1);
    Output->scrnIndex = rhdPtr->scrnIndex;

    ErrorF("FOOBAR\n");
    
    Output->Name = "AtomOutput";
    Output->Id = OutputType;
    Output->Sense = NULL;
    Private = xnfcalloc(sizeof(struct rhdAtomOutputPrivate), 1);
    Output->Private = Private;
    EncoderConfig = &Private->EncoderConfig;

    switch (OutputType) {
        case RHD_OUTPUT_NONE:
	    xfree(Output);
	    xfree(Private);
	    return NULL;
	case RHD_OUTPUT_DACA:
	    Output->Sense = rhdAtomDACSense;
	    Private->EncoderId = atomEncoderDACA;
	    Private->ControlId = atomDAC1Output;
	    break;
	case RHD_OUTPUT_DACB:
	    Output->Sense = rhdAtomDACSense;
	    Private->EncoderId = atomEncoderDACB;
	    Private->ControlId = atomDAC2Output;
	    break;
	case RHD_OUTPUT_TMDSA:
	case RHD_OUTPUT_LVTMA:
	    if (OutputType == RHD_OUTPUT_LVTMA &&
		ConnectorType == RHD_CONNECTOR_PANEL) {
		LVDSInfoRetrieve(rhdPtr, Private);
		Private->RunDualLink = Private->DualLink;
		Private->EncoderId = atomEncoderLVDS;
	    } else {
		Private->EncoderId = atomEncoderTMDS1;
		if (OutputType == RHD_CONNECTOR_DVI)
		    Private->DualLink = TRUE;
		else
		    Private->DualLink = FALSE;

	    }

	    if (OutputType == RHD_OUTPUT_LVTMA)
		Private->ControlId = atomLVTMAOutput;
	    else
		Private->ControlId = atomTMDSAOutput;
	    Private->EncoderVersion = rhdAtomEncoderControlVersion(rhdPtr->atomBIOS, Private->EncoderId);
	    switch (Private->EncoderVersion.cref) {
		case 1:
		    EncoderConfig->u.lvds.Is24bit = Private->LVDS24Bit;
		    break;
		case 2:
		    EncoderConfig->u.lvds2.Is24bit = Private->LVDS24Bit;
		    EncoderConfig->u.lvds2.SpatialDither = Private->SpatialDither;
		    EncoderConfig->u.lvds2.LinkB = 0; /* @@@ */
		    EncoderConfig->u.lvds2.Hdmi = FALSE;
#if 0
		    if (ConnectorType == RHD_CONNECTOR_HDMI_B
			|| ConnectorType == RHD_CONNECTOR_HDMI_A)
			EncoderConfig->u.lvds2.hdmi = TRUE;
#endif
		    switch (Private->GreyLevel) {
			case 2:
			    EncoderConfig->u.lvds2.TemporalGrey = atomTemporalDither2;
			    break;
			case 4:
			    EncoderConfig->u.lvds2.TemporalGrey = atomTemporalDither4;
			    break;
			case 0:
			default:
			    EncoderConfig->u.lvds2.TemporalGrey = atomTemporalDither0;
		    }
		    if (Private->SpatialDither)
			EncoderConfig->u.lvds2.SpatialDither = TRUE;
		    else
			EncoderConfig->u.lvds2.SpatialDither = FALSE;
		    EncoderConfig->u.lvds2.Coherent = Private->Coherent;
		    break;
		case 3:
		    /* for these outputs we should not have v3 */
		    xfree(Output);
		    xfree(Private);
		    return NULL;
	    }
	    break;
	case RHD_OUTPUT_DVO:
	    break;
	case RHD_OUTPUT_KLDSKP_LVTMA:
	    Private->EncoderId = atomEncoderDIG2;
	    Private->EncoderVersion = rhdAtomEncoderControlVersion(rhdPtr->atomBIOS,
								   Private->EncoderId);
	    Private->TransmitterId = atomTransmitterLVTMA;
	    EncoderConfig->u.dig.Link = atomTransLinkB;
	    EncoderConfig->u.dig.Transmitter = atomTransmitterLVTMA;

	    TransmitterConfig = &Private->TransmitterConfig;
	    TransmitterConfig->Link = atomTransLinkB;
	    TransmitterConfig->Encoder =  Private->TransmitterId;

	    if (ConnectorType == RHD_CONNECTOR_PANEL) {
		TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomLVDS;
		LVDSInfoRetrieve(rhdPtr, Private);
	    } else {
		TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomDVI;
		TMDSInfoRetrieve(rhdPtr, Private);
	    }
	    break;

	case RHD_OUTPUT_UNIPHYA:
	    Private->EncoderId = atomEncoderDIG1;
	    EncoderConfig->u.dig.Link = atomTransLinkA;
	    EncoderConfig->u.dig.Transmitter = atomTransmitterUNIPHY;
	    if (RHDIsIGP(rhdPtr->ChipSet))
		Private->TransmitterId = atomTransmitterPCIEPHY;
	    else
		Private->TransmitterId = atomTransmitterUNIPHY;

	    TransmitterConfig = &Private->TransmitterConfig;
	    TransmitterConfig->Link = atomTransLinkA;
	    TransmitterConfig->Encoder =  Private->TransmitterId;
	    if (RHDIsIGP(rhdPtr->ChipSet)) {
		AtomBiosArgRec data;
		data.val = 1;
		if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS, ATOM_GET_PCIE_LANES,
				    &data) == ATOM_SUCCESS)
		    TransmitterConfig->Lanes = data.pcieLanes.Chassis;
		/* only do 'chassis' for now */
		else {
		    xfree(Private);
		    xfree(Output);
		    return NULL;
		}
	    }

	    if (ConnectorType == RHD_CONNECTOR_PANEL)
		LVDSInfoRetrieve(rhdPtr, Private);
	    else
		TMDSInfoRetrieve(rhdPtr, Private);
	    break;
	    switch (ConnectorType) {
		case RHD_CONNECTOR_DVI:
		case RHD_CONNECTOR_DVI_SINGLE:
		    TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomDVI;
		    break;
#if 0
		case RHD_CONNECTOR_DP:
		case RHD_CONNECTOR_DP_DUAL:
		    TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomDP;
		    break;
		case RHD_CONNECTOR_HDMI_A:
		case RHD_CONNECTOR_HDMI_B:
		    TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomHDMI;
		    break;
#endif
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unknown connector type\n");
		    xfree(Output);
		    xfree(Private);
		    return NULL;
	    }
	    break;
	case RHD_OUTPUT_UNIPHYB:
	    Private->EncoderId = atomEncoderDIG2;
	    EncoderConfig->u.dig.Link = atomTransLinkB;
	    EncoderConfig->u.dig.Transmitter = atomTransmitterUNIPHY;
	    if (RHDIsIGP(rhdPtr->ChipSet))
		Private->TransmitterId = atomTransmitterPCIEPHY;
	    else
		Private->TransmitterId = atomTransmitterUNIPHY;

	    TransmitterConfig = &Private->TransmitterConfig;
	    TransmitterConfig->Link = atomTransLinkB;
	    TransmitterConfig->Encoder =  Private->TransmitterId;
	    if (RHDIsIGP(rhdPtr->ChipSet)) {
		AtomBiosArgRec data;
		data.val = 1;
		if (RHDAtomBiosFunc(rhdPtr->scrnIndex, rhdPtr->atomBIOS, ATOM_GET_PCIE_LANES,
				    &data) == ATOM_SUCCESS)
		    TransmitterConfig->Lanes = data.pcieLanes.Chassis;
		/* only do 'chassis' for now */
		else {
		    xfree(Private);
		    xfree(Output);
		    return NULL;
		}
	    }

	    switch (ConnectorType) {
		case RHD_CONNECTOR_DVI:
		case RHD_CONNECTOR_DVI_SINGLE:
		    TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomDVI;
		    break;
#if 0
		case RHD_CONNECTOR_DP:
		case RHD_CONNECTOR_DP_DUAL:
		    TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomDP;
		    break;
		case RHD_CONNECTOR_HDMI_A:
		case RHD_CONNECTOR_HDMI_B:
		    TransmitterConfig->Mode = EncoderConfig->u.dig.EncoderMode = atomHDMI;
		    break;
#endif
		default:
		    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unknown connector type\n");
		    xfree(Output);
		    xfree(Private);
		    return NULL;
	    }
	    break;
	default:
	    xf86DrvMsg(rhdPtr->scrnIndex, X_ERROR, "Unknown output type\n");
	    xfree(Output);
	    xfree(Private);
	    return NULL;
    }

    Output->Mode = rhdAtomOutputSet;
    Output->Power = rhdAtomOutputPower;
    Output->Save = rhdAtomOutputSave;
    Output->Restore = rhdAtomOutputRestore;
    Output->ModeValid = rhdAtomOutputModeValid;
    Output->Destroy = rhdAtomOutputDestroy;
    Private->CrtcSourceVersion = rhdAtomSelectCrtcSourceVersion(rhdPtr->atomBIOS);

    return Output;
}