.=0x0000
# Initialize the interrupt vector
 mov r20, #0x230
 mov r0, r20
 ; setup the reset handler as an infinite loop
 mov [r0], r0
 ; setup the timer handler
 mov r1, #0x2080  ; timer handler
 add r0, #4
 mov [r0], r1
 ; setup the serial handler
 mov r1, #0x2100
 add r0, #4
 mov [r0], r0

# setup stack
 ; load stack pointer, stack grow downward.
 mov r15, #0x2000
# enable interrupts for serial and timer
 mov r0, r21
 or  r0, #0x02 ; enable TIMER
 mov r21, r0
# enable interrupts globally
 mov r0, r18
 xor r0, #0x04 ; enable interrupts
 mov r18, r0

# Initialize timer to 0x16, takes two 32bit writes
 mov r1, #0x1004  ; TIMER_WRITE
 mov r0, #0       ; high 32bits
 mov [r1], r0
 mov r0, #100     ; low 32bits
 mov [r1], r0

# loop forever here
 mov r0, r16    ; get PC for loop on halt
 halt
 bne r0, r0, #0  ; loop over indefinitely

#Timer interrupt handler
.=0x2080
	push r0
	push r1
	mov r1, #0x1004  ; TIMER_WRITE
 	mov r0, #0       ; high 32bits
 	mov [r1], r0
 	mov r0, #100     ; low 32bits
 	mov [r1], r0
	pop r1
	pop r0
	iret

#Timer interrupt handler
.=0x2100
	iret
