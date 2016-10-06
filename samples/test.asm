.=0x0000
	mov r3, #0x100
	mov r1, #2 ; load immediate value
	mov r2, #3 ; load immediate value
	add r1, r2    ; test addition
	bne r3,r1,#5  ; test compare & no branch
	sub r1, #1    ; test substraction
	mov r4, r16   ; get at PC
	add r4, #12   ; add 12 to jump over the brl
	beq r4,r1,#4  ; test compare and branch
	halt          ; ERROR, we should not get here
	div r1, #2    ; test division
	bne r3,r1,#2  ; compare and branch if something wrong
	mul r1, r1    ; test multiplication
	bne r3,r1,#4  ; compare and branch if something wrong

	# setup stack
 	mov r15, #0x2000 ; load stack pointer, stack grow downward.
 	# test pushing and popping
	push r1
	pop r0
	bne r3,r1,r0  ; compare and branch if something wrong

	# setup stack at 0x2000
 	mov r15, #0x2000 ; load stack pointer r15.

 	# Testing a simple calling convention through the stack
 	# all arguments are passed by registers
 	# return value is in r0 (only 32bit values).
	# R0, R1, R3, R4 are scratch registers, clobbered by function calls.

	# do test this with a recursive function: fact(N)
	mov r2, #0x210
	mov r0, #4
	brl r2
	# result is in r0
	# and should be 0x18
	mov r2, r16       ; get PC+4
	bne r2, r0, #0x18 ; forever loop if the result is not correct

	# All is good then.
	halt

.=0x100
	halt  ; if you get here, something is wrong.

# this is a generic epilog,
# assuming all local push to save stuff have been popped
.=0x200  ; func epilog
	mov r15,r14
	pop r14
	pop r16

.=0x210  ; fact(N) prolog
	push r17       ; save return address
	push r14       ; push frame pointer
	mov r14, r15   ; set new frame pointer
	sub r15, #4
	mov r17, #0x200 ; set epilog
	# this is end of the prolog
	beq r17, r0, #1
	mov [r14-4], r0
	sub ro,#1
	mov r2, #0x210
	brl r2
	mov r1, [r14-4]
	mul r0, r1
	mov r16, #0x200 ; -> epilog
