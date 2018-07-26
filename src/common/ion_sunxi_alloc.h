#ifndef __ION_SUNXI_ALLOC__
#define __ION_SUNXI_ALLOC__

#include "ion.h"
#include "ion_sunxi.h"

#define MAX_ION_BLOCK 16

int  sunxi_alloc_open(void);
int  sunxi_alloc_close(void);
int  sunxi_alloc_alloc(int size);
int  sunxi_alloc_free(void * pbuf);
int  sunxi_alloc_vir2phy(void * pbuf);
int  sunxi_alloc_phy2vir(void * pbuf);
void sunxi_flush_cache(void* startAddr, int size);

#endif
