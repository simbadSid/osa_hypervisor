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

#ifndef ASM_H_
#define ASM_H_


enum
{
	MOV=1,
	LOAD,
	FETCH,
	STORE,
	ADD,
	SUB,
	MUL,
	DIV,
	BRL,
	BEQ,
	BNE,
	BLT,
	BGT,
	AND,
	OR,
	XOR,
	HALT,
	IRET,
	PUSH,
	POP
};

typedef uint8_t reg_t;

/**
 * This is for the encoding of assembly instructions.
 * All instructions are encoding in 32bits, like most RISC processor
 */
typedef struct __attribute ((packed)) instr
{
	unsigned int op :5;
	union __attribute ((packed))
	{
		struct __attribute ((packed))
		{
			unsigned int src :5;
			unsigned int dst :5;
		} mov;
		struct __attribute ((packed))
		{
			unsigned int dst :5;
			unsigned int value :16;
		} load;
		struct __attribute ((packed))
		{
			unsigned int src :5;
			unsigned int dst :5;
			int offset :5;
		} fetch;
		struct __attribute ((packed))
		{
			unsigned int src :5;
			unsigned int dst :5;
			int offset :5;
		} store;
		struct __attribute ((packed))
		{
			unsigned int lhr :5;
			unsigned int imm  :1;
			union __attribute ((packed))
			{
				unsigned int rhr :5;
				unsigned int rhv :16;
			};
		} arith;
// ------------------------------------------------------------
		struct __attribute ((packed)) {
		unsigned int lhr :5;
		unsigned int imm  :1;
		union __attribute ((packed)) {
		unsigned int rhr :5;
		unsigned int rhv :16;
		};
		} logic;
		struct __attribute ((packed)) {
		unsigned int dst :5;
		unsigned int lhr :5;
		unsigned int imm  :1;
		union __attribute ((packed)) {
		unsigned int rhr :5;
		unsigned int rhv :8;
		};
		} bcond;
		struct __attribute ((packed)) {
		unsigned int reg :5;
		} brl;
		struct __attribute ((packed)) {
		unsigned int reg :5;
		} stack;
  };
} instr_t;


#endif /* ASM_H_ */
