#ifndef __PARSER_H__
#define __PARSER_H__

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

#include "asm.h"

/**
  * Basically, an assembler that translates the given program
  * into binary and loads it into the memory
  **/

void parse(FILE *program, uint8_t* memory, uint32_t entry_point);

#endif