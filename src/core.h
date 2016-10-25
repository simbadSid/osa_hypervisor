#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

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

#define ALWAYS_INLINE static __inline __attribute__((always_inline, no_instrument_function))

#define SIZE_MEM_ARRAY 0x801000 // 2^19, 128 pages of 4K
#define BASE 0x1000 // Base is at 4K(second page, first page is resrved for I/O

#define STATUS_KERNEL_MODE  (1<<0)
#define STATUS_MMU_ON  (1<<1)
#define STATUS_IRQ_DISABLED  (1<<2)
#define STATUS_TRAPPED  (1<<3)

#define NREGS 26
struct core {
	int32_t regs[NREGS];
	// single IRQ at a time
	struct {
#define IRQ_CLEAR 0
#define IRQ_SET 1
    uint32_t state;
	  uint32_t irqno;
    uint32_t pending;
    struct {
      uint32_t pc;
      uint32_t status;
    } regs;
	} irq;
  struct {
#define TRAP_CLEAR 0
#define TRAP_SET 1
    uint32_t state;
    uint32_t trapno;
    struct {
      uint32_t status;
    } regs;
  } trap;
};

#define MMU_PRESENT (1<<10)
#define MMU_RPROTECTED (1<<11)
#define MMU_WPROTECTED (1<<12)
#define MMU_KPROTECTED (1<<13)

#define REGISTER_PAGE_TABLE_ADDRESS	19
struct page_table_entry		// address is physical: 22 first bits, phys address is equal to entry_nb * 4K + offset
{
	uint16_t entry_nb;		// 10 bits for 1024 entries in all
	uint16_t offset;		// In a 4K page, it is 12 bits wide 10 last ones, control, 1 bit each
	char kernel;			// kernel if set, user otherwise
	char read;				// read ?
	char write;				// write ?
	char copy_on_write;
	char present;			// if not, page fault, allocate if possible, triple fault if no handler or no memory left, present is for a whole page
};

#define MASK_VIRTUAL_ADDRESS_OFFSET			0xFFF
#define MASK_VIRTUAL_ADDRESS_ENTRY_NB		0x3FF000
#define MASK_VIRTUAL_ADDRESS_OFFSET_NB_BIT	12

struct virtual_address
{
	uint16_t entry_nb;		// 10 bits, 1024 entries
	uint16_t offset;		// 12 bits, 4K bytes
};

typedef enum {
  IRQ_RESET,
  IRQ_TIMER,
  IRQ_SERIAL,
	NIRQS
} INTERRUPTS;

typedef enum {
  TRAP_MEMORY_ERROR, // wrong memory access without paging
  TRAP_UNKNOWN_INSTRUCTION, // this kind of error should be NMI
  TRAP_ARITHMETIC,
  TRAP_PAGE_FAULT,
  NTRAPS
} TRAPS;

// Values for page faults
#define PAGE_KERNEL_PROTECTED  1
#define PAGE_READ_PROTECTED    2
#define PAGE_WRITE_PROTECTED   3
#define PAGE_NOT_PRESENT       4

/**
  * Decoding loop, read the next instruction and execute it
  **/
void decoding_loop(void);

void trap(int trapno, uint32_t maddr, uint32_t infos);


/**
 * Raise the interruption "interrupt"
 * Switching interrupt off is responsibility of the programmer, PC is saved in a special stack
 **/
void raise_interrupt(INTERRUPTS interrupt);

/**
  * Memory Management Unit
  *   address translation, based on an architected page table.
  *   translates virtual addresses into physical addresses.
  *   checking access right in respect to mode(load/store)
  *
  * The translation may trap, which is recorded in the status register
  * of the core, setting bit STATUS_TRAPPED.
  */
uint32_t  mmu_translate(uint32_t virt, char mode);

/**
 * Issues a load on the bus, at the given address.
 * The load may trap, which is recorded in the status register
 * of the core, setting bit STATUS_TRAPPED.
 */
uint32_t  bus_load(uint32_t addr);

/**
 * Issues a store on the bus, at the given address, with the given value.
 * The store may trap, which is recorded in the status register
 * of the core, setting bit STATUS_TRAPPED.
 */
void      bus_store(uint32_t addr, uint32_t value);

void bus_emul_devices(void);

void bus_init(void);

/***********************************************************************************************************/

extern void _echoi(instr_t instr);
#ifdef CONFIG_DEBUG
#define echoi(instr) _echoi(instr)
#else
#define echoi(instr)
#endif /* CONFIG_DEBUG */

#endif /* __INTERPRETER_H__ */
