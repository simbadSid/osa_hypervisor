# This program uses the timer to wait for 5 seconds
.=0x0000
# Input/Output addresses
mov r1, #0x1000  ; TIMER_READ
mov r2, #0x1004  ; TIMER_WRITE

# Initialize timer to 0x16, takes two 32bit writes
mov r0, #0
mov [r2], r0
mov r0, #16
mov [r2], r0

# loop: take the current PC as the loop address
mov r3, r16
# Read current time, ignore high b
mov r5, [r1]
mov r4, [r1]
# Read time while it is less than 5 seconds
# mov r4, 4117
bgt r3, r4, #0
halt
