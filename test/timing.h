/**
 * timing.h
 * Library for obtaining wall-clock time and calculating averages
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
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// tSamples in
#define tSamples_baseten 0
#define tSamples 1

void
tStart();

void
tEnd(int pI);

void
tPrint();
