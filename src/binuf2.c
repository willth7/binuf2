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

uint64_t str_int_dec(int8_t* a) {
	uint64_t b = 0;
	for(uint8_t i = 0; i < 20; i++) {
		if (a[i] == 0 || a[i] == ')') {
			return b;
		}
		b *= 10;
		if (a[i] == '1') {
			b += 1;
		}
		else if (a[i] == '2') {
			b += 2;
		}
		else if (a[i] == '3') {
			b += 3;
		}
		else if (a[i] == '4') {
			b += 4;
		}
		else if (a[i] == '5') {
			b += 5;
		}
		else if (a[i] == '6') {
			b += 6;
		}
		else if (a[i] == '7') {
			b += 7;
		}
		else if (a[i] == '8') {
			b += 8;
		}
		else if (a[i] == '9') {
			b += 9;
		}
		else if (a[i] != '0' && a[i] != ')') {
			return -1;
		}
	}
}

uint8_t* bin_read(int8_t* path, uint32_t* bn) {
	int32_t fd = open(path, O_RDONLY);
	if (fd == -1) {
		printf("error: failed to open file '%s'\n", path);
		*bn = 0;
		return 0;
	}
	
	struct stat fs;
	fstat(fd, &fs);
	uint8_t* bin = mmap(0, fs.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close (fd);
	*bn = fs.st_size;
	return bin;
}

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

void uf2_fsz(struct uf2_s* u, uint32_t un) {
	for (uint32_t i = 0; i < un; i++) {
		u[i].fmid = un * 512;
	}
}

void uf2_writ(struct uf2_s* u, uint32_t un, int8_t* path) {
	int32_t fd = open(path, O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        printf("error: failed to create file '%s'\n", path);
        return;
    }
	
    ftruncate(fd, 512 * un);
    uint8_t* mem = mmap(0, 512 * un, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(mem, u, 512 * un);
	
	munmap(mem, 512 * un);
	close(fd);
}

void uf2_help() {
	printf("usage: binuf2 [options] out.uf2\n");
	printf("\ngeneric options\n\n");
	printf("  -f                    file size flag\n");
	printf("  --fam-id id           family id flag\n");
	printf("  -i source.bin         source binary file\n");
	printf("  -a address            address to load binary\n");
	printf("  -o offset             offset of address\n");
	printf("  --help                manual\n");
	printf("\nRaspberry Pi Pico specific options\n\n");
	printf("  --rpi-pico            flags Raspberry Pi Pico family id\n");
	printf("  --sram source.bin     loads binary into the sram of the Raspberry Pi Pico\n");
	printf("  --flash source.bin    loads binary into the flash of the Raspberry Pi Pico\n");
}

int8_t main(int32_t argc, int8_t** argv) {
	if (argc == 1) {
		uf2_help();
	}
	
	uint8_t fsz_flag = 0;
	uint8_t fmid_flag = 0;
	uint32_t fmid;
	
	uint32_t path[256] = {};
	uint32_t addr[256] = {};
	uint32_t off[256] = {};
	uint32_t in = 0;
	
	for (uint32_t i = 1; i < argc - 1; i++) {
		if (!strcmp(argv[i], "-f")) {
			fsz_flag = 1;
		}
		else if (!strcmp(argv[i], "--fam-id")) {
			
		}
		else if (!strcmp(argv[i], "-i")) {
			if (i + 1 == argc) {
				printf("error: expected file\n");
				return -1;
			}
			path[in] = i + 1;
			in++;
			i++;
		}
		else if (!strcmp(argv[i], "-a")) {
			if (i + 1 == argc) {
				printf("error: expected address\n");
				return -1;
			}
			addr[in - 1] = str_int_dec(argv[i + 1]);
			i++;
		}
		else if (!strcmp(argv[i], "-o")) {
			if (i + 1 == argc) {
				printf("error: expected offset\n");
				return -1;
			}
			off[in - 1] = str_int_dec(argv[i + 1]);
			i++;
		}
		else if (!strcmp(argv[i], "--help")) {
			uf2_help();
		}
		else if (!strcmp(argv[i], "--rpi-pico")) {
			fmid_flag = 1;
			fmid = 3834380118;
		}
		else if (!strcmp(argv[i], "--sram")) {
			if (i + 1 == argc) {
				printf("error: expected file\n");
				return -1;
			}
			path[in] = i + 1;
			addr[in] = 536870912;
			in++;
			i++;
		}
		else if (!strcmp(argv[i], "--flash")) {
			if (i + 1 == argc) {
				printf("error: expected file\n");
				return -1;
			}
			path[in] = i + 1;
			addr[in] = 268435456;
			in++;
			i++;
		}
		else {
			printf("error: unknown option '%s'\n", argv[i]);
			return -1;
		}
	}
	
	struct uf2_s* u = calloc(512, 65536);
	uint32_t un = 0;
	
	for (uint32_t i = 0; i < in; i++) {
		uint32_t bn = 0;
		uint8_t* bin = bin_read(argv[path[i]], &bn);
		un += uf2_bin(bin, bn, addr[i] + off[i], u);
		munmap(bin, bn);
	}
	
	if (fmid_flag) {
		uf2_fmid(u, un, fmid);
	}
	else if (fsz_flag) {
		uf2_fsz(u, un);
	}
	
	uf2_writ(u, un, argv[argc - 1]);
	
	free(u);
	return 0;
}
