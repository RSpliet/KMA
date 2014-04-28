/**
 * cl.h
 * Wrapper functions for OpenCL operations
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

#ifndef USER_CL_H
#define USER_CL_H

#include <CL/opencl.h>

typedef struct {
	size_t x, y, z;
} size_t3;

struct opts {
	cl_device_type devtype;
	unsigned int print_buffer, debug;
	unsigned int wi_entries;
	size_t3* wi;
};

extern struct opts options;

/**
 * kernel_read - Read a kernel from file
 * @kernel: File name of the kernel, relative to executable
 * @return: Character buffer with kernel file contents
 */
extern char *kernel_read(const char*);

/**
 * platform_bits() - Return the number of bits (32/64) of given device
 * @dev: Device ID
 * @return: the number of bits of the platform, 0 on failure
 */
extern unsigned int platform_bits(cl_device_id);

/**
 * options_read - Read and process options
 * @argc: Number of arguments
 * @argv: Pointer to character arrays
 * @opt_process: Function pointer that processes app specific options
 *
 * opt_process should take as arguments (index, argc, argv), returns -1 on
 * failure. On success, it should return the number of processed arguments - 1.
 */
extern int options_read(unsigned int, char **,
		int (*opt_process)(unsigned int, unsigned int, char **));

/**
 * options_print - Print generic options
 */
extern void options_print();

/**
 * clSetup - Setup cl basic environment
 */
extern int clSetup(cl_platform_id *, cl_device_id *, cl_context *,
		cl_command_queue *);

extern cl_program program_compile(cl_platform_id, cl_context, cl_device_id *, unsigned int,
		char**);

#endif
