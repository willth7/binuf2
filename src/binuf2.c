//   Copyright 2022 Will Thomas
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0;
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

uint32_t mag0 = 171066965;
uint32_t mag1 = 2656915799;
uint32_t magz = 179400496;

uint32_t pico_fmid = 3834380118;

struct uf2_s {
	uint32_t mag0;
	uint32_t mag1;
	uint32_t flag;
	uint32_t addr;
	uint32_t bytn;
	uint32_t blki;
	uint32_t blkn;
	uint32_t fmid;
	uint8_t byte[476];
	uint32_t magz;
};

uint32_t uf2_bin(uint8_t* bin, uint32_t bn, uint32_t addr, struct uf2_s* u) {
	for (uint32_t i = 0; i * 256 < bn; i++) {
		u[i].mag0 = mag0;
		u[i].mag1 = mag1;
		u[i].magz = magz;
		u[i].addr = addr + (i * 256);
		u[i].blki = i;
		if (bn - (i * 256) > 256) {
			u[i].bytn = 256;
			memcpy(u[i].byte, bin + (i * 256), 256);
		}
		else {
			u[i].bytn = bn - (i * 256);
			memcpy(u[i].byte, bin + (i * 256), bn - (i * 256));
			for (uint32_t j = 0; j <= i; j++) {
				u[j].blkn = i + 1;
			}
			return i + 1;
		}
	}
}

void uf2_fmid(struct uf2_s* u, uint32_t un, uint32_t fmid) {
	for (uint32_t i = 0; i < un; i++) {
		u[i].flag |= 8192;
		u[i].fmid = fmid;
	}
}

int8_t uf2_writ(struct uf2_s* u, uint32_t un, int8_t* path) {
	int32_t fd = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if (fd == -1) {
        printf("error: failed to create file '%s'\n", path);
        return -1;
    }
	
    ftruncate(fd, 512 * un);
    uint8_t* mem = mmap(0, 512 * un, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(mem, u, 512 * un);
	
	munmap(mem, 512 * un);
	close(fd);
	return 0;
}

int8_t main(int32_t argc, int8_t** argv) {
	if (argc != 3) {
		printf("usage: execbin [binary.bin] [size of stack(bytes)]\n");
		return -1;
	}

	int32_t fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		printf("failed to open file '%s'\n", argv[1]);
		return -1;
	}
	
	struct stat fs;
	fstat(fd, &fs);
	uint8_t* bin = mmap(0, fs.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close (fd);
	
	struct uf2_s u = {};
	uint32_t un = uf2_bin(bin, fs.st_size, 0x10000000, &u);
	uf2_fmid(&u, un, pico_fmid);
	return uf2_writ(&u, un, argv[2]);
}
