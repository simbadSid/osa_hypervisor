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

This is a simple and ugly parser to assembly a source text of assembly
instructions. It could definitely be improved using a real lexer and parser,
but it was supposed to be really simple, grew more complex, and there you
have it... an ugly useful piece of code that I do not want to redo.

YOU DO NOT HAVE TO REALLY UNDERSTAND THE DETAILS HERE.
THIS IS JUST ABOUT GENERATING ASSEMBLY INSTRUCTIONS.

HOWEVER YOU DO NEED TO UNDERSTAND THE FORMAT OF THE ASSEMBLY INSTRUCTIONS,
SO THAT YOU CAN UNDERSTAND THE FETCH-DECODE-ISSUE LOOP OF THE CORE.

*/
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "asm.h"
#include "parser.h"

instr_t translate_line(char *line, uint32_t offset);
void store_instr(instr_t instruction, uint32_t addr);

uint32_t char_to_int(uint8_t x) {
  return (uint32_t) (x - '0');
}

uint32_t skip_spaces(char *line, int offset) {
  while (line[offset] == ' ' || line[offset] == '\t')
    offset++;
  return offset;
}

uint32_t skip_comma(char *line, int offset) {
  while (line[offset] != ',')
    offset++;
  return skip_spaces(line,offset+1);
}

uint32_t skip_number(char *line, int offset) {
  while (line[offset] != '+' && line[offset] != '-' && line[offset] != ']' )
    offset++;
  return offset;
}

int lineno=0;

void syntax_error(char* line) {
  printf("Syntax error: [%d] %s \n", lineno,line);
  exit(-1);
}

#define ALIGN32_GRAIN (sizeof(uint32_t)-1)
#define ALIGN32(v) ((((uintptr_t)(v))+ALIGN32_GRAIN)& ~ALIGN32_GRAIN)




void parse(FILE *program, uint8_t* memory, uint32_t entry_point)
{
	char *asmline = (char *) malloc(sizeof(char) * 16);
	size_t size = 16;
	int readen;
	uint32_t pc = entry_point;
	instr_t inst;

	for (;;)
	{
		readen = getline(&asmline, &size, program);
		if (readen<=0)
			break;

		uint32_t offset;
		asmline[readen - 1] = '\0'; // remove the '\n'
		offset = skip_spaces(asmline, 0);
		if (offset>=readen-1)
			continue;
		if (asmline[offset]=='#' || asmline[offset]==';')
			continue;
		if (asmline[offset]=='.')
		{
			if (asmline[offset+1]!='=')
				syntax_error(asmline);
			pc = strtol(&asmline[offset + 2],NULL,0); // atoi((&asmline[offset + 2]));
			if (pc != ALIGN32(pc))
			{
				printf("Unaligned 0x%x address\n",pc);
				syntax_error(asmline);
			}
			continue;
		}
		inst = translate_line(asmline,offset);
		*(instr_t*)(memory + pc) = inst;
#ifdef CONFIG_DEBUG
		printf(" [0x%x] 0x%x \t%s \n",pc,*(uint32_t*)&inst,asmline);
#endif
		lineno++;
		pc += 4;
	}
	free(asmline);
	fclose(program); // might as well free ressources now

	return;
}


instr_t translate_mov(char *line, uint32_t offset) {
  instr_t inst;
  *(uint32_t *)&inst = 0;
  offset = skip_spaces(line, offset);
  reg_t dst;
  reg_t src;
  if (line[offset] == 'r') {
    dst = atoi((&line[offset + 1]));
    offset = skip_comma(line, offset);

    if (line[offset] == 'r') {
      inst.op = MOV;
      inst.mov.dst = dst;
      inst.mov.src = atoi((&line[offset + 1]));
      return inst;
    }

    if (line[offset] == '#') {
      inst.op = LOAD;
      inst.load.dst = dst;
      inst.load.value = strtol(&line[offset + 1],NULL,0); // atoi((&line[offset + 1]));
      return inst;
    }

    if (line[offset] == '[') {
      int off = 0;
      offset = skip_spaces(line, offset+1);
      if (line[offset] != 'r')
        syntax_error(line);
      src = atoi((&line[offset + 1]));
      offset = skip_number(line, offset);
      if (line[offset] == '+') {
        offset = skip_spaces(line, offset);
        off = atoi((&line[offset + 1]));
        offset = skip_number(line, offset+1);
      } else if (line[offset] == '-') {
        offset = skip_spaces(line, offset);
        off = -atoi((&line[offset + 1]));
        offset = skip_number(line, offset+1);
      }
      if (line[offset] != ']')
        syntax_error(line);

      inst.op = FETCH;
      inst.fetch.dst = dst;
      inst.fetch.src = src;
      inst.fetch.offset = off;
      return inst;
    }
    syntax_error(line);
  }
  if (line[offset] == '[') {
    int off=0;
    offset = skip_spaces(line, offset+1);
    if (line[offset] != 'r')
      syntax_error(line);
    dst = atoi((&line[offset + 1]));
    offset = skip_number(line, offset);
    if (line[offset] == '+') {
      offset = skip_spaces(line, offset);
      off = atoi((&line[offset + 1]));
      offset = skip_number(line, offset+1);
    } else if (line[offset] == '-') {
      offset = skip_spaces(line, offset);
      off = -atoi((&line[offset + 1]));
      offset = skip_number(line, offset+1);
    }
    if (line[offset] != ']')
      syntax_error(line);
    offset = skip_comma(line, offset);
    if (line[offset] != 'r')
      syntax_error(line);

    inst.op = STORE;
    inst.store.offset = off;
    inst.store.dst = dst;
    inst.store.src = atoi((&line[offset + 1]));
    return inst;
  }
  syntax_error(line);
  return inst; // keep compiler happy
}

int translate_branch(char *line, uint32_t offset, instr_t *ipt) {
  instr_t inst;
  *(uint32_t *)&inst = 0;
  if (strncmp(&line[offset], "brl ", 4) == 0) {
    inst.op = BRL;
    offset = skip_spaces(line, offset + 4);
    if (line[offset] != 'r')
      syntax_error(line);
    inst.brl.reg = atoi((&line[offset + 1]));
    *ipt = inst;
    return 1;
  }

  if (strncmp(&line[offset], "beq ", 4) == 0) {
    inst.op = BEQ;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "bne ", 4) == 0) {
    inst.op = BNE;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "blt ", 4) == 0) {
    inst.op = BLT;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "bgt ", 4) == 0) {
    inst.op = BGT;
    offset = skip_spaces(line, offset + 4);
  } else
    return 0;

  // grab destination register
  if (line[offset] != 'r')
    syntax_error(line);
  inst.bcond.dst = atoi((&line[offset + 1]));
  offset = skip_comma(line, offset);

  // grab first condition register
  if (line[offset] != 'r')
    syntax_error(line);
  inst.bcond.lhr = atoi((&line[offset + 1]));
  offset = skip_comma(line, offset);

  // grab second condition register
  if (line[offset] == 'r') {
    inst.bcond.imm = 0;
    inst.bcond.rhr = atoi((&line[offset + 1]));
  } else if (line[offset] == '#') {
    inst.bcond.imm = 1;
    inst.bcond.rhv = strtol(&line[offset + 1],NULL,0); // atoi((&line[offset + 1]));
  } else
    syntax_error(line);

  *ipt = inst;
  return 1;
}

int translate_arith(char *line, uint32_t offset, instr_t *ipt) {
  instr_t inst;
  *(uint32_t *)&inst = 0;

  if (strncmp(&line[offset], "add ", 4) == 0) {
    inst.op = ADD;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "sub ", 4) == 0) {
    inst.op = SUB;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "mul ", 4) == 0) {
    inst.op = MUL;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "div ", 4) == 0) {
    inst.op = DIV;
    offset = skip_spaces(line, offset + 4);
  } else
    return 0;

  if (line[offset] != 'r')
    syntax_error(line);
  inst.arith.lhr = atoi((&line[offset + 1]));

  offset = skip_comma(line, offset);

  if (line[offset] == 'r') {
    inst.arith.imm = 0;
    inst.arith.rhr = atoi((&line[offset + 1]));
  } else if (line[offset] == '#') {
    inst.arith.imm = 1;
    inst.arith.rhv = strtol(&line[offset + 1],NULL,0); // atoi((&line[offset + 1]));
  } else
    syntax_error(line);

  *ipt = inst;
  return 1;
}

int translate_logic(char *line, uint32_t offset, instr_t *ipt) {
  instr_t inst;
  *(uint32_t *)&inst = 0;

  if (strncmp(&line[offset], "and ", 4) == 0) {
    inst.op = AND;
    offset = skip_spaces(line, offset + 4);
  } else if (strncmp(&line[offset], "or ", 3) == 0) {
    inst.op = OR;
    offset = skip_spaces(line, offset + 3);
  } else if (strncmp(&line[offset], "xor ", 4) == 0) {
    inst.op = XOR;
    offset = skip_spaces(line, offset + 4);
  } else
    return 0;

  if (line[offset] != 'r')
    syntax_error(line);
  inst.logic.lhr = atoi((&line[offset + 1]));

  offset = skip_comma(line, offset);

  if (line[offset] == 'r') {
    inst.logic.imm = 0;
    inst.logic.rhr = atoi((&line[offset + 1]));
  } else if (line[offset] == '#') {
    inst.logic.imm = 1;
    inst.logic.rhv = strtol(&line[offset + 1],NULL,0); // atoi((&line[offset + 1]));
  } else
    syntax_error(line);

  *ipt = inst;
  return 1;
}


int translate_stack(char *line, uint32_t offset, instr_t *ipt) {
  instr_t inst;
  *(uint32_t *)&inst = 0;

  if (strncmp(&line[offset], "push ", 5) == 0) {
    inst.op = PUSH;
    offset = skip_spaces(line, offset + 5);
  } else if (strncmp(&line[offset], "pop ", 4) == 0) {
    inst.op = POP;
    offset = skip_spaces(line, offset + 4);
  } else
    return 0;

  if (line[offset] != 'r')
    syntax_error(line);
  inst.stack.reg = atoi((&line[offset + 1]));

  *ipt = inst;
  return 1;
}

instr_t translate_line(char *line, uint32_t offset) {
#if 0
  char opcode[4];
  opcode[3] = '\0';
  opcode[0] = line[0];
  opcode[1] = line[1];
  opcode[2] = line[2];
#endif
  instr_t inst;
  *(uint32_t *)&inst= 0;

  if (strncmp(&line[offset], "iret", 4) == 0) {
    inst.op = IRET;
    return inst;
  }

  if (strncmp(&line[offset], "halt", 4) == 0) {
    inst.op = HALT;
    return inst;
  }

  if (strncmp(&line[offset], "mov ", 4) == 0) {
    offset = skip_spaces(line, offset + 4);
    return translate_mov(line, offset);
  }

  if (translate_branch(line, offset, &inst))
    return inst;

  if (translate_arith(line, offset, &inst))
    return inst;

  if (translate_logic(line, offset, &inst))
    return inst;

  if (translate_stack(line, offset, &inst))
    return inst;

  syntax_error(line);
  return inst;
}
