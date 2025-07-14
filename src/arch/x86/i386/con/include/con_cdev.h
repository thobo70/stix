/**
 * @file con_cdev.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Console character device driver interface
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header defines the interface for the console character device driver
 * that provides unified keyboard input, VGA screen output, and VT52 terminal
 * emulation functionality.
 */

#ifndef _CON_CDEV_H
#define _CON_CDEV_H

#include "../../../../include/tdefs.h"
#include "cdev.h"

// Console device major number
#define CON_MAJOR   1

// Console cursor position structure
struct cursor_pos {
    int x;  ///< Column position (0-based)
    int y;  ///< Row position (0-based)
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
int con_cdev_init(void);

/**
 * @brief Keyboard interrupt handler
 * 
 * Called from keyboard interrupt to process typed characters.
 * Handles special keys and queues printable characters.
 * 
 * @param scancode Raw keyboard scancode
 */
void con_keyboard_handler(unsigned char scancode);

/**
 * @brief Get console device descriptor
 * 
 * @return Pointer to console character device descriptor
 */
struct cdev *con_get_cdev(void);

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
int con_printf(const char *format, ...);

/**
 * @brief Write single character to console
 * 
 * @param c Character to write
 */
void con_putchar(char c);

/**
 * @brief Read single character from console
 * 
 * @return Character read, or -1 if no character available
 */
int con_getchar(void);

/**
 * @brief Set console mode
 * 
 * @param echo Enable echo mode
 * @param raw Enable raw mode
 */
void con_set_mode(bool echo, bool raw);

// Convenience macros for common operations
#define CON_CLEAR()             con_ioctl(0, CON_IOCTL_CLEAR_SCREEN, NULL)
#define CON_SET_CURSOR(x, y)    do { \
                                    struct cursor_pos pos = {x, y}; \
                                    con_ioctl(0, CON_IOCTL_SET_CURSOR, &pos); \
                                } while(0)
#define CON_SET_ECHO(enable)    con_ioctl(0, CON_IOCTL_SET_ECHO, (void*)(uintptr_t)(enable))
#define CON_SET_RAW(enable)     con_ioctl(0, CON_IOCTL_SET_RAW, (void*)(uintptr_t)(enable))

#endif /* _CON_CDEV_H */
