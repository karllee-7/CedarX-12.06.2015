#include <stdio.h>
#include <vector>
#include "ion.h"
#include "ion_sunxi.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include "ion_sunxi_alloc.h"

using std::vector;

sunxi_alloc_cls::sunxi_alloc_cls(){
	fd = open("/dev/ion", O_RDONLY);
	assert(fd != 0);
	printf("open /dev/ion success.\n");
}
sunxi_alloc_cls::~sunxi_alloc_cls(){
	assert(fd != 0);
	close(fd);
	printf("close /dev/ion success.\n");
}

void sunxi_alloc_cls::client_open(){
}
void sunxi_alloc_cls::client_close(){
}
void *sunxi_alloc_cls::alloc(size_t size){
	int ret;
	struct ion_allocation_data allocation_data = {
		.len = (size_t)size,
		.align = 4096,
		.heap_id_mask = ION_HEAP_TYPE_CUSTOM,
		.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC
	};
	ret = ioctl(fd, ION_IOC_ALLOC, &allocation_data);
	if(ret){
		printf("ion ION_IOC_ALLOC error.\n");
		return NULL;
	}
	struct ion_handle_data handle_data = {
		.handle = allocation_data.handle
	};
	/*===========================================*/
	sunxi_phys_data phys_data = {
		.handle = allocation_data.handle
	};
	struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_PHYS_ADDR,
		.arg = (unsigned long)&phys_data
	};
	ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
	if(ret){
		printf("ion ION_IOC_SUNXI_PHYS_ADDR error.\n");
		ioctl(fd, ION_IOC_FREE, &handle_data);
		return NULL;
	}
	/*===========================================*/
	struct ion_fd_data fd_data = {
		.handle = allocation_data.handle
	};
	ret = ioctl(fd, ION_IOC_MAP, &fd_data);
	if(ret){
		printf("ion ION_IOC_MAP error.\n");
		ioctl(fd, ION_IOC_FREE, &handle_data);
		return NULL;
	}
	void *addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_data.fd, 0);
	if(addr == MAP_FAILED){
		printf("ion mmap error.\n");
		ioctl(fd, ION_IOC_FREE, &handle_data);
		return NULL;
	}
	/*===========================================*/
	sunxi_mem_elm mem_elm(handle_data.handle, \
			fd_data.fd, \
			addr, \
			(void*)phys_data.phys_addr, \
			size);
	allocations.push_back(mem_elm);
	return allocations.back().get_virt_addr();
}
void sunxi_alloc_cls::free(void *addr){
	vector<sunxi_mem_elm>::iterator iter;
	int ret;
	for(iter = allocations.begin(); iter != allocations.end(); iter++){
		if(iter->get_virt_addr() == addr)
			break;
	}
	if(iter == allocations.end()){
		printf("can't find the ion buffer addr.");
		return ;
	}
	ret = munmap(iter->get_virt_addr(), iter->get_len());
	if(ret){
		printf("error munmap error.\n");
		return ;
	}
	struct ion_handle_data handle_data = {
		.handle = iter->get_handle()
	};
	ret = ioctl(fd, ION_IOC_FREE, &handle_data);
	if(ret){
		printf("free ion handle error.\n");
		return ;
	}
	allocations.erase(iter);
}
void *sunxi_alloc_cls::vir2phy(void *addr){
	vector<sunxi_mem_elm>::iterator iter;
	int ret;
	for(iter = allocations.begin(); iter != allocations.end(); iter++){
		if(iter->get_virt_addr() == addr)
			break;
	}
	if(iter == allocations.end()){
		printf("can't find the ion buffer addr.");
		return 0;
	}
	return iter->get_phys_addr();
}
void sunxi_alloc_cls::flush_cache(void* startAddr, size_t size){
	vector<sunxi_mem_elm>::iterator iter;
	int ret;
	for(iter = allocations.begin(); iter != allocations.end(); iter++){
		if(iter->get_virt_addr() == startAddr)
			break;
	}
	if(iter == allocations.end()){
		printf("can't find the ion buffer addr.");
		return;
	}
	sunxi_cache_range cache_range = {
		.start = (long)(iter->get_virt_addr()),
		.end = (long)(iter->get_virt_addr()) + (long)size
	};
	struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_FLUSH_RANGE,
		.arg = (unsigned long)&cache_range
	};
	ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
	if(ret)
		printf("ion flush error.\n");
}

sunxi_alloc_cls sunxi_allocation;

int  sunxi_alloc_open(void){
	sunxi_allocation.client_open();
	return 0;
}
int  sunxi_alloc_close(void){
	sunxi_allocation.client_close();
	return 0;
}
int  sunxi_alloc_alloc(int size){
	return (int)sunxi_allocation.alloc((size_t)size);
}
int sunxi_alloc_free(void * pbuf){
	sunxi_allocation.free(pbuf);
	return 0;
}
int  sunxi_alloc_vir2phy(void * pbuf){
	return (int)sunxi_allocation.vir2phy(pbuf);
}
int  sunxi_alloc_phy2vir(void * pbuf){
	assert(0);
	return 0;
}
void sunxi_flush_cache(void* startAddr, int size){
	sunxi_allocation.flush_cache(startAddr, (size_t)size);
}

