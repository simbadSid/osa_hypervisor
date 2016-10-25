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

/*
 * Memory complex instructions
 *    - memory fetch/store
 *    - stack push/pop
 */
void fetch(reg_t  dest,reg_t  addr, int offset);
void store(reg_t addr,reg_t src, int offset);
void push(reg_t reg);
void pop(reg_t reg);

/**
  * Arithmetic instructions:
  *    dest = dest OP src
  */
void add(reg_t dest, reg_t src);
void sub(reg_t dest, reg_t src);
void mul(reg_t dest, reg_t src);
void division(reg_t dest, reg_t src);

/**
  * Branch(and link), branch to specified address and save current pc to link register
  * return 0 if all is OK
  * return 1 if address is not reachable or special address
  **/
void branch_and_link(uint32_t addr);

/**
  * ret : allows to return from an interrupt
  * pc and mode are restored as before
  **/
void iret(void);

extern uint8_t *memory; // physical memory array, allocate globally
extern struct core* core; // registers and all

instr_t fetchi(int32_t *regs, uint8_t *memory) {
  instr_t inst;
  uint32_t pc = regs[16];
  uint32_t status = regs[18];

  uint8_t trapped=0;
  loop:
  if (status & STATUS_MMU_ON) {
    pc = mmu_translate(pc,'r');
    if (regs[18] & STATUS_TRAPPED) {
      if (trapped)
        goto reset;
      trapped = 1;
      regs[18] &= ~STATUS_TRAPPED;
      // since we trapped, we need to fetch the first instruction of the trap handler.
      pc = regs[16];
      goto loop;
    }
  }
  *(uint32_t *) &inst = bus_load(pc);
  if (regs[18] & STATUS_TRAPPED) {
    if (trapped)
      goto reset;
    trapped = 1;
    regs[18] &= ~STATUS_TRAPPED;
    // since we trapped, we need to fetch the first instruction of the trap handler.
    pc = regs[16];
    goto loop;
  }

  echoi(inst); // echo the instruction of debug purposes.
  regs[16] = pc + 4; // increment pc
  return inst;

  reset: /* that's it, we trapped while trying to fetch the trap handler first instruction. */
  regs[18] &= ~STATUS_TRAPPED;
  regs[18] &= ~STATUS_MMU_ON;
  regs[18] |= STATUS_IRQ_DISABLED;
  regs[18] |= STATUS_KERNEL_MODE;
  pc = 0x00000000;
  *(uint32_t *) &inst = bus_load(pc);
  regs[16] = pc + 4; // increment pc
  return inst;

}

/**
 * A trap occurred with the current instruction,
 * when issuing the instruction (trying to execute it).
 *
 * In all cases, the semantics of a trap is that the instruction
 * execution has been aborted as if it never began executing.
 * This will jump to the trap handler, that will return using a reti.
 * Several outcomes are possible here from the handler.
 *
 * 1) The instruction will never be executed, but emulated.
 *    The PC is left unchanged.
 * 2) The PC is moved back on the trapping instruction
 *    for re-execution after the trap cause has been fixed.
 *    The instruction can have been changed.
 * 3) The PC is changed entirely, jumping to some other code.
 */
void trap(int trapno, uint32_t maddr, uint32_t infos) {
  int32_t *regs = core->regs;
  uint32_t pc = regs[16];
  regs[24] = pc-4;
  regs[23] = infos;
  regs[25] = maddr;

  uint32_t addr = (regs[22] + (trapno << 2));

  pc = bus_load(addr);
  if (regs[18] & STATUS_TRAPPED)
    goto reset;
  regs[16] = pc;
  regs[18] |= STATUS_TRAPPED;
  regs[18] |= STATUS_IRQ_DISABLED;
  regs[18] |= STATUS_KERNEL_MODE;
  core->trap.state |= TRAP_SET;
  core->trap.regs.status = regs[18];
  return;

  reset: /* that's it, we trapped while trying to fetch the trap handler first instruction. */
  regs[18] &= ~STATUS_TRAPPED;
  regs[18] &= ~STATUS_MMU_ON;
  regs[18] |= STATUS_IRQ_DISABLED;
  regs[18] |= STATUS_KERNEL_MODE;
  regs[16] = 0x00000000;
  return;
}

int check_interrupts(void);

/**
 * This is the heart of the emulation of a core.
 * This is the core loop:
 *   fetching, decoding, and issuing instructions.
 * Everything starts here.
 */
void decoding_loop(void)
{
	instr_t instr;
	int32_t *regs = core->regs;

	srand(time(NULL));
	int halt = 0;
	while (!halt)
	{
		bus_emul_devices();
		check_interrupts();
		instr = fetchi(regs, memory); // fetch and decode
		switch (instr.op)
		{
		case MOV: // mov between registers
		  regs[instr.mov.dst] = regs[instr.mov.src];
		  break;
		case LOAD: // load immediate value
		  regs[instr.load.dst] = instr.load.value;
		  break;
		case FETCH:
		  fetch(instr.fetch.dst, instr.fetch.src, instr.fetch.offset);
		  regs[18] &= ~STATUS_TRAPPED;
		  break;
		case STORE:
		  store(instr.store.dst, instr.store.src, instr.store.offset);
		  regs[18] &= ~STATUS_TRAPPED;
		  break;
		case ADD: // add
		  if (instr.arith.imm)
			regs[instr.arith.lhr] = regs[instr.arith.lhr] + instr.arith.rhv;
		  else
			regs[instr.arith.lhr] = regs[instr.arith.lhr] + regs[instr.arith.rhr];
		  break;
		case SUB: // sub
		  if (instr.arith.imm)
			regs[instr.arith.lhr] = regs[instr.arith.lhr] - instr.arith.rhv;
		  else
			regs[instr.arith.lhr] = regs[instr.arith.lhr] - regs[instr.arith.rhr];
		  break;
		case MUL: // mul
		  if (instr.arith.imm)
			regs[instr.arith.lhr] = regs[instr.arith.lhr] * instr.arith.rhv;
		  else
			regs[instr.arith.lhr] = regs[instr.arith.lhr] * regs[instr.arith.rhr];
		  break;
		case DIV: // div
		  if (instr.arith.imm) {
			if (instr.arith.rhv == 0) {
			  trap(TRAP_ARITHMETIC,0,0);
			  regs[18] &= ~STATUS_TRAPPED;
			  break;
			}
			regs[instr.arith.lhr] = regs[instr.arith.lhr] / instr.arith.rhv;
		  } else {
			if (regs[instr.arith.rhr] == 0) {
			  trap(TRAP_ARITHMETIC,0,0);
			  regs[18] &= ~STATUS_TRAPPED;
			  break;
			}
			regs[instr.arith.lhr] = regs[instr.arith.lhr] / regs[instr.arith.rhr];
		  }
		  break;
		case BRL: // branch
		  branch_and_link(instr.brl.reg);
		  break;
		case BEQ: // conditional branch
		  if (instr.bcond.imm) {
			if (regs[instr.bcond.lhr] == instr.bcond.rhv)
			  core->regs[16] = regs[instr.bcond.dst];
		  } else {
			if (regs[instr.bcond.lhr] == regs[instr.bcond.rhr])
			  core->regs[16] = regs[instr.bcond.dst];
		  }
		  break;
		case BNE: // conditional branch
		  if (instr.bcond.imm) {
			if (regs[instr.bcond.lhr] != instr.bcond.rhv)
			  core->regs[16] = regs[instr.bcond.dst];
		  } else {
			if (regs[instr.bcond.lhr] != regs[instr.bcond.rhr])
			  core->regs[16] = regs[instr.bcond.dst];
		  }
		  break;
		case BLT: // conditional branch
		  if (instr.bcond.imm) {
			if (regs[instr.bcond.lhr] < instr.bcond.rhv)
			  core->regs[16] = regs[instr.bcond.dst];
		  } else {
			if (regs[instr.bcond.lhr] < regs[instr.bcond.rhr])
			  core->regs[16] = regs[instr.bcond.dst];
		  }
		  break;
		case BGT: // conditional branch
		  if (instr.bcond.imm) {
			if (regs[instr.bcond.lhr] > instr.bcond.rhv)
			  core->regs[16] = regs[instr.bcond.dst];
		  } else {
			if (regs[instr.bcond.lhr] > regs[instr.bcond.rhr])
			  core->regs[16] = regs[instr.bcond.dst];
		  }
		  break;
		case AND:
		  if (instr.logic.imm)
			regs[instr.logic.lhr] = regs[instr.logic.lhr] & instr.logic.rhv;
		  else
			regs[instr.logic.lhr] = regs[instr.logic.lhr] & regs[instr.logic.rhr];
		  break;
		case OR:
		  if (instr.logic.imm)
			regs[instr.logic.lhr] = regs[instr.logic.lhr] | instr.logic.rhv;
		  else
			regs[instr.logic.lhr] = regs[instr.logic.lhr] | regs[instr.logic.rhr];
		  break;
		case XOR:
		  if (instr.logic.imm)
			regs[instr.logic.lhr] = regs[instr.logic.lhr] ^ instr.logic.rhv;
		  else
			regs[instr.logic.lhr] = regs[instr.logic.lhr] ^ regs[instr.logic.rhr];
		  break;
		case PUSH:
		  push(instr.stack.reg);
		  regs[18] &= ~STATUS_TRAPPED;
		  break;
		case POP:
		  pop(instr.stack.reg);
		  regs[18] &= ~STATUS_TRAPPED;
		  break;
		case HALT:
		  do {
			bus_emul_devices();
		  } while (!check_interrupts());
		  break;
		case IRET: // ret : return from an interrupt, restore registers that need to be
		  iret();
		  break;
		default:
		  trap(TRAP_UNKNOWN_INSTRUCTION,0,0);
		  regs[18] &= ~STATUS_TRAPPED;
		  break;
		}
	}
}

/**
 * This is a wrapper implementing a fetch from memory.
 * If the MMU is turned on, the address is translated.
 */
void fetch(reg_t reg_dest, reg_t reg_addr, int offset) {
  int32_t *regs = core->regs;
  uint32_t addr = regs[reg_addr] + offset;

  if (regs[18] & STATUS_MMU_ON) {
    addr = mmu_translate(addr,'r');
    if (regs[18] & STATUS_TRAPPED)
      return;
  }
  uint32_t value;
  value = bus_load(addr);
  if (regs[18] & STATUS_TRAPPED)
    return;
  regs[reg_dest] = value;
  return;
}

/**
 * Push the given register value on the stack.
 * Exceptions:
 *  - invalid address:
 *    the trap handler must solve the problem,
 *    either change the instruction and reset the PC
 *    or jump to somewhere else, changing the PC
 *  - page fault:
 *    the page fault handler will correct the
 *    situation and reset the PC to re-execute the
 *    instruction, or it will trap to memory error.
 */
void store(reg_t reg_addr, reg_t reg_src, int offset) {
  int32_t *regs = core->regs;
  uint32_t addr = regs[reg_addr] + offset;
  if (regs[18] & STATUS_MMU_ON) {
    addr = mmu_translate(addr,'w');
    if (regs[18] & STATUS_TRAPPED) {
      regs[18] &= ~STATUS_TRAPPED;
      return;
    }
  }
  bus_store(addr,regs[reg_src]);
  return;
}

/**
 * Pop 32bit value from the stack to the given register.
 * Exceptions:
 *  - invalid address:
 *    the trap handler must solve the problem,
 *    either change the instruction and reset the PC
 *    or jump to somewhere else, changing the PC
 *  - page fault:
 *    the page fault handler will correct the
 *    situation and reset the PC to re-execute the
 *    instruction, or it will trap to memory error.
 */
void pop(reg_t reg) {
  int32_t *regs = core->regs;
  uint32_t addr = regs[15];
  regs[15] = addr + 4;

  if (regs[18] & STATUS_MMU_ON) {
    addr = mmu_translate(addr,'r');
    if (regs[18] & STATUS_TRAPPED)
      return;
  }
  uint32_t value;
  value = bus_load(addr);
  if (regs[18] & STATUS_TRAPPED)
    return;
  regs[reg] = value;
  return;
}

/**
 * Push the given register value on the stack.
 * Exceptions:
 *  - invalid address:
 *    the trap handler must solve the problem,
 *    either change the instruction and reset the PC
 *    or jump to somewhere else, changing the PC
 *  - page fault:
 *    the page fault handler will correct the
 *    situation and reset the PC to re-execute the
 *    instruction, or it will trap to memory error.
 */
void push(reg_t reg) {
  int32_t *regs = core->regs;
  uint32_t addr = regs[15] - 4;
  regs[15] = addr;

  if (regs[18] & STATUS_MMU_ON) {
    addr = mmu_translate(addr,'w');
    if (regs[18] & STATUS_TRAPPED) {
      regs[18] &= ~STATUS_TRAPPED;
      return;
    }
  }
  bus_store(addr,regs[reg]);
  return;
}


void branch_and_link(uint32_t addr) {
  int32_t *regs = core->regs;
  regs[17] = regs[16];
  regs[16] = regs[addr];
}

/**
 * This emulates the core deciding on processing interrupts or not.
 * What happens here depends on the CPU configuration.
 */

int check_interrupts(void) {
  int32_t *regs = core->regs;
  // are IRQs enabled?
  if (regs[18] & STATUS_IRQ_DISABLED)
    return 0;
  // are handling an IRQ already?
  if (core->irq.state & IRQ_SET)
    return 0;
  // do we have pending IRQs?
  if (core->irq.pending) {
    int irq;
    // let's find out which IRQs are pending...
    for (irq = 0; irq < NIRQS; irq++) {
      if (core->irq.pending & (1 << irq)) {
        // the irq is pending, is it enabled?
        if (regs[21] & (1 << irq)) {
          // pending and enabled, let's dispatch it
          core->irq.pending &= ~(1 << irq);
          core->irq.state |= IRQ_SET;
          core->irq.regs.pc = regs[16];
          core->irq.regs.status = regs[18];

          uint32_t offset = (regs[20] + (irq << 2));
          uint32_t* addr = ((uint32_t*) (memory + offset));
          uint32_t pc = *addr;
          regs[16] = pc;
          regs[18] |= 0x01; // Going to kernel mode
          regs[18] |= STATUS_IRQ_DISABLED; // Disable interrupt
          return 1;
        }
      }
    }
  }
  return 0;
}

/**
 * Wrapper emulating the iret instruction, that is,
 * the return from an interrupt service routine.
 */
void iret(void) {
  int32_t *regs = core->regs;
  if (0 != (core->irq.state & IRQ_SET)) {
    core->irq.state &= ~IRQ_SET;
    regs[16] = core->irq.regs.pc;
    regs[18] = core->irq.regs.status;
    check_interrupts();
  } else if (0 != (core->trap.state & TRAP_SET)) {
    core->trap.state &= ~TRAP_SET;
    regs[16] = regs[24];
    regs[18] = core->trap.regs.status;
  } else {
    printf("PANIC: iret, not handling interrupt \n");
    exit(-1);
  }
}

/*
 * This is called to raise an interrupt, similating what a
 * device would do when raising its interrupt line to the
 * processor or to the Programmable Interrupt Controller (PIC).
 * Notice that we do not emulate a PIC for simplicity.
 */
void raise_interrupt(INTERRUPTS irq) {
  core->irq.pending |= (1 << irq);
}

/**
 * Memory translation --
 * Emulate a one-level architected page table
 * So one page table, with 1024 entries of 32bit each.
 *
 * If the PC register is unchanged, the translation succeeds and
 * the returned value is the translated address,
 *
 * If the translation failed, the trap handler must have changed
 * the PC register (either to re-execute the instruction once the problem
 * has been resolved, or jump somewhere else.
 *
 * address space: 22 bits = 4MB = 1024 * 4KB pages
 *    first 12 bits = offset in the page
 *    next  10 bits = page number, [0:1024[ pages
 * page entry :
 *    first 10 bits = physical address of the 4KB page
 *    bit [10] :  present
 *    bit [11] :  1 if read protected
 *    bit [12] :  1 if write protected
 *    bit [13] :  1 if kernel protected
 */
uint32_t mmu_translate(uint32_t addr, char mode)
{
	struct virtual_address addrV;
	struct page_table_entry pageTableEntry;
	uint32_t *addrPageTableEntryAddr = (uint32_t*)(memory + core->regs[REGISTER_PAGE_TABLE_ADDRESS]), pageTableEntryVal;

	addrV.offset			= addr & MASK_VIRTUAL_ADDRESS_OFFSET;
	addrV.entry_nb			= (addr & MASK_VIRTUAL_ADDRESS_ENTRY_NB) >> MASK_VIRTUAL_ADDRESS_OFFSET_NB_BIT;
	pageTableEntryVal		= *(addrPageTableEntryAddr + addrV.entry_nb);

	pageTableEntry.offset	= pageTableEntryVal & 0x3FF;
	pageTableEntry.present	= pageTableEntryVal & MMU_PRESENT;
	pageTableEntry.read		= pageTableEntryVal	& MMU_RPROTECTED;
	pageTableEntry.write	= pageTableEntryVal	& MMU_WPROTECTED;
	pageTableEntry.kernel	= pageTableEntryVal	& MMU_KPROTECTED;

	uint32_t infos = 0;
	int32_t statusKernelMode = core->regs[18] & STATUS_KERNEL_MODE;

	while(1)
	{
		if		(!pageTableEntry.present)							infos = PAGE_NOT_PRESENT;
		else if ((mode == 'r')			&& (!pageTableEntry.read))	infos = PAGE_READ_PROTECTED;
		else if ((mode == 'W')			&& (!pageTableEntry.write))	infos = PAGE_WRITE_PROTECTED;
		else if ((!statusKernelMode)	&& (!pageTableEntry.kernel))infos = PAGE_KERNEL_PROTECTED;
		else							break;
		int32_t pc = core->regs[16];
		trap(TRAP_PAGE_FAULT, addr, infos);
		if (pc != core->regs[16])
			continue;
	}

	return (pageTableEntry.offset << 12);
}
