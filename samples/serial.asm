# This small program tests the serial line, with interrupts.
# So you should make sure that you have tested your interrupts.
# The program is a simple echo program, reading characters from
# the serial line RX and echoing then back out on TX.
#
# Note: use the provided sockcon program to have a serial-line console
#       that should connect to your emulation through a unix socket domain.

.=0x0000
# Initialize the interrupt vector
 mov r20, #0x200
 ; setup reset handler
 mov r0, r20
 mov [r0], r0
 ; setup timer handler (inifinite loop0
 add r0, #4
 mov r1, r0
 mov [r0], r1
 ; setup serial handler
 mov r1, #0x280  ; serial handler
 add r0, #4
 mov [r0], r1  ; SERIAL

# setup stack
 mov r15, #0x2000 ; load stack pointer, stack grow downward.
# enable interrupts for serial and timer
 mov r0, r21
 xor  r0, #0x2 ; disable TIMER
 or  r0, #0x4  ; enable SERIAL
 mov r21, r0
# enable interrupts globally
 mov r0, r18
 xor  r0, #0x04 ; enable interrupts
 mov r18, r0

 mov r0, r16    ; get PC for loop
 halt
 brl r0  ; loop over indefinitely

# Interrupt vector
.=0x200

# Serial interrupt handler
.=0x280
  push r0
  push r1
  push r2
  mov r1, #0x1018  ; load serial STATUS at 0x18
  # loop until we have room to echo...
  mov r2, r16    ; get PC for loop
  mov  r0, [r1]  ; load Serial STATUS
  and  r0, #0x02 ; and with available space on TX
  beq r2, r0, #0 ; no room available -> loop

  # read one character from the serial line
  mov r1, #0x1010  ; serial RX at 0x10
  mov  r0, [r1]  ; load Serial RX
  # echo the character back out
  mov r1, #0x1014  ; serial TX at 0x14
  mov [r1], r0   ; store Serial TX

  pop r2
  pop r1
  pop r0
  iret

