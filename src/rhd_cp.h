#ifndef _RHD_CP_H
# define _RHD_CP_H


# if USE_DRI

struct rhdCP {
    Bool              CPRuns;           /* CP is running */
    Bool              CPInUse;          /* CP has been used by X server */
    Bool              CPStarted;        /* CP has started */
    int               CPMode;           /* CP mode that server/clients use */
    int               CPFifoSize;       /* Size of the CP command FIFO */
    int               CPusecTimeout;    /* CP timeout in usecs */
    Bool              needCacheFlush;

				/* CP accleration */
    drmBufPtr         indirectBuffer;
    int               indirectStart;

    /* Debugging info for BEGIN_RING/ADVANCE_RING pairs. */
    int               dma_begin_count;
    char              *dma_debug_func;
    int               dma_debug_lineno;

};

#  ifdef USE_XAA
/* radeon_accelfuncs.c */
extern void RADEONAccelInitCP(ScreenPtr pScreen, XAAInfoRecPtr a);
#  endif

#  define RADEONCP_START(pScrn, info)					\
do {									\
    int _ret = drmCommandNone(info->dri->drmFD, DRM_RADEON_CP_START);	\
    if (_ret) {								\
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,				\
		   "%s: CP start %d\n", __FUNCTION__, _ret);		\
    }									\
    info->cp->CPStarted = TRUE;                                         \
} while (0)

#  define RADEONCP_RELEASE(pScrn, info)					\
do {									\
    if (info->cp->CPInUse) {						\
	RADEON_PURGE_CACHE(info);						\
	RADEON_WAIT_UNTIL_IDLE(info);					\
	RADEONCPReleaseIndirect(pScrn);					\
	info->cp->CPInUse = FALSE;					\
    }									\
} while (0)

#  define RADEONCP_STOP(pScrn, info)					\
do {									\
    int _ret;								\
     if (info->cp->CPStarted) {						\
        _ret = RADEONCPStop(pScrn, info);				\
        if (_ret) {							\
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,			\
		   "%s: CP stop %d\n", __FUNCTION__, _ret);		\
        }								\
        info->cp->CPStarted = FALSE;                                    \
   }									\
    RADEONEngineRestore(pScrn);						\
    info->cp->CPRuns = FALSE;						\
} while (0)

#  define RADEONCP_RESET(pScrn, info)					\
do {									\
    if (RADEONCP_USE_RING_BUFFER(info->cp->CPMode)) {			\
	int _ret = drmCommandNone(info->dri->drmFD, DRM_RADEON_CP_RESET);	\
	if (_ret) {							\
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,			\
		       "%s: CP reset %d\n", __FUNCTION__, _ret);	\
	}								\
    }									\
} while (0)

#  define RADEONCP_REFRESH(pScrn, info)					\
do {									\
    if (!info->cp->CPInUse) {						\
	if (info->cp->needCacheFlush) {					\
	    RADEON_PURGE_CACHE(info);					\
	    RADEON_PURGE_ZCACHE();					\
	    info->cp->needCacheFlush = FALSE;				\
	}								\
	RADEON_WAIT_UNTIL_IDLE(info);					\
            BEGIN_RING(4);                                              \
            OUT_RING_REG(R300_SC_SCISSOR0, info->accel_state->re_top_left);   \
	    OUT_RING_REG(R300_SC_SCISSOR1, info->accel_state->re_width_height);      \
            ADVANCE_RING();                                             \
	info->cp->CPInUse = TRUE;					\
    }									\
} while (0)


#  define CP_PACKET0(reg, n)						\
	(RADEON_CP_PACKET0 | ((n) << 16) | ((reg) >> 2))
#  define CP_PACKET1(reg0, reg1)						\
	(RADEON_CP_PACKET1 | (((reg1) >> 2) << 11) | ((reg0) >> 2))
#  define CP_PACKET2()							\
	(RADEON_CP_PACKET2)
#  define CP_PACKET3(pkt, n)						\
	(RADEON_CP_PACKET3 | (pkt) | ((n) << 16))


#  define RADEON_VERBOSE	0

#  define RING_LOCALS	uint32_t *__head = NULL; int __expected; int __count = 0

#  define BEGIN_RING_INFO(info, n) do {					\
    if (RADEON_VERBOSE) {						\
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,				\
		   "BEGIN_RING(%d) in %s\n", (unsigned int)n, __FUNCTION__);\
    }									\
    if (++info->cp->dma_begin_count != 1) {					\
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,				\
		   "BEGIN_RING without end at %s:%d\n",			\
		   info->cp->dma_debug_func, info->cp->dma_debug_lineno);	\
	info->cp->dma_begin_count = 1;					\
    }									\
    info->cp->dma_debug_func = __FILE__;					\
    info->cp->dma_debug_lineno = __LINE__;					\
    if (!info->cp->indirectBuffer) {					\
	info->cp->indirectBuffer = RADEONCPGetBuffer(pScrn);		\
	info->cp->indirectStart = 0;					\
    } else if (info->cp->indirectBuffer->used + (n) * (int)sizeof(uint32_t) >	\
	       info->cp->indirectBuffer->total) {				\
	RADEONCPFlushIndirect(pScrn, 1);				\
    }									\
    __expected = n;							\
    __head = (pointer)((char *)info->cp->indirectBuffer->address +	\
		       info->cp->indirectBuffer->used);			\
    __count = 0;							\
} while (0)

#  define BEGIN_RING(n) BEGIN_RING_INFO(info, n)

#  define ADVANCE_RING_INFO(info) do {						\
    if (info->cp->dma_begin_count-- != 1) {				\
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,				\
		   "ADVANCE_RING without begin at %s:%d\n",		\
		   __FILE__, __LINE__);					\
	info->cp->dma_begin_count = 0;					\
    }									\
    if (__count != __expected) {					\
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,				\
		   "ADVANCE_RING count != expected (%d vs %d) at %s:%d\n", \
		   __count, __expected, __FILE__, __LINE__);		\
    }									\
    if (RADEON_VERBOSE) {						\
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,				\
		   "ADVANCE_RING() start: %d used: %d count: %d\n",	\
		   info->cp->indirectStart,				\
		   info->cp->indirectBuffer->used,			\
		   __count * (int)sizeof(uint32_t));			\
    }									\
    info->cp->indirectBuffer->used += __count * (int)sizeof(uint32_t);	\
} while (0)

#  define ADVANCE_RING() ADVANCE_RING_INFO(info)

#  define OUT_RING(x) do {						\
    if (RADEON_VERBOSE) {						\
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,				\
		   "   OUT_RING(0x%08x)\n", (unsigned int)(x));		\
    }									\
    __head[__count++] = (x);						\
} while (0)

#  define OUT_RING_REG(reg, val)						\
do {									\
    OUT_RING(CP_PACKET0(reg, 0));					\
    OUT_RING(val);							\
} while (0)

#  define FLUSH_RING()							\
do {									\
    if (RADEON_VERBOSE)							\
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,				\
		   "FLUSH_RING in %s\n", __FUNCTION__);			\
    if (info->cp->indirectBuffer) {					\
	RADEONCPFlushIndirect(pScrn, 0);				\
    }									\
} while (0)


#  define RADEON_WAIT_UNTIL_2D_IDLE()					\
do {									\
    BEGIN_RING(2);							\
    OUT_RING(CP_PACKET0(RADEON_WAIT_UNTIL, 0));				\
    OUT_RING((RADEON_WAIT_2D_IDLECLEAN |				\
	      RADEON_WAIT_HOST_IDLECLEAN));				\
    ADVANCE_RING();							\
} while (0)

#  define RADEON_WAIT_UNTIL_3D_IDLE()					\
do {									\
    BEGIN_RING(2);							\
    OUT_RING(CP_PACKET0(RADEON_WAIT_UNTIL, 0));				\
    OUT_RING((RADEON_WAIT_3D_IDLECLEAN |				\
	      RADEON_WAIT_HOST_IDLECLEAN));				\
    ADVANCE_RING();							\
} while (0)

#  define RADEON_WAIT_UNTIL_IDLE(info)					\
do {									\
    if (RADEON_VERBOSE) {						\
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,				\
		   "WAIT_UNTIL_IDLE() in %s\n", __FUNCTION__);		\
    }									\
    BEGIN_RING_INFO(info,2);						\
    OUT_RING(CP_PACKET0(RADEON_WAIT_UNTIL, 0));				\
    OUT_RING((RADEON_WAIT_2D_IDLECLEAN |				\
	      RADEON_WAIT_3D_IDLECLEAN |				\
	      RADEON_WAIT_HOST_IDLECLEAN));				\
    ADVANCE_RING_INFO(info);							\
} while (0)

#  define RADEON_PURGE_CACHE(info)						\
do {									\
    BEGIN_RING_INFO(info,2);						\
        OUT_RING(CP_PACKET0(R300_RB3D_DSTCACHE_CTLSTAT, 0));		\
        OUT_RING(R300_RB3D_DC_FLUSH_ALL);				\
    ADVANCE_RING_INFO(info);							\
} while (0)

#  define RADEON_PURGE_ZCACHE()						\
do {									\
    BEGIN_RING(2);							\
        OUT_RING(CP_PACKET0(R300_RB3D_ZCACHE_CTLSTAT, 0));		\
        OUT_RING(R300_ZC_FLUSH_ALL);					\
    ADVANCE_RING();							\
} while (0)

# endif /* USE_DRI */

static __inline__ void RADEON_MARK_SYNC(RHDPtr info, ScrnInfoPtr pScrn)
{
# ifdef USE_EXA
    if (info->useEXA)
	exaMarkSync(pScrn->pScreen);
# endif
# ifdef USE_XAA
    if (!info->useEXA)
	SET_SYNC_FLAG(info->accel);
# endif
}

static __inline__ void RADEON_SYNC(RHDPtr info, ScrnInfoPtr pScrn)
{
# ifdef USE_EXA
    if (info->useEXA)
	exaWaitSync(pScrn->pScreen);
# endif
# ifdef USE_XAA
    if (!info->useEXA && info->accel)
	info->accel->Sync(pScrn);
# endif
}

#endif
