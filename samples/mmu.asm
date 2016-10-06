
# This is a simple sample/test of the MMU.
# (1) we set up the trap vector in memory at 0x500.
#     with the following handlers:
#       - 0x100 memory error handler
#       - 0x200 unknown instruction handler
#       - 0x300 arithmetic handler
#       - 0x400 page fault handler
# (2) we setup a stack, so that we can push/pop
#     Stack top at 0x3FF0, remember that the stack grows downward
# (3) we setup a page table.
#     the page table itself is at 0x2000.
#     At 0x0000-0x0FFF we have this code, etc.
#     At 0x1000-0x1FFF we have device mmio registers.
#     At 0x2000-0x2FFF we have the page table.
#     At 0x3000-0x3FFF we have our stack.
#     At 0x4000-0x4FFF we have free memory.

.=0x0000
# Initialize the trap vector
	mov r22, #0x500
	mov r0, r22
	mov r1, #0x0100
	mov [r0], r1  ; MEMORY_ERROR
	add r0, #4
	mov r1, #0x0200
	mov [r0], r1  ; UNKNOWN_INSTRUCTION
	add r0, #4
	mov r1, #0x0300
	mov [r0], r1  ; ARITHMETIC_EXCEPTION
	add r0, #4
	mov r1, #0x0400
	mov [r0], r1  ; PAGE_FAULT

# setup stack at 0x3FF0
 	mov r15, #0x3FF0 ; load stack pointer r15.

# Setup the page table at 0x2000
	# load the page table address in r19
	mov r19, #0x2000
	# now let's fill up the first entries
	mov r0, r19
	# Page-0 (0x0000-0x0FFF)
	# contains this code, it must be present.
	mov r1, #0x400
	mov [r0], r1
	# Page-1 (0x1000-0x1FFF)
	# contains devices, it must be present.
	mov r1, #0x401
	add r0, #4
	mov [r0], r1
	# Page-2 (0x2000-0x2FFF)
	# contains the page table, it must be present.
	mov r1, #0x402  ; PRESENT, PAGE 0x2000-0x2FFF
	add r0, #4
	mov [r0], r1
	# Page-3 (0x3000-0x3FFF)
	# contains the stack, must be presetn
	mov r1, #0x403  ; PRESENT, PAGE 0x2000-0x2FFF
	add r0, #4
	mov [r0], r1
	# Page-4 (0x4000-0x4FFF)
	# Not present, will generate a page fault
	xor r3, r3
	add r0, #4
	mov [r0], r3

# We now enable the MMU
	mov r1, #2
	or r18, r1

# If you are still running after enabling the MMU,
# it is a good sign!
# Indeed, you are reading from page-0 when executing
# the following instructions (below 0x0FFF).
# Also, you are reading from page-2 since our page table
# is at 0x2000.

	# test reading from page-1 (devices)
	mov r0, #0x1000
	mov r1, [r0]
	# test reading/writing from the page hosting our stack
	mov r0, #0x3000
	mov r1, #0x1234
	mov [r0], r1
	mov r1, [r0]
	# Now, let's read from page-4 at 0x4000, since the page
	# is not present, we should get a page fault.
	# In trap handler, we will setup the virtual page 0x4000
	# at physical page 0x5000, shared with the virtual page 0x5000.
	mov r0, #0x4000
	mov r1, #0x4321
	mov [r0], r1
	mov r2, [r0]
	mov r3, r16
	bne r3, r1, r2
	mov r0, #0x5000
	mov r2, [r0]
	mov r3, r16
	bne r3, r1, r2
	halt

.=0x100  ; memory error handler
	mov r0, r16
	brl r0 ; loop forever

.=0x200  ; unknown instruction handler
	mov r0, r16
	brl r0 ; loop forever

.=0x300  ; arithmetic handler
	mov r0, r16
	brl r0 ; loop forever

.=0x400  ; page fault handler
	# first, save the registers that we will clobber
	push r0
	push r1
	push r2
	# verfiy that we trapped at the expected address 0x4000
	# the address that generated the page fault is available in r25
	mov r0, r25     ; the address that trapped
	and r0, #0xF000
	mov r2, #0x4000
	mov r1, r16
	bne r1, r0, r2 ; loop forever on error
	# OK, so we got the right page fault here
	# We will mark the virtual page 0x4000 & 0x5000 as available,
	# sharing the same physical page 0x5000.
	mov r0, r19
	add r0, #16
	# Virtual page 0x4000 set to physical 0x5000
	mov r1, #0x405
	mov [r0], r1
	# Virtual page 0x5000 also set to physical 0x5000
	add r0, #4
	mov [r0], r1
	# pop back the registers we save earlier
	pop r2
	pop r1
	pop r0
	iret
