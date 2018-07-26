#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

typedef struct {
	ion_user_handle_t handle;
	int fd;
	void *virt_addr;
	void *phys_addr;
	size_t len;
} mem_elm_t;

typedef struct {
	int fd;
	int mem_cnt;
	int mem_max;
	mem_elm_t *mem;
} allocation_t;

allocation_t allocations = {
	.fd = 0,
	.mem_cnt = 0,
	.mem_max = MAX_ION_BLOCK,
	.mem = NULL
};

int mem_erase(unsigned index){
	unsigned i;
	if(index < allocations.mem_cnt){
		for(i=index; i<(allocations.mem_cnt-1); i++){
			allocations.mem[i].handle    = allocations.mem[i+1].handle;
			allocations.mem[i].fd        = allocations.mem[i+1].fd;
			allocations.mem[i].virt_addr = allocations.mem[i+1].virt_addr;
			allocations.mem[i].phys_addr = allocations.mem[i+1].phys_addr;
			allocations.mem[i].len       = allocations.mem[i+1].len;
		}
		allocations.mem_cnt -= 1;
		return 0;
	}else{
		printf("ion error: the index to erase is not exist.\n");
		return -1;
	}
}
inline int mem_is_full(void){
	return (allocations.mem_cnt >= allocations.mem_max);
}
inline int mem_is_empty(void){
	return (allocations.mem_cnt == 0);
}
inline mem_elm_t *mem_next(void){
	assert(allocations.mem_cnt < allocations.mem_max);
	return &allocations.mem[allocations.mem_cnt];
}
inline mem_elm_t *mem_last(void){
	assert(allocations.mem_cnt > 0);
	return &allocations.mem[allocations.mem_cnt-1];
}
inline void mem_inc(void){
	assert(allocations.mem_cnt < allocations.mem_max);
	allocations.mem_cnt++;
}
inline void mem_dec(void){
	assert(allocations.mem_cnt > 1);
	allocations.mem_cnt--;
}
inline int mem_find_virt(void *addr){
	int index;
	for(index=0; index<allocations.mem_cnt; index++){
		if(allocations.mem[index].virt_addr == addr)
			break;
	}
	if(index == allocations.mem_max)
		return -1;
	return index;
}
int sunxi_alloc_open(void){
	assert(allocations.fd == 0);
	allocations.fd = open("/dev/ion", O_RDONLY);
	assert(allocations.fd != 0);
	allocations.mem = (mem_elm_t*)malloc(sizeof(mem_elm_t)*MAX_ION_BLOCK);
	assert(allocations.mem != NULL);
	memset((void*)allocations.mem, 0, sizeof(mem_elm_t)*MAX_ION_BLOCK);
	printf("open /dev/ion success.\n");
	return 0;
}
int sunxi_alloc_close(void){
	int ret, index;
	struct ion_handle_data handle_data;
	while(!mem_is_empty()){
		ret = munmap(mem_last()->virt_addr, mem_last()->len);
		if(ret){
			printf("error munmap error.\n");
			return -1;
		}
		handle_data.handle = mem_last()->handle;
		ret = ioctl(allocations.fd, ION_IOC_FREE, &handle_data);
		if(ret){
			printf("free ion handle error.\n");
			return -1;
		}
		mem_dec();
	}
	close(allocations.fd);
	allocations.fd = 0;
	printf("close /dev/ion success.\n");
	return 0;
}
int sunxi_alloc_alloc(int size){
	int ret;
	struct ion_allocation_data allocation_data = {
		.len = (size_t)size,
		.align = 4096,
		.heap_id_mask = ION_HEAP_TYPE_CUSTOM,
		.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC
	};
	struct ion_handle_data handle_data;
	sunxi_phys_data phys_data;
	struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_PHYS_ADDR
	};
	struct ion_fd_data fd_data;
	/*===========================================*/
	if(mem_is_full()){
		printf("ion buffer full.\n");
		return 0;
	}
	/*===========================================*/
	ret = ioctl(allocations.fd, ION_IOC_ALLOC, &allocation_data);
	if(ret){
		printf("ion ION_IOC_ALLOC error.\n");
		return -1;
	}
	handle_data.handle = allocation_data.handle;
	/*===========================================*/
	phys_data.handle = allocation_data.handle;
	custom_data.arg = (unsigned long)&phys_data;
	ret = ioctl(allocations.fd, ION_IOC_CUSTOM, &custom_data);
	if(ret){
		printf("ion ION_IOC_SUNXI_PHYS_ADDR error.\n");
		ioctl(allocations.fd, ION_IOC_FREE, &handle_data);
		return -1;
	}
	/*===========================================*/
	fd_data.handle = allocation_data.handle;
	ret = ioctl(allocations.fd, ION_IOC_MAP, &fd_data);
	if(ret){
		printf("ion ION_IOC_MAP error.\n");
		ioctl(allocations.fd, ION_IOC_FREE, &handle_data);
		return -1;
	}
	/*===========================================*/
	void *addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_data.fd, 0);
	if(addr == MAP_FAILED){
		printf("ion mmap error.\n");
		ioctl(allocations.fd, ION_IOC_FREE, &handle_data);
		return -1;
	}
	/*===========================================*/
	mem_next()->handle = handle_data.handle;
	mem_next()->fd = fd_data.fd;
	mem_next()->virt_addr = addr;
	mem_next()->phys_addr = (void*)phys_data.phys_addr;
	mem_next()->len = size;
	mem_inc();
	return (int)addr;
}
int sunxi_alloc_free(void * pbuf){
	int ret, index;
	struct ion_handle_data handle_data;
	index = mem_find_virt(pbuf);
	if(index == -1){
		printf("can't find the ion buffer addr.");
		return -1;
	}
	ret = munmap(allocations.mem[index].virt_addr, allocations.mem[index].len);
	if(ret){
		printf("error munmap error.\n");
		return -1;
	}
	handle_data.handle = allocations.mem[index].handle;
	ret = ioctl(allocations.fd, ION_IOC_FREE, &handle_data);
	if(ret){
		printf("free ion handle error.\n");
		return -1;
	}
	mem_erase(index);
	return 0;
}
int sunxi_alloc_vir2phy(void * pbuf){
	int ret, index;
	struct ion_handle_data handle_data;
	index = mem_find_virt(pbuf);
	if(index == -1){
		printf("can't find the ion buffer addr.");
		return -1;
	}
	return (int)(allocations.mem[index].phys_addr);
}
void sunxi_flush_cache(void* startAddr, int size){
	int ret;
	sunxi_cache_range cache_range = {
		.start = (long)(startAddr),
		.end = (long)(startAddr) + (long)size
	};
	struct ion_custom_data custom_data = {
		.cmd = ION_IOC_SUNXI_FLUSH_RANGE,
		.arg = (unsigned long)&cache_range
	};
	ret = ioctl(allocations.fd, ION_IOC_CUSTOM, &custom_data);
	if(ret)
		printf("ion flush error.\n");
}
int  sunxi_alloc_phy2vir(void * pbuf){
	assert(0);
	return 0;
}

