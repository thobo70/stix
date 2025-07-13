/**
 * @file keybrd.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief PS/2 Keyboard driver implementation
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file implements a basic PS/2 keyboard driver for the STIX operating system.
 * It provides scancode to ASCII conversion and basic keyboard state tracking.
 */

#include "include/keybrd.h"
#include "include/io.h"

// Keyboard state
static struct kbd_state {
    kbd_handler_t handler;      ///< Interrupt handler function
    unsigned int modifiers;     ///< Current modifier key state
    bool key_state[256];        ///< State of each key (pressed/released)
    bool extended_mode;         ///< Processing extended scancode
    bool initialized;           ///< Driver initialization state
} kbd;

// US QWERTY keyboard layout (scancode to ASCII mapping)
static const char kbd_layout_normal[128] = {
    0,    0,   '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w', 'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
    'd',  'f', 'g',  'h',  'j',  'k',  'l',  ';',  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n', 'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,   0,    0,    0,    0,    0,    '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3', '0',  '.',  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

// Shifted characters
static const char kbd_layout_shift[128] = {
    0,    0,   '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W', 'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
    'D',  'F', 'G',  'H',  'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N', 'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,   0,    0,    0,    0,    0,    '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3', '0',  '.',  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

/**
 * @brief Initialize keyboard driver
 * 
 * Initializes the PS/2 keyboard controller and sets up
 * interrupt handling.
 * 
 * @return 0 on success, negative on error
 */
int kbd_init(void)
{
    // Clear keyboard state
    kbd.handler = NULL;
    kbd.modifiers = 0;
    kbd.extended_mode = false;
    kbd.initialized = false;
    
    // Clear key state array
    for (int i = 0; i < 256; i++) {
        kbd.key_state[i] = false;
    }
    
    // Wait for keyboard controller to be ready
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_INPUT_FULL) {
        // Wait for input buffer to be empty
    }
    
    // Enable keyboard (this is usually done by BIOS, but let's be safe)
    outb(KBD_COMMAND_PORT, 0xAE);  // Enable first PS/2 port
    
    // Flush any pending data
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_OUTPUT_FULL) {
        inb(KBD_DATA_PORT);
    }
    
    kbd.initialized = true;
    return 0;
}

/**
 * @brief Set keyboard interrupt handler
 * 
 * Sets the function to be called when keyboard interrupts occur.
 * 
 * @param handler Function to handle keyboard interrupts
 */
void kbd_set_handler(kbd_handler_t handler)
{
    kbd.handler = handler;
}

/**
 * @brief Convert scancode to ASCII character
 * 
 * Converts a keyboard scancode to its corresponding ASCII character,
 * taking into account shift state and special keys.
 * 
 * @param scancode Raw keyboard scancode
 * @return ASCII character, or 0 for non-printable keys
 */
char kbd_scancode_to_ascii(unsigned char scancode)
{
    char c;
    bool shift_pressed;
    
    if (scancode >= 128) {
        return 0;  // Invalid scancode
    }
    
    // Determine if shift is active
    shift_pressed = (kbd.modifiers & KBD_MOD_SHIFT) != 0;
    
    // For letters, consider caps lock
    c = shift_pressed ? kbd_layout_shift[scancode] : kbd_layout_normal[scancode];
    
    // Apply caps lock for letters
    if ((kbd.modifiers & KBD_MOD_CAPSLOCK) && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if ((kbd.modifiers & KBD_MOD_CAPSLOCK) && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    return c;
}

/**
 * @brief Check if a key is currently pressed
 * 
 * @param scancode Scancode to check
 * @return true if key is pressed, false otherwise
 */
bool kbd_is_key_pressed(unsigned char scancode)
{
    return kbd.key_state[scancode];
}

/**
 * @brief Get current keyboard modifier state
 * 
 * @return Bitmask of currently active modifiers
 */
unsigned int kbd_get_modifiers(void)
{
    return kbd.modifiers;
}