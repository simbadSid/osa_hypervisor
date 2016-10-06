
# This is a simple sample/test of the trap mechanism
# (1) we set up the trap vector in memory at 0x500.
#     with the following handlers:
#       - 0x100 memory error handler
#       - 0x200 unknown instruction handler
#       - 0x300 arithmetic handler
#       - 0x400 page fault handler
# (2) we set up a stack, so that we can push/pop
#     Stack top at 0x2000, remember that the stack grows downward
# (3) we do some tests
#     - we force trap on an unknown instruction
#     - we force an arithmetic trap (division by 0)
#     - we force a memory error trap (reading an invalid address)

.=0x0000
# Initialize the trap vector
	mov r22, #0x500  ; load trap vector r22
	mov r0, r22
	mov r1, #0x0100
	mov [r0], r1  ; MEMORY_ERROR
	add r0, #4
	mov r1, #0x0200
	mov [r0], r1  ; UNKNOWN_INSTRUCTION
	add r0, #4
	mov r1, #0x0300
	mov [r0], r1  ; AIRTHMETIC_EXCEPTION
	add r0, #4
	mov r1, #0x0400
	mov [r0], r1  ; PAGE_FAULT

# setup stack
# Remember that the stack grows downward.
	mov r15, #0x2000 ; load stack pointer r15.

# Now let's begin the test
# force first trap, an unknown instruction
	mov r0, #0x1000
	brl r0

 .=0x80 ; will come back here at 0x80
	# force second trap, arithmetic
	xor r0,r0
	div r0,r0

 .=0x90 ; will come back here at 0x90
	# force third trap, memory error (invalid address)
 	xor r0,r0
 	sub r0,#1
 	# will loop here, because the handler does not change r24
 	# so the same instruction will execute again
 	mov r1,[r0]

 .=0xa0   ; will come back here at 0xa0
	halt

# Memory error handler:
#   Should not happen here, so do nothing,
#   Just halt.
.=0x100
	halt ;

# unknown instruction handler
#   Just update the r24 register so that
#   execution continues at 0x80 as intended
#   in this test
.=0x200
	mov r24, #0x80
	iret

# arithmetic handler:
#   Just update the r24 register so that
#   execution continues at 0x80 as intended
#   in this test
.=0x300
	mov r24, #0x90
	iret

# Page fault handler:
#   Should not happen here, so do nothing,
#   Just halt.
.=0x400
	halt
