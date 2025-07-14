/**
 * @file con_cdev.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Console character device driver
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file implements the console character device driver that provides
 * a unified interface combining keyboard input, VGA screen output, and
 * VT52 terminal emulation.
 */

#include "include/con_cdev.h"
#include "include/scrnctl.h"
#include "include/vt52emu.h"
#include "include/keybrd.h"
#include "include/cdev.h"
#include "../../../../include/clist.h"
#include "../../../../include/tdefs.h"
#include "tdefs.h"

// Console device state
static struct console_state {
    byte_t input_queue_id;      ///< Input character queue ID from clist
    bool echo_enabled;          ///< Echo typed characters to screen
    bool raw_mode;              ///< Raw mode (no line buffering)
    bool initialized;           ///< Driver initialization state
    int cursor_x, cursor_y;     ///< Current cursor position cache
} console;

// Function prototypes for device operations
static int con_open(int unit);
static int con_close(int unit);
static int con_read(int unit, char *buffer, int count);
static int con_write(int unit, const char *buffer, int count);
static int con_ioctl(int unit, int cmd, void *arg);

// Console device descriptor
static struct cdev console_cdev = {
    .name = "console",
    .open = con_open,
    .close = con_close,
    .read = con_read,
    .write = con_write,
    .ioctl = con_ioctl
};

// Console control commands for ioctl
#define CON_IOCTL_SET_ECHO      0x1001  ///< Enable/disable echo
#define CON_IOCTL_SET_RAW       0x1002  ///< Enable/disable raw mode
#define CON_IOCTL_CLEAR_SCREEN  0x1003  ///< Clear screen
#define CON_IOCTL_SET_CURSOR    0x1004  ///< Set cursor position
#define CON_IOCTL_GET_CURSOR    0x1005  ///< Get cursor position
#define CON_IOCTL_SET_COLOR     0x1006  ///< Set text color

/**
 * @brief Initialize console character device driver
 * 
 * Initializes the console driver, VGA screen controller,
 * VT52 terminal emulator, and keyboard interface.
 * 
 * @return 0 on success, negative on error
 */
int con_cdev_init(void)
{
    // Initialize clist system first
    init_clist();
    
    // Initialize subsystems
    vga_init();
    vt52_init();
    
    // Initialize keyboard if available
    if (kbd_init() == 0) {
        // Set up keyboard interrupt handler
        kbd_set_handler(con_keyboard_handler);
    }
    
    // Initialize console state
    console.input_queue_id = clist_create();
    console.echo_enabled = true;
    console.raw_mode = false;
    console.initialized = true;
    console.cursor_x = 0;
    console.cursor_y = 0;
    
    // Register character device
    return cdev_register(&console_cdev, CON_MAJOR);
}

/**
 * @brief Keyboard interrupt handler
 * 
 * Called from keyboard interrupt to process typed characters.
 * Handles special keys and queues printable characters.
 * 
 * @param scancode Raw keyboard scancode
 */
void con_keyboard_handler(unsigned char scancode)
{
    char c;
    
    // Convert scancode to ASCII character
    c = kbd_scancode_to_ascii(scancode);
    
    if (c == 0) {
        // Handle special keys
        switch (scancode) {
            case 0x48: // Up arrow
                if (console.echo_enabled) {
                    vt52_process_char('\033');
                    vt52_process_char('A');
                }
                break;
            case 0x50: // Down arrow
                if (console.echo_enabled) {
                    vt52_process_char('\033');
                    vt52_process_char('B');
                }
                break;
            case 0x4D: // Right arrow
                if (console.echo_enabled) {
                    vt52_process_char('\033');
                    vt52_process_char('C');
                }
                break;
            case 0x4B: // Left arrow
                if (console.echo_enabled) {
                    vt52_process_char('\033');
                    vt52_process_char('D');
                }
                break;
        }
        return;
    }
    
    // Queue the character for reading
    clist_push(console.input_queue_id, &c, 1);
    
    // Echo character if enabled
    if (console.echo_enabled) {
        vt52_process_char(c);
        
        // Handle special characters
        if (c == '\b') {
            // Backspace - move cursor back and erase
            vt52_process_char(' ');
            vt52_process_char('\b');
        }
    }
}

/**
 * @brief Open console device
 * 
 * @param unit Device unit number (ignored for console)
 * @return 0 on success
 */
static int con_open(int unit)
{
    (void)unit; // Unused parameter
    
    if (!console.initialized) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Close console device
 * 
 * @param unit Device unit number (ignored for console)
 * @return 0 on success
 */
static int con_close(int unit)
{
    (void)unit; // Unused parameter
    return 0;
}

/**
 * @brief Read characters from console
 * 
 * Reads characters from the keyboard input queue. In line mode,
 * blocks until a complete line is available. In raw mode,
 * returns immediately with available characters.
 * 
 * @param unit Device unit number (ignored)
 * @param buffer Buffer to store read characters
 * @param count Maximum number of characters to read
 * @return Number of characters read, or negative on error
 */
static int con_read(int unit, char *buffer, int count)
{
    (void)unit; // Unused parameter
    int chars_read = 0;
    char c;
    
    if (!console.initialized || !buffer || count <= 0) {
        return -1;
    }
    
    while (chars_read < count) {
        // Try to get character from input queue
        if (clist_pop(console.input_queue_id, &c, 1) == 1) {
            buffer[chars_read++] = c;
            
            // In line mode, stop at newline
            if (!console.raw_mode && c == '\n') {
                break;
            }
        } else {
            // No characters available
            if (console.raw_mode || chars_read > 0) {
                break; // Return what we have
            }
            // In line mode with no characters, we would normally block
            // For this implementation, we'll return 0
            break;
        }
    }
    
    return chars_read;
}

/**
 * @brief Write characters to console
 * 
 * Writes characters to the VGA screen through the VT52 terminal
 * emulator, which processes escape sequences and control characters.
 * 
 * @param unit Device unit number (ignored)
 * @param buffer Buffer containing characters to write
 * @param count Number of characters to write
 * @return Number of characters written, or negative on error
 */
static int con_write(int unit, const char *buffer, int count)
{
    (void)unit; // Unused parameter
    int i;
    
    if (!console.initialized || !buffer || count <= 0) {
        return -1;
    }
    
    // Process each character through VT52 emulator
    for (i = 0; i < count; i++) {
        vt52_process_char(buffer[i]);
    }
    
    return count;
}

/**
 * @brief Console device I/O control
 * 
 * Handles various console control operations like setting echo mode,
 * raw mode, cursor position, and screen clearing.
 * 
 * @param unit Device unit number (ignored)
 * @param cmd Control command
 * @param arg Command argument (type depends on command)
 * @return 0 on success, negative on error
 */
static int con_ioctl(int unit, int cmd, void *arg)
{
    (void)unit; // Unused parameter
    
    if (!console.initialized) {
        return -1;
    }
    
    switch (cmd) {
        case CON_IOCTL_SET_ECHO:
            console.echo_enabled = (bool)(uintptr_t)arg;
            return 0;
            
        case CON_IOCTL_SET_RAW:
            console.raw_mode = (bool)(uintptr_t)arg;
            return 0;
            
        case CON_IOCTL_CLEAR_SCREEN:
            vt52_clear_screen();
            return 0;
            
        case CON_IOCTL_SET_CURSOR:
            if (arg) {
                struct cursor_pos *pos = (struct cursor_pos *)arg;
                vt52_set_cursor(pos->x, pos->y);
                console.cursor_x = pos->x;
                console.cursor_y = pos->y;
                return 0;
            }
            return -1;
            
        case CON_IOCTL_GET_CURSOR:
            if (arg) {
                struct cursor_pos *pos = (struct cursor_pos *)arg;
                vga_get_cursor(&pos->x, &pos->y);
                console.cursor_x = pos->x;
                console.cursor_y = pos->y;
                return 0;
            }
            return -1;
            
        case CON_IOCTL_SET_COLOR:
            {
                byte_t color = (byte_t)(uintptr_t)arg;
                vga_set_color(color);
                return 0;
            }
            
        default:
            return -1; // Unknown command
    }
}

/**
 * @brief Get console device descriptor
 * 
 * @return Pointer to console character device descriptor
 */
struct cdev *con_get_cdev(void)
{
    return &console_cdev;
}

/**
 * @brief Print formatted string to console
 * 
 * Convenience function for formatted output to the console.
 * This is a simple implementation without full printf support.
 * 
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of characters written
 */
int con_printf(const char *format, ...)
{
    // Simple implementation - just write the format string
    // A full implementation would need variable argument parsing
    int len = 0;
    const char *p = format;
    
    while (*p) {
        len++;
        p++;
    }
    
    return con_write(0, format, len);
}

/**
 * @brief Write single character to console
 * 
 * @param c Character to write
 */
void con_putchar(char c)
{
    con_write(0, &c, 1);
}

/**
 * @brief Read single character from console
 * 
 * @return Character read, or -1 if no character available
 */
int con_getchar(void)
{
    char c;
    int result = con_read(0, &c, 1);
    
    if (result == 1) {
        return (unsigned char)c;
    }
    
    return -1; // No character available
}

/**
 * @brief Set console mode
 * 
 * @param echo Enable echo mode
 * @param raw Enable raw mode
 */
void con_set_mode(bool echo, bool raw)
{
    console.echo_enabled = echo;
    console.raw_mode = raw;
}
