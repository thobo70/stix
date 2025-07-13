/**
 * @file io.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief I/O port access functions
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header provides low-level I/O port access functions for x86 architecture.
 */

#ifndef _IO_H
#define _IO_H

#include "../../../../include/tdefs.h"

/**
 * @brief Read a byte from an I/O port
 * 
 * @param port I/O port address
 * @return Byte value read from port
 */
static inline unsigned char inb(unsigned short port)
{
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "dN" (port));
    return result;
}

/**
 * @brief Write a byte to an I/O port
 * 
 * @param port I/O port address
 * @param value Byte value to write
 */
static inline void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

/**
 * @brief Read a word from an I/O port
 * 
 * @param port I/O port address
 * @return Word value read from port
 */
static inline unsigned short inw(unsigned short port)
{
    unsigned short result;
    __asm__ volatile ("inw %1, %0" : "=a" (result) : "dN" (port));
    return result;
}

/**
 * @brief Write a word to an I/O port
 * 
 * @param port I/O port address
 * @param value Word value to write
 */
static inline void outw(unsigned short port, unsigned short value)
{
    __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

/**
 * @brief Read a double word from an I/O port
 * 
 * @param port I/O port address
 * @return Double word value read from port
 */
static inline unsigned int inl(unsigned short port)
{
    unsigned int result;
    __asm__ volatile ("inl %1, %0" : "=a" (result) : "dN" (port));
    return result;
}

/**
 * @brief Write a double word to an I/O port
 * 
 * @param port I/O port address
 * @param value Double word value to write
 */
static inline void outl(unsigned short port, unsigned int value)
{
    __asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

/**
 * @brief I/O delay function
 * 
 * Provides a small delay by performing a dummy I/O operation.
 * Used for timing-sensitive I/O operations.
 */
static inline void io_wait(void)
{
    outb(0x80, 0); // Write to unused port for delay
}

#endif /* _IO_H */
