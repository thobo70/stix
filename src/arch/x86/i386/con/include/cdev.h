/**
 * @file cdev.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Character device framework
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header defines the character device framework interface for
 * device drivers in the STIX operating system.
 */

#ifndef _CDEV_H
#define _CDEV_H

#include "../../../../include/tdefs.h"

// Forward declaration
struct cdev;

/**
 * @brief Character device operations structure
 * 
 * This structure contains function pointers for the various operations
 * that can be performed on a character device.
 */
struct cdev {
    const char *name;                           ///< Device name
    int (*open)(int unit);                      ///< Open device
    int (*close)(int unit);                     ///< Close device
    int (*read)(int unit, char *buffer, int count);     ///< Read from device
    int (*write)(int unit, const char *buffer, int count);   ///< Write to device
    int (*ioctl)(int unit, int cmd, void *arg); ///< Device control
};

/**
 * @brief Register a character device
 * 
 * Registers a character device with the system, making it available
 * for use by applications.
 * 
 * @param cdev Pointer to character device structure
 * @param major Major device number
 * @return 0 on success, negative on error
 */
int cdev_register(struct cdev *cdev, int major);

/**
 * @brief Unregister a character device
 * 
 * Removes a character device from the system.
 * 
 * @param major Major device number
 * @return 0 on success, negative on error
 */
int cdev_unregister(int major);

/**
 * @brief Find character device by major number
 * 
 * @param major Major device number
 * @return Pointer to character device structure, or NULL if not found
 */
struct cdev *cdev_find(int major);

/**
 * @brief Open a character device
 * 
 * @param major Major device number
 * @param minor Minor device number (unit)
 * @return 0 on success, negative on error
 */
int cdev_open(int major, int minor);

/**
 * @brief Close a character device
 * 
 * @param major Major device number
 * @param minor Minor device number (unit)
 * @return 0 on success, negative on error
 */
int cdev_close(int major, int minor);

/**
 * @brief Read from a character device
 * 
 * @param major Major device number
 * @param minor Minor device number (unit)
 * @param buffer Buffer to store read data
 * @param count Maximum number of bytes to read
 * @return Number of bytes read, or negative on error
 */
int cdev_read(int major, int minor, char *buffer, int count);

/**
 * @brief Write to a character device
 * 
 * @param major Major device number
 * @param minor Minor device number (unit)
 * @param buffer Buffer containing data to write
 * @param count Number of bytes to write
 * @return Number of bytes written, or negative on error
 */
int cdev_write(int major, int minor, const char *buffer, int count);

/**
 * @brief Control a character device
 * 
 * @param major Major device number
 * @param minor Minor device number (unit)
 * @param cmd Control command
 * @param arg Command argument
 * @return 0 on success, negative on error
 */
int cdev_ioctl(int major, int minor, int cmd, void *arg);

// Common device major numbers
#define NULL_MAJOR      0   ///< Null device
#define CON_MAJOR       1   ///< Console device
#define SERIAL_MAJOR    2   ///< Serial port device
#define MEM_MAJOR       3   ///< Memory device

#endif /* _CDEV_H */
