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

#include "core.h"

extern uint8_t *memory; // physical memory array, allocate globally
extern struct core* core; // registers and all

void echoi_bcond(instr_t instr) {
  int32_t *regs = core->regs;
  switch (instr.op) {
  case BEQ: // conditional branch
    printf("\t BEQ ");
    break;
  case BNE: // conditional branch
    printf("\t BNE ");
    break;
  case BLT: // conditional branch
    printf("\t BLT ");
    break;
  case BGT: // conditional branch
    printf("\t BGT ");
    break;
  }
  if (instr.bcond.imm)
    printf("r%d r%d #%d--> @ 0x%x r%d=0x%x \n", instr.bcond.dst,
        instr.bcond.lhr, instr.bcond.rhv, regs[instr.bcond.dst],
        instr.bcond.lhr, regs[instr.bcond.lhr]);
  else
    printf("r%d r%d r%d--> @ 0x%x r%d=0x%x \n", instr.bcond.dst,
        instr.bcond.lhr, instr.bcond.rhr, regs[instr.bcond.dst],
        instr.bcond.lhr, regs[instr.bcond.lhr]);
}

void echoi_arith(instr_t instr) {
  int32_t *regs = core->regs;
  switch (instr.op) {
  case ADD: // add
    if (instr.arith.imm)
      printf("\t ADD r%d, #%d <-- 0x%x = 0x%x + 0x%x \n", instr.arith.lhr,
          instr.arith.rhv, regs[instr.arith.lhr] + instr.arith.rhv,
          regs[instr.arith.lhr], instr.arith.rhv);
    else
      printf("\t ADD r%d, r%d <-- 0x%x = 0x%x + 0x%x \n", instr.arith.lhr,
          instr.arith.rhr, regs[instr.arith.lhr] + regs[instr.arith.rhr],
          regs[instr.arith.lhr], regs[instr.arith.rhr]);
    break;
  case SUB: // sub
    if (instr.arith.imm)
      printf("\t SUB r%d, #%d <-- 0x%x = 0x%x - 0x%x \n", instr.arith.lhr,
          instr.arith.rhv, regs[instr.arith.lhr] - instr.arith.rhv,
          regs[instr.arith.lhr], instr.arith.rhv);
    else
      printf("\t SUB r%d, r%d <-- 0x%x = 0x%x - 0x%x \n", instr.arith.lhr,
          instr.arith.rhr, regs[instr.arith.lhr] - regs[instr.arith.rhr],
          regs[instr.arith.lhr], regs[instr.arith.rhr]);
    break;
  case MUL: // mul
    if (instr.arith.imm)
      printf("\t MUL r%d, #%d <-- 0x%x = 0x%x + 0x%x \n", instr.arith.lhr,
          instr.arith.rhv, regs[instr.arith.lhr] * instr.arith.rhv,
          regs[instr.arith.lhr], instr.arith.rhv);
    else
      printf("\t MUL r%d, r%d <-- 0x%x = 0x%x * 0x%x \n", instr.arith.lhr,
          instr.arith.rhr, regs[instr.arith.lhr] * regs[instr.arith.rhr],
          regs[instr.arith.lhr], regs[instr.arith.rhr]);
    break;
  case DIV: // div
    if (instr.arith.imm) {
      if (instr.arith.rhv)
        printf("\t DIV r%d, #%d <-- 0x%x = 0x%x / 0x%x \n", instr.arith.lhr,
            instr.arith.rhv, regs[instr.arith.lhr] / instr.arith.rhv,
            regs[instr.arith.lhr], instr.arith.rhv);
      else
        printf("\t DIV r%d, #%d <-- NaN = 0x%x / 0x%x \n", instr.arith.lhr,
            instr.arith.rhv, regs[instr.arith.lhr], instr.arith.rhv);

    } else {
      if (regs[instr.arith.rhr])
        printf("\t DIV r%d, r%d <-- 0x%x = 0x%x / 0x%x \n", instr.arith.lhr,
            instr.arith.rhr, regs[instr.arith.lhr] / regs[instr.arith.rhr],
            regs[instr.arith.lhr], regs[instr.arith.rhr]);
      else
        printf("\t DIV r%d, r%d <-- NaN = 0x%x / 0x%x \n", instr.arith.lhr,
            instr.arith.rhr, regs[instr.arith.lhr], regs[instr.arith.rhr]);
    }
    break;
  }
}

void echoi_logical(instr_t instr) {
  int32_t *regs = core->regs;
  switch (instr.op) {
  case AND:
    if (instr.logic.imm)
      printf("\t AND r%d #%d  -> 0x%x= 0x%x & 0x%x\n", instr.logic.lhr,
          instr.logic.rhv, regs[instr.logic.lhr] & instr.logic.rhv,
          regs[instr.logic.lhr], instr.logic.rhv);
    else
      printf("\t AND r%d r%d  -> 0x%x= 0x%x & 0x%x\n", instr.logic.lhr,
          instr.logic.rhr, regs[instr.logic.lhr] & regs[instr.logic.rhr],
          regs[instr.logic.lhr], regs[instr.logic.rhr]);
    break;
  case OR:
    if (instr.logic.imm)
      printf("\t OR r%d #%d  -> 0x%x= 0x%x | 0x%x\n", instr.logic.lhr,
          instr.logic.rhv, regs[instr.logic.lhr] | instr.logic.rhv,
          regs[instr.logic.lhr], instr.logic.rhv);
    else
      printf("\t OR r%d r%d  -> 0x%x= 0x%x | 0x%x\n", instr.logic.lhr,
          instr.logic.rhr, regs[instr.logic.lhr] | regs[instr.logic.rhr],
          regs[instr.logic.lhr], regs[instr.logic.rhr]);
    break;
  case XOR:
    if (instr.logic.imm)
      printf("\t XOR r%d #%d  -> 0x%x= 0x%x ^ 0x%x\n", instr.logic.lhr,
          instr.logic.rhv, regs[instr.logic.lhr] ^ instr.logic.rhv,
          regs[instr.logic.lhr], instr.logic.rhv);
    else
      printf("\t XOR r%d r%d  -> 0x%x= 0x%x ^ 0x%x\n", instr.logic.lhr,
          instr.logic.rhr, regs[instr.logic.lhr] ^ regs[instr.logic.rhr],
          regs[instr.logic.lhr], regs[instr.logic.rhr]);
    break;
  }
}

void _echoi(instr_t instr) {
  uint32_t addr, translated;
  uint32_t value;
  int32_t *regs = core->regs;
  uint32_t pc = regs[16];
  printf("%6x 0x%8x", pc, *(uint32_t*)&instr);
  switch (instr.op) {
  case MOV: // mov between registers
    printf("\t MOV r%d, r%d  {0x%x} \n", instr.mov.dst, instr.mov.src,
        regs[instr.mov.src]);
    break;
  case LOAD: // load immediate value
    printf("\t MOV r%d, #0x%x   \n", instr.load.dst, instr.load.value);
    break;
  case FETCH:
    addr = regs[instr.fetch.src] + instr.fetch.offset;
    if (addr < SIZE_MEM_ARRAY) {
      if (core->regs[18] & 0x2) {
        uint32_t page = addr >> 12;
        uint32_t offset = addr & 0x00000FFF;

        uint32_t entry = *((uint32_t*) (memory + core->regs[19] + (page * 4)));
        if (0 != (entry & (1 << 10))) {
          translated = (((entry & 0x00003FF) * 4096) + offset);
          value = *(uint32_t*) (memory + translated);
          if (instr.fetch.offset==0)
            printf("\t MOV r%d, [r%d] <-- [0x%x]=0x%x  \n", instr.fetch.dst,
              instr.fetch.src, addr, value);
          else if (instr.fetch.offset>0)
            printf("\t MOV r%d, [r%d + %d] <-- [0x%x]=0x%x  \n", instr.fetch.dst,
                instr.fetch.src, instr.fetch.offset, addr, value);
          else
            printf("\t MOV r%d, [r%d - %d] <-- [0x%x]=0x%x  \n", instr.fetch.dst,
                instr.fetch.src, -instr.fetch.offset, addr, value);
        } else {
          if (instr.fetch.offset==0)
          printf("\t MOV r%d, [r%d] <-- [0x%x]=NOT_PRESENT  \n",
              instr.fetch.dst, instr.fetch.src, addr);
          else if (instr.fetch.offset>0)
            printf("\t MOV r%d, [r%d + %d] <-- [0x%x]=NOT_PRESENT  \n",
                instr.fetch.dst, instr.fetch.src, instr.fetch.offset, addr);
          else
            printf("\t MOV r%d, [r%d - %d] <-- [0x%x]=NOT_PRESENT  \n",
                instr.fetch.dst, instr.fetch.src, -instr.fetch.offset, addr);
        }
      } else {
        value = *(uint32_t*) (memory + addr);
        if (instr.fetch.offset==0)
          printf("\t MOV r%d, [r%d] <-- [0x%x]=0x%x  \n", instr.fetch.dst,
              instr.fetch.src, addr, value);
        else if (instr.fetch.offset>0)
          printf("\t MOV r%d, [r%d + %d] <-- [0x%x]=0x%x  \n", instr.fetch.dst,
              instr.fetch.src, instr.fetch.offset, addr, value);
        else
          printf("\t MOV r%d, [r%d - %d] <-- [0x%x]=0x%x  \n", instr.fetch.dst,
              instr.fetch.src, -instr.fetch.offset, addr, value);
      }
    } else {
      if (instr.fetch.offset==0)
        printf("\t MOV r%d, [r%d] <-- [0x%x]=INVALID  \n", instr.fetch.dst,
            instr.fetch.src, addr);
      else if (instr.fetch.offset>0)
        printf("\t MOV r%d, [r%d + %d] <-- [0x%x]=INVALID  \n", instr.fetch.dst,
            instr.fetch.src, instr.fetch.offset,addr);
      else
        printf("\t MOV r%d, [r%d - %d] <-- [0x%x]=INVALID  \n", instr.fetch.dst,
            instr.fetch.src, -instr.fetch.offset, addr);

    }
    break;
  case STORE:
    addr = regs[instr.store.dst] + instr.store.offset;
    if (instr.store.offset==0)
      printf("\t MOV [r%d], r%d <-- [0x%x]=0x%x \n", instr.store.dst,
          instr.store.src, addr, regs[instr.store.src]);
    else if (instr.store.offset > 0)
      printf("\t MOV [r%d + %d], r%d <-- [0x%x]=0x%x \n", instr.store.dst,
          instr.store.offset, instr.store.src, addr, regs[instr.store.src]);
    else
      printf("\t MOV [r%d - %d], r%d <-- [0x%x]=0x%x \n", instr.store.dst,
          -instr.store.offset, instr.store.src, addr, regs[instr.store.src]);
    break;
  case ADD: // add
    echoi_arith(instr);
    break;
  case SUB: // sub
    echoi_arith(instr);
    break;
  case MUL: // mul
    echoi_arith(instr);
    break;
  case DIV: // div
    echoi_arith(instr);
    break;
  case BRL: // branch
    printf("\t BRL r%d --> @ 0x%x \n", instr.brl.reg, regs[instr.brl.reg]);
    break;
  case BEQ: // conditional branch
    echoi_bcond(instr);
    break;
  case BNE: // conditional branch
    echoi_bcond(instr);
    break;
  case BLT: // conditional branch
    echoi_bcond(instr);
    break;
  case BGT: // conditional branch
    echoi_bcond(instr);
    break;
  case AND:
    echoi_logical(instr);
    break;
  case OR:
    echoi_logical(instr);
    break;
  case XOR:
    echoi_logical(instr);
    break;
  case PUSH:
    printf("\t PUSH r%d (0x%x)\n", instr.stack.reg, regs[instr.stack.reg]);
    break;
  case POP:
    printf("\t POP r%d \n", instr.stack.reg);
    break;
  case HALT:
    printf("\t HALT\n");
    return;
  case IRET: // ret : return from an interrupt, restore registers that need to be
    printf("\t IRET\n");
    break;
  default:
    printf("  Unknown op=%d \n", instr.op);
  }
  fflush(stdout);
}

