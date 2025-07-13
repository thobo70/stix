/**
 * @file keybrd.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Keyboard driver interface
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header defines the interface for the PS/2 keyboard driver
 * used by the console character device driver.
 */

#ifndef _KEYBRD_H
#define _KEYBRD_H

#include "../../../../include/tdefs.h"

// Keyboard I/O ports
#define KBD_DATA_PORT       0x60    ///< Keyboard data port
#define KBD_STATUS_PORT     0x64    ///< Keyboard status/command port
#define KBD_COMMAND_PORT    0x64    ///< Keyboard command port

// Keyboard status register bits
#define KBD_STATUS_OUTPUT_FULL  0x01    ///< Output buffer full
#define KBD_STATUS_INPUT_FULL   0x02    ///< Input buffer full

// Special scancodes
#define KBD_SCANCODE_EXTENDED   0xE0    ///< Extended scancode prefix
#define KBD_SCANCODE_RELEASE    0x80    ///< Key release bit

// Special key scancodes (make codes)
#define KBD_KEY_ESC         0x01
#define KBD_KEY_BACKSPACE   0x0E
#define KBD_KEY_TAB         0x0F
#define KBD_KEY_ENTER       0x1C
#define KBD_KEY_CTRL        0x1D
#define KBD_KEY_LSHIFT      0x2A
#define KBD_KEY_RSHIFT      0x36
#define KBD_KEY_ALT         0x38
#define KBD_KEY_SPACE       0x39
#define KBD_KEY_CAPSLOCK    0x3A

// Extended key scancodes
#define KBD_KEY_UP          0x48
#define KBD_KEY_DOWN        0x50
#define KBD_KEY_LEFT        0x4B
#define KBD_KEY_RIGHT       0x4D
#define KBD_KEY_HOME        0x47
#define KBD_KEY_END         0x4F
#define KBD_KEY_PGUP        0x49
#define KBD_KEY_PGDN        0x51
#define KBD_KEY_INSERT      0x52
#define KBD_KEY_DELETE      0x53

// Keyboard handler function type
typedef void (*kbd_handler_t)(unsigned char scancode);

/**
 * @brief Initialize keyboard driver
 * 
 * Initializes the PS/2 keyboard controller and sets up
 * interrupt handling.
 * 
 * @return 0 on success, negative on error
 */
int kbd_init(void);

/**
 * @brief Set keyboard interrupt handler
 * 
 * Sets the function to be called when keyboard interrupts occur.
 * 
 * @param handler Function to handle keyboard interrupts
 */
void kbd_set_handler(kbd_handler_t handler);

/**
 * @brief Convert scancode to ASCII character
 * 
 * Converts a keyboard scancode to its corresponding ASCII character,
 * taking into account shift state and special keys.
 * 
 * @param scancode Raw keyboard scancode
 * @return ASCII character, or 0 for non-printable keys
 */
char kbd_scancode_to_ascii(unsigned char scancode);

/**
 * @brief Check if a key is currently pressed
 * 
 * @param scancode Scancode to check
 * @return true if key is pressed, false otherwise
 */
bool kbd_is_key_pressed(unsigned char scancode);

/**
 * @brief Get current keyboard modifier state
 * 
 * @return Bitmask of currently active modifiers
 */
unsigned int kbd_get_modifiers(void);

// Modifier key bits
#define KBD_MOD_LSHIFT      0x01
#define KBD_MOD_RSHIFT      0x02
#define KBD_MOD_SHIFT       (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT)
#define KBD_MOD_CTRL        0x04
#define KBD_MOD_ALT         0x08
#define KBD_MOD_CAPSLOCK    0x10

#endif /* _KEYBRD_H */
