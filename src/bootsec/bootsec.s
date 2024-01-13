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
  mov $message, %si
  mov $0xe, %ah

print_msg:
  lodsb
  cmp $0, %al
  je finish
  int $0x10
  jmp print_msg

finish:
  cli
  # send shutdown
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

message:
  .asciz "BootSec started ..."

  # Magic number at the end of the boot sector
  .fill 510-(.-start), 1, 0
  .byte 0x55
  .byte 0xAA
