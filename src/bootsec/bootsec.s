/**
 * @file bootsec.s
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief boot sector
 * @version 0.1
 * @date 2024-01-08
 * 
 * @copyright Copyright (c) 2024
 * 
 * This assembly code represents the boot sector of an operating system.
 * It contains the necessary code to print a message on the screen and halt the system.
 * The boot sector starts at the 'start' label and jumps to the 'main' label.
 * The 'main' label initializes the source index register (SI) with the address of the message string
 * and the AH register with the value 0xE, which represents the video mode.
 * The 'print_msg' label uses the LODSB instruction to load a byte from the memory location pointed by SI
 * and compares it with 0. If the byte is zero, it jumps to the 'finish' label, otherwise it calls the BIOS interrupt 0x10
 * to print the character on the screen and jumps back to 'print_msg' to continue printing the message.
 * The 'finish' label disables interrupts, halts the system, and jumps back to 'finish' to create an infinite loop.
 * The 'message' label contains the message string "BootSec started ..." which will be printed on the screen.
 * The boot sector ends with a magic number (0x55AA) at the last two bytes to indicate that it is a valid boot sector.
 */
.code16
.section .text
.globl start

start:
  jmp main

main:
  mov %dl, bootdev         # Save boot device from DL register
  cld

  # Initialize stack: set SS:SP to 0x0000:0x7C00 (just below bootloader)
  xor %ax, %ax
  mov %ax, %ss
  mov $0x7C00, %sp

  call init_serial

  mov $message, %si
  call puts

  call serwait           # Wait for serial port to be ready
  
finish:
  cli

  # Send "Shutdown" to port 0x8900, exit bochs (see in bochs unmapped.cc[237])
  mov $0x8900, %dx
  movb $'S', %al
  outb %al, %dx
  movb $'h', %al
  outb %al, %dx
  movb $'u', %al
  outb %al, %dx
  movb $'t', %al
  outb %al, %dx
  movb $'d', %al
  outb %al, %dx
  movb $'o', %al
  outb %al, %dx
  movb $'w', %al
  outb %al, %dx
  movb $'n', %al
  outb %al, %dx

  # send shutdown https://wiki.osdev.org/Shutdown (but seems not to work for bochs)
  mov $0x2000, %dx
  mov $0xB004, %ax
  outw %ax, %dx        # for bochs and QEMU <2.0
  mov $0x0604, %dx
  outw %ax, %dx        # for QEMU
  mov $0x4004, %ax
  mov $0x3400, %dx
  outw %ax, %dx        # for virtual box
  hlt
  jmp finish

# --- Subroutines at the end ---

# putc: outputs character in AL to screen and COM1, no registers preserved
putc:
  mov %al, %cl           # Save char in CL for COM1 after BIOS call

  # Print to screen (BIOS teletype)
  mov $0xe, %ah
  xor %bx, %bx
  int $0x10

  # Print to COM1
  mov $0x3FD, %dx        # Line Status Register (base+5)
.wait_transmit:
  inb %dx, %al
  test $0x20, %al
  jz .wait_transmit

  mov %cl, %al           # Restore char to AL
  mov $0x3F8, %dx
  outb %al, %dx

  ret

# puts: outputs zero-terminated string at SI using putc
puts:
  lodsb
  cmp $0, %al
  je .done
  call putc
  jmp puts
.done:
  ret

serwait:
  # Wait for the fifo of serial port to be empty
  mov $0x3FD, %dx        # Line Status Register (base+5)
.wait_fifo_empty:
  inb %dx, %al
  test $0x40, %al
  jz .wait_fifo_empty
  ret

init_serial:
  # Disable interrupts
  mov $0x3F9, %dx
  mov $0x00, %al
  outb %al, %dx

  # Set DLAB=1 to set baud rate divisor
  mov $0x3FB, %dx
  mov $0x80, %al
  outb %al, %dx

  # Set divisor to 12 (0x0C) for 9600 baud (1.8432 MHz / (16*12))
  mov $0x3F8, %dx
  mov $0x0C, %al
  outb %al, %dx
  mov $0x3F9, %dx
  mov $0x00, %al
  outb %al, %dx

  # Set DLAB=0, 8 bits, no parity, one stop bit
  mov $0x3FB, %dx
  mov $0x03, %al
  outb %al, %dx

  # Enable FIFO, clear them, 14-byte threshold
  mov $0x3FA, %dx
  mov $0xC7, %al
  outb %al, %dx

  # Mark data terminal ready, signal request to send, enable auxiliary output 2
  mov $0x3FC, %dx
  mov $0x0B, %al
  outb %al, %dx

  ret

bootdev:
  .byte 0x00

message:
  .asciz "BootSec started ...\n"

  # Magic number at the end of the boot sector
  .fill 510-(.-start), 1, 0
  .byte 0x55
  .byte 0xAA
