/* k1lzma.c -- LzmaLib's LzmaUncompress, for K1's MacDecompress (k1port.h).
 *
 * K1 ships its BIFs LZMA-compressed (the main OBB's data/ .bzf files) and libandroid_port.so's
 * MacDecompress decodes each resource with libLzmaLib.so's LzmaUncompress: 5 property bytes, then
 * the raw LZMA stream. That is LzmaLib.c's body, over the LZMA SDK's LzmaDec.c (public domain). */
#include <stdlib.h>

#include "LzmaDec.h"
#include "k1port.h"

static void* Alloc(ISzAllocPtr p, size_t n) { (void)p; return malloc(n); }
static void Free(ISzAllocPtr p, void* a) { (void)p; free(a); }
static const ISzAlloc g_alloc = { Alloc, Free };

int k1lzma_uncompress(unsigned char* pDst, size_t* pnDst, const unsigned char* pSrc, size_t* pnSrc,
                      const unsigned char* pProps, size_t nProps)
{
    ELzmaStatus status;
    SizeT nDst = *pnDst, nSrc = *pnSrc;
    SRes r = LzmaDecode(pDst, &nDst, pSrc, &nSrc, pProps, (unsigned)nProps, LZMA_FINISH_ANY, &status,
                        &g_alloc);
    *pnDst = nDst;
    *pnSrc = nSrc;
    if (r == SZ_OK && status == LZMA_STATUS_NEEDS_MORE_INPUT) return SZ_ERROR_INPUT_EOF;
    return r;
}
