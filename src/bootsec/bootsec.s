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
  call print
  # Set up the necessary parameters for the BIOS interrupt 0x13 to read sectors from disk
  mov $0x1000, %bx    # Destination buffer address
  mov $0x10, %ah      # Function number: Read sectors from disk
  mov $0x9, %al       # Number of sectors to read
  mov $0x2, %ch       # Cylinder number
  mov $0x1, %cl       # Sector number
  mov $0x0, %dh       # Head number
  int $0x13           # Call BIOS interrupt 0x13 to read sectors from disk
  jc disk_error
  mov $start_message, %si
  call print
  jmp far 0x0:$0x1000

disk_error:
  mov $error_message, %si
  call print
  jmp finish

print:
  push %ax
  push %bx
  push %cx
  push %dx
  push %si

  mov $0x0E, %ah       # Function number: Print character
print_loop:
  lodsb

  cmp $0, %al
  je print_done
  int $0x10
  jmp print_loop

print_done:
  pop %si
  pop %dx
  pop %cx
  pop %bx
  pop %ax
  ret

finish:
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
  .asciz "BootSec started ...\n"
start_message:
  .asciz "Start\n"
sd_str:
  .asciz "Shutdown"
error_message:
  .asciz "Disk read error!"

  # Magic number at the end of the boot sector
  .fill 510-(.-start), 1, 0
  .byte 0x55
  .byte 0xAA
