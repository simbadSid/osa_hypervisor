/*
Copyright (C) Pr. Olivier Gruber.

This code is part of a suite of educational software,
it is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Classpath; see the file COPYING.  If not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA.
*/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "asm.h"
#include "parser.h"
#include <time.h>
#include <fcntl.h>
#include "bus.h"
#include "core.h"





uint8_t			*memory;	// physical memory array, allocate globally
struct core		*core;		// registers and all
struct bus		*bus;		// bus and its devices.





int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "A program name should be given on the command line\n");
		return EXIT_FAILURE;
	}

	fcntl(0, F_SETFL, O_NONBLOCK);

	FILE *program = fopen(argv[1], "r");

	if (program == NULL)
	{
		fprintf(stderr, "Cannot open file %s \n", argv[1]);
		return EXIT_FAILURE;
	}

	printf("Loading =file %s... \n", argv[1]);
	printf("  sizeof(struct instr)=%d \n", (int) sizeof(struct instr));

	memory = (uint8_t*) malloc(sizeof(char *) * SIZE_MEM_ARRAY);
	core = (struct core*)malloc(sizeof(struct core));
	memset(core, 0, sizeof(struct core));

	bus = (struct bus*)malloc(sizeof(struct bus));
	memset(bus, 0, sizeof(struct bus));
	bus_init();

	core->regs[16] = 0x0000;									// PC = 0

	/*
	 * Boot in physical mode with interrupts disabled,
	 * with no MMU translation.
	 */
	core->regs[18] = 0x1;
	core->regs[18] |= 0x04;										// Disable interrupt

	parse(program,memory,BASE);									// parser will close the file once it is done with it

	printf("Parsing done.\n");
	printf("Starting emulation...\n");
	decoding_loop();
	printf("Emulation halted.\n");

	// should put that in an atexit function maybe?
	free(core);
	free(memory);
	return EXIT_SUCCESS;
}


