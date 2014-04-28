/**
 * cl.c
 * OpenCL wrapper functions
 * Copyright (C) 2013-2014 Roy Spliet, Delft University of Technology
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "test/cl.h"

struct opts options;

static size_t3 wi_single[1] = {{1,1,1}};

size_t
kernel_size(const char *filename)
{
	struct stat fs;

	stat(filename, &fs);
	return fs.st_size;
}

char *
kernel_read(const char *filename)
{
	size_t size;
	char *file;
	FILE *fp;

	fp = fopen(filename, "r");
	if(!fp) {
		printf("Error: Could not open CL source file %s", filename);
		return NULL;
	}

	size = kernel_size(filename);
	file = calloc(1, size+1);
	if(!file) {
		printf("Error: Could not allocate space for %s", filename);
		return NULL;
	}

	fread(file, size, 1, fp);
	fclose(fp);

	return file;
}

unsigned int
platform_bits(cl_device_id dev)
{
	unsigned int bits;
	cl_int error;

	error = clGetDeviceInfo(dev, CL_DEVICE_ADDRESS_BITS, sizeof(cl_uint),
				&bits, NULL);
	if(error != CL_SUCCESS) {
		printf("Error: could not discover device address space\n");
		return 0;
	}

	return bits;
}

cl_program
program_compile(cl_platform_id pid, cl_context ctx, cl_device_id *cid, unsigned int srcf,
		char** src)
{
	cl_program prg;
	cl_int err;
	char *status;
	int bStatus = 0;
	size_t ret_val_size;
	unsigned int i, cf;
	char *cbuf, *cbufptr, *cflags[4];
	size_t len;
	size_t binsize;
	char *bin;

	/* Put together compiler options */
	/* GNU GCC only */
	cbuf = getcwd(NULL, 0);
	len = strlen(cbuf);
	cflags[2] = malloc(len+4);
	sprintf(cflags[2], "-I %s", cbuf);
	free(cbuf);

	clGetPlatformInfo(pid, CL_PLATFORM_VENDOR, 0, NULL, &ret_val_size);
	status = malloc(ret_val_size+1);
	clGetPlatformInfo(pid, CL_PLATFORM_VENDOR, ret_val_size, status, NULL);

	if(strncmp(status, "Advanced Micro Devices", 22) == 0) {
		cflags[1] = "-D CL_PLATFORM=1";
	} else if(strncmp(status, "NVIDIA", 6) == 0) {
		cflags[1] = "-D CL_PLATFORM=2 -cl-nv-opt-level=0";
	} else if(strncmp(status, "Intel", 5) == 0) {
		cflags[1] = "-D CL_PLATFORM=3";
	} else {
		cflags[1] = "-D CL_PLATFORM=0";
	}

	if(platform_bits(*cid) == 64)
		cflags[0] = "-D CL_BITNESS=64";
	else
		cflags[0] = "-D CL_BITNESS=32";

	cflags[3] = "-g";

	if(options.debug == 1) {
		cf = 4;
	} else {
		cf = 3;
	}
	len = 0;

	for(i = 0; i < cf; i++) {
		len += strlen(cflags[i]) + 1;
	}
	cbuf = cbufptr = calloc(1, len + 1);
	for(i = 0; i < cf; i++) {
		cbufptr += sprintf(cbufptr, "%s ", cflags[i]);
	}

	/* And start compiling */
	prg = clCreateProgramWithSource(ctx, srcf, (const char **)src, NULL, &err);
	if(err != CL_SUCCESS) {
		printf("Build error: Could not create program: %i\n", err);
		return (cl_program)-1;
	}

	err = clBuildProgram(prg, 1, cid, cbuf, NULL, NULL);
	if(err != CL_SUCCESS) {
		printf("Build error: Could not build program: %i\n", err);

		err = clGetProgramBuildInfo(prg, *cid, CL_PROGRAM_BUILD_STATUS,
				sizeof(cl_build_status), &bStatus, NULL);
		if(err != CL_SUCCESS) {
			printf("Build error: Could not read back build status: %i\n", err);
			return (cl_program)-1;
		}

		if(bStatus != CL_BUILD_SUCCESS) {
			printf("Build error: %i\n\n",bStatus);
			printf("Compiler output:\n");
			clGetProgramBuildInfo(prg, *cid, CL_PROGRAM_BUILD_LOG, 0, NULL,
					&ret_val_size);
			status = malloc(ret_val_size+1);
			clGetProgramBuildInfo(prg, *cid, CL_PROGRAM_BUILD_LOG, ret_val_size,
					status, NULL);

			printf("%s",status);
			return (cl_program)-1;
		}
	}
	/*
	printf("8<--- Source ---\n");

	clGetProgramInfo(prg, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binsize, NULL);
	printf("Binary size: %d\n", binsize);
	bin = malloc(binsize);
	clGetProgramInfo(prg, CL_PROGRAM_BINARIES, binsize, &bin, NULL);
	printf("%s\n", bin);

	printf("8<--- End source ---\n\n");
	*/
	return prg;
}

int
threadcfg_read(char *file)
{
	FILE *fp;
	unsigned int entries, i;
	int ret;
	size_t3 *entry;

	fp = fopen(file, "r");
	if(!fp) {
		printf("Error: file %s not found\n\n", file);
		return -1;
	}

	ret = fscanf(fp, "%d\n", &entries);
	if(ret < 1 || entries <= 0) {
		printf("Error: malformed thread config file provided\n\n");
		return -1;
	}

	entry = malloc(sizeof(size_t3)*entries);
	for(i = 0; i < entries; i++) {
		ret = fscanf(fp, "{%zu,%zu,%zu}\n", &entry[i].x, &entry[i].y,
				&entry[i].z);
		if(ret != 3) {
			printf("Error: malformed thread config file provided\n\n");
			return -1;
		}
	}

	options.wi_entries = entries;
	options.wi = entry;

	fclose(fp);

	return 0;
}

int
options_read(unsigned int argc, char **argv,
		int (*opt_process)(unsigned int, unsigned int, char **))
{
	unsigned int i;
	int ret;
	size_t3 tSingle;

	options.devtype = CL_DEVICE_TYPE_GPU;
	options.print_buffer = 0;
	options.debug = 0;
	options.wi_entries = 1;
	options.wi = wi_single;

	for(i = 1; i < argc; i++) {
		if(strncmp(argv[i], "-c", 2) == 0) {
			options.devtype = CL_DEVICE_TYPE_CPU;
		} else if(strncmp(argv[i], "-p", 2) == 0) {
			options.print_buffer = 1;
		} else if(strncmp(argv[i], "-g", 2) == 0) {
			options.debug = 1;
		} else if(strncmp(argv[i], "-t", 2) == 0) {
			i++;
			if(i < argc) {
				ret = sscanf(argv[i], "%zu,%zu,%zu", &tSingle.x,
						&tSingle.y, &tSingle.z);
				if(ret != 3)
					return -1;

				wi_single->x = tSingle.x;
				wi_single->y = tSingle.y;
				wi_single->z = tSingle.z;

				options.wi_entries = 1;
				options.wi = &wi_single[0];
			} else {
				printf("Error: -t requires a {x,y,z} tuple\n\n");
				return -1;
			}
		} else if(strncmp(argv[i], "-T", 2) == 0) {
			i++;
			if(i < argc) {
				ret = threadcfg_read(argv[i]);
				if(ret < 0)
					return -1;
			} else {
				printf("Error: -T requires a file name\n\n");
				return -1;
			}
		} else {
			ret = -1;
			if(opt_process)
				ret = opt_process(i, argc, argv);

			if(ret < 0) {
				printf("Error: Unknown option: %s\n\n", argv[i]);
				return -1;
			} else {
				i += ret;
			}
		}
	}

	return 0;
}

void
options_print()
{
	printf("\t-c:\t\tExecute on CPU\n");
	printf("\t-p:\t\tPrint buffer(s)\n");
	printf("\t-g:\t\tCompile kernel with debug symbols (if available)\n");
	printf("\t-t x,y,z:\tProvide a single thread-configuration\n\n");
	printf("\t-T [file]:\tProvide a file with thread-configurations\n");
	printf("Thread configuration files are {x, y, z} tuples, always in 3"
			" dimensions.\n");
	printf("The first line contains the amount of tuples. If no argument"
			" is given\n");
	printf("the program starts with a single work-item.\n");
}

int
clSetup(cl_platform_id *pid, cl_device_id *cid, cl_context *ctx,
		cl_command_queue *cq)
{
	cl_int err;
	cl_uint platforms, devs;
	cl_platform_id *plats;
	unsigned int i;
	cl_device_id *cids;

	*cid = 0;

	if (clGetPlatformIDs(0, NULL, &platforms) != CL_SUCCESS) {
		printf("Error: Could not get platforms\n");
		return -1;
	}
	plats = malloc(platforms*sizeof(cl_platform_id));
	clGetPlatformIDs(platforms, plats, NULL);

	for (i = 0; i < platforms; i++) {
		clGetDeviceIDs(plats[i], options.devtype, 0, NULL, &devs);
		cids = malloc(devs * sizeof(cl_device_id));
		if (clGetDeviceIDs(plats[i], options.devtype, devs, cids, NULL) == CL_SUCCESS) {
			*pid = plats[i];
			*cid = cids[devs - 1];
			printf("%d\n", devs);
			free(cids);
			break;
		}
		free(cids);
	}

	free(plats);

	if (*cid == 0) {
		printf("Error: Could not get device\n");
		return -1;
	}

	cl_context_properties ctxprops[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)*pid,
		0
	};

	*ctx = clCreateContext(ctxprops, 1, cid, NULL, NULL, &err);
	if (err != CL_SUCCESS) {
		printf("Error: Could not create context\n");
		return -1;
	}

	*cq = clCreateCommandQueue(*ctx, *cid, 0, &err);
	if (!*cq) {
		printf("Error: Could not create command queue\n");
		return -1;
	}

	return 0;
}
