/*
 * nativeTest.cpp
 *
 *  Created on: Jul 6, 2017
 *      Author: lichao
 */
#include "videobuffer.h"
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

int main(int c, char *s[]) {
	OmxGraphics_t hwd;
	void *handle = dlopen((const char *)s[1], RTLD_NOW);
	printf("handle %p\n", handle);
	int ret = InitVideoBuffer(&hwd, handle);
	printf("alloc %p \n", hwd.alloc);
	printf("lock %p \n", hwd.lock);
	printf("unlock %p \n", hwd.unlock);
	printf("destory %p \n", hwd.destory);
	while(1) usleep(100000);
	dlclose(handle);
	return 0;
}



