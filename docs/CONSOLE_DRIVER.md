# Console Driver Implementation

## Overview

The STIX operating system includes a comprehensive console driver that provides:

- **VGA Text Mode Output**: 80x25 character display with 16-color support
- **VT52 Terminal Emulation**: Industry-standard terminal protocol with ANSI extensions
- **PS/2 Keyboard Input**: Hardware keyboard driver with scancode conversion
- **Character Device Interface**: Unified console device driver framework

## Architecture

The console driver consists of several modular components:

### VGA Screen Controller (`src/arch/x86/i386/con/scrnctl.c`)
- Direct VGA memory access (0xB8000)
- Character and color attribute control
- Cursor positioning and screen manipulation
- 16 foreground and 16 background colors

### VT52 Terminal Emulator (`src/arch/x86/i386/con/vt52emu.c`)
- Escape sequence processing
- Cursor movement commands
- Screen clearing and line manipulation
- ANSI color extension support

### PS/2 Keyboard Driver (`src/arch/x86/i386/con/keybrd.c`)
- Hardware keyboard controller interface
- Scancode to ASCII conversion
- Modifier key state tracking (Shift, Ctrl, Alt, Caps Lock)
- US QWERTY layout support

### Console Character Device (`src/arch/x86/i386/con/con_cdev.c`)
- Unified character device interface
- Input/output buffering using clist
- Echo mode and raw mode support
- Device control (ioctl) operations

### Character Device Framework (`src/dd/cdev.c`)
- Generic character device registration
- Device major/minor number management
- Standard device operations (open, close, read, write, ioctl)

## Files

```
src/arch/x86/i386/con/
├── scrnctl.c           # VGA screen controller implementation
├── vt52emu.c           # VT52 terminal emulator
├── keybrd.c            # PS/2 keyboard driver
└── con_cdev.c          # Console character device driver

src/include/
├── scrnctl.h           # VGA screen controller interface
├── vt52emu.h           # VT52 terminal emulator interface
├── keybrd.h            # Keyboard driver interface
├── con_cdev.h          # Console device interface
├── cdev.h              # Character device framework
└── io.h                # I/O port access functions

src/dd/
└── cdev.c              # Character device framework implementation
```

## Features

### VGA Text Mode Support
- 80x25 character display
- 16 foreground colors (black, blue, green, cyan, red, magenta, brown, light grey, dark grey, light blue, light green, light cyan, light red, light magenta, light brown, white)
- 16 background colors
- Hardware cursor control
- Screen scrolling
- Character and attribute manipulation

### VT52 Terminal Emulation
- **Cursor Movement**: Home, up, down, left, right
- **Screen Control**: Clear screen, clear to end of line, clear to end of screen
- **Line Operations**: Insert line, delete line
- **Cursor Positioning**: Direct cursor addressing
- **ANSI Extensions**: Color control, enhanced functionality

### Keyboard Support
- PS/2 keyboard controller interface
- Scancode processing and ASCII conversion
- Modifier key support (Shift, Ctrl, Alt, Caps Lock)
- Special key handling (arrows, function keys, etc.)
- Key press/release state tracking

### Character Device Interface
- Standard Unix-like device operations
- Input buffering and queuing
- Echo mode for interactive use
- Raw mode for direct character access
- Device control commands

## Usage Example

```c
#include "con_cdev.h"

int main(void) {
    // Initialize console driver
    con_cdev_init();
    
    // Clear screen and set colors
    vt52_clear_screen();
    vga_set_color(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4));
    
    // Write text with VT52 emulation
    con_printf("Hello, STIX Console!\n");
    
    // Position cursor and write
    vt52_set_cursor(10, 5);
    con_printf("Text at position (10,5)");
    
    // Read user input
    char buffer[256];
    int count = con_read(0, buffer, sizeof(buffer));
    
    return 0;
}
```

## Hardware Requirements

The console driver requires:
- x86 compatible processor
- VGA-compatible graphics adapter
- PS/2 keyboard controller
- Kernel-level execution (ring 0)
- Hardware interrupt support

## Important Note

**The console driver components are NOT included in the test builds** because they require direct hardware access that is not available in user-space programs. The driver components contain:

- Direct memory access to VGA buffer (0xB8000)
- I/O port operations for keyboard and cursor control
- Hardware interrupt handling

These operations will cause segmentation faults when run in user-space. The console driver is designed to run in kernel mode as part of the full STIX operating system.

## Testing

The console driver components can be tested in:
- Kernel mode execution
- Hardware emulators (like Bochs)
- Real hardware boot
- Dedicated kernel testing framework

For user-space testing, only the high-level interfaces and logic can be tested with mock implementations of the hardware access functions.

## Integration

The console driver integrates with:
- Character device subsystem (`cdev`)
- Character list management (`clist`)
- Interrupt handling system
- Memory management
- Process management (for blocking I/O)

The driver is designed to be the primary console interface for the STIX operating system, providing both input and output capabilities for user interaction.
