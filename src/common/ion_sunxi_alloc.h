#ifndef __ION_SUNXI_ALLOC__
#define __ION_SUNXI_ALLOC__

#include "ion.h"
#include "ion_sunxi.h"
#include <vector>

using std::vector;

class sunxi_mem_elm{
	ion_user_handle_t handle;
	int fd;
	void *virt_addr;
	void *phys_addr;
	size_t len;
public:
	sunxi_mem_elm(ion_user_handle_t handle, \
			int fd, void *virt_addr, \
			void *phys_addr, size_t len): \
		handle(handle), \
		fd(fd), \
		virt_addr(virt_addr), \
		phys_addr(phys_addr), \
		len(len) \
	{
	};
	void *get_virt_addr(){
		return virt_addr;
	}
	void *get_phys_addr(){
		return phys_addr;
	}
	ion_user_handle_t get_handle(){
		return handle;
	}
	size_t get_len(){
		return len;
	}
};

class sunxi_alloc_cls{
	int fd;
	vector<sunxi_mem_elm> allocations;
public:
	sunxi_alloc_cls();
	~sunxi_alloc_cls();

	void client_open();
	void client_close();
	void *alloc(size_t size);
	void free(void *addr);
	void *vir2phy(void *addr);
	void flush_cache(void* startAddr, size_t size);
};

int  sunxi_alloc_open(void);
int  sunxi_alloc_close(void);
int  sunxi_alloc_alloc(int size);
int  sunxi_alloc_free(void * pbuf);
int  sunxi_alloc_vir2phy(void * pbuf);
int  sunxi_alloc_phy2vir(void * pbuf);
void sunxi_flush_cache(void* startAddr, int size);

#endif
