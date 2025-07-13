/**
 * @file bootsec.s
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief boot sector
 * @version 0.1
 * @date 2024-01-08
 * 
 * @copyright Copyright (c) 2024
 * 
 * This assembly code implements a BIOS boot sector for an operating system.
 * It initializes the stack, serial port, and retrieves BIOS disk geometry.
 * The boot sector prints messages to both the screen and COM1, loads the second sector from disk with retries and error handling,
 * and reports the result. It also provides routines for serial output, decimal printing, and BIOS parameter display.
 * On completion, it signals Bochs to shut down via a special port.
 * The boot sector ends with the 0x55AA signature required by the BIOS.
 */
.code16
.section .text
.globl start

start:
  cli
  cld
  mov %dl, bootdev         # Save boot device from DL register

  # Initialize stack: set SS:SP to 0x0000:0x7C00 (just below bootloader)
  xor %ax, %ax
  mov %ax, %ss
  mov $0x7C00, %sp

  call init_serial

  call init_bdparam        # Initialize BIOS boot drive parameters
  call pr_bdparam          # Print BIOS drive parameters

  mov $message, %si
  call puts

  xor %ax, %ax
  mov %ax, %es
  mov $0x7E00, %bx        # ES:BX = 0x7E00 (buffer for loading sector)
  mov $0x00, %ch          # CH = 0 (cylinder)
  mov $0x00, %dh          # DH = 0 (head)
  mov $0x02, %cl          # CL = 2 (sector number, 1-based)
  mov bootdev, %dl       # DL = boot device (drive number)
  call load_sector_retry   # Load sector 2 from boot device into ES:BX
  jnc good1                # If CF not set, jump to good1 (no error)
  mov $'!', %al
  jmp done1
good1:
  mov $'.', %al
done1:
  call putc                # Print result character

  mov $m_loaded, %si
  call puts                # Print "OK"

  mov $0x7E00, %si        # Set SI to 0x7E00 (start of 2nd boot sector)
  call puts
  
finish:
  call serwait           # flush serial port
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

error:                # Error handling %si points to error message
  call puts
  jmp finish

# --- Subroutines at the end ---

# putc: outputs character in AL to screen and COM1, no registers preserved
putc:
  push %ax
  push %bx
  push %cx
  push %dx
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

  pop %dx
  pop %cx
  pop %bx
  pop %ax
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

init_bdparam:
  # Read BIOS parameters from the boot device
  mov $0x08, %ah         # AH = 0x08 (get drive parameters)
  mov bootdev, %dl       # DL = drive number (boot device)
  xor %bx, %bx           # BX = 0 (no buffer)
  mov %bx, %es           # ES = 0 (no buffer)
  mov %bx, %di           # DI = 0 (no offset)
  int $0x13
  mov $e_bdparam, %si    # SI points to error message
  jc error

  # Cylinder = ((CL >> 6) << 8) | CH
  movzbw %ch, %ax        # AX = CH (low 8 bits of cylinder)
  movzbw %cl, %bx        # BX = CL
  mov %bx, %dx
  shr $6, %dx            # DX = high 2 bits of cylinder
  shl $8, %dx
  or %dx, %ax            # AX = full cylinder number
  mov %ax, bdcyls        # Store cylinder number in bdcyls

  # Sectors per track = CL & 0x3F
  and $0x3F, %bl         # BL = sectors per track
  mov %bl, bdsecs        # Store sectors per track in bdsecs

  # Number of heads = DH + 1
  movzbw %dh, %cx
  inc %cx                # CX = number of heads
  mov %cx, bdheads       # Store number of heads in bdheads

  ret
  
pr_bdparam:
  # Print BIOS drive parameters
  mov $'D', %al
  call putc
  movzbw bootdev, %ax    # Load boot device
  call putnum            # Print boot device
  mov $'C', %al
  call putc
  mov bdcyls, %ax        # Load cylinder count
  call putnum            # Print cylinder count
  mov $'H', %al
  call putc
  mov bdheads, %ax       # Load head count
  call putnum            # Print head count
  mov $'S', %al
  call putc
  mov bdsecs, %ax        # Load sector count
  call putnum            # Print sector count
  ret

# load_sector_retry:
# Loads one sector from the boot device using BIOS INT 13h with up to 2 retries.
# If an error occurs, performs a disk reset (INT 13h AH=0) before retrying.
# Inputs:
#   DL = drive number (bootdev)
#   ES:BX = buffer to load sector into
#   CH = cylinder (track) number (0-based)
#   DH = head number (0-based)
#   CL = sector number (1-based)
#   Loads 1 sector.
# Returns:
#   CF clear on success, set on error

load_sector_retry:
  push %ax
  push %bx
  push %cx
  push %dx
  push %es
  push %di

  mov $3, %di           # retry counter (3 attempts: 1 initial + 2 retries)

.retryread:
  mov $0x0201, %ax      # AH=0x02 (read), AL=1 sector
  int $0x13             # BIOS disk service
  jnc .success          # jump if CF not set (success)

  # On error, reset disk system
  mov $0x0000, %ax      # AH=0x00 (reset disk system)
  int $0x13

  # Short delay after reset (simple loop, adjust count as needed)
  push %cx
  mov $0xFFFF, %cx
.delay:
  loop .delay
  pop %cx

  dec %di
  jnz .retryread            # retry if attempts left

  stc                   # set CF to indicate failure
  jmp .doneread

.success:
  clc                   # clear CF to indicate success

.doneread:
  pop %di
  pop %es
  pop %dx
  pop %cx
  pop %bx
  pop %ax
  ret

# putnum: prints the unsigned number in AX as decimal using putc
putnum:
  push %ax
  push %bx
  push %cx
  push %dx

  mov $10, %bx        # Divisor for decimal
  xor %cx, %cx        # CX will count digits

.convert:
  # Convert number to decimal digits (in reverse order, store in stack)
.repeat:
  xor %dx, %dx
  div %bx             # AX = AX / 10, DX = AX % 10
  push %dx            # Save remainder (digit)
  inc %cx             # Count digits
  test %ax, %ax
  jnz .repeat

.print_digits:
  pop %dx
  add $'0', %dl
  mov %dl, %al
  call putc
  loop .print_digits

  pop %dx
  pop %cx
  pop %bx
  pop %ax
  ret

bootdev:
  .byte 0x00
bdcyls:
  .word 0x0000          # Number of cylinders
bdheads:
  .word 0x0000          # Number of heads
bdsecs:
  .word 0x0000          # Number of sectors

message:
  .asciz "\nload "
m_loaded:
  .asciz "\nOK\n"
e_bdparam:
  .asciz "bdparam!"

  # Magic number at the end of the boot sector
  .fill 508-(.-start), 1, 0
secnum:
  .word 0x0002  # 2nd sector to be loaded at 0x7E00
  .byte 0x55
  .byte 0xAA
