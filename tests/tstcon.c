/**
 * @file tstcon.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "utils.h"
#include "dd.h"
#include "clist.h"


#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <signal.h>

static struct termios stored_settings;


void tstcon_open(ldevminor_t minor);
void tstcon_close(ldevminor_t minor);
void tstcon_read(ldevminor_t minor, byte_t cl);
void tstcon_write(ldevminor_t minor, byte_t cl);
void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg);

cdev_t tstcon = {
  NULL,
  tstcon_open,
  tstcon_close,
  tstcon_read,
  tstcon_write,
  tstcon_ioctl
};



void set_keypress(void)
{
    struct termios new_settings;
    tcgetattr(0,&stored_settings);
    new_settings = stored_settings;
    /* Disable canonical mode, and set buffer size to 1 byte */
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0,TCSANOW,&new_settings);
}



void reset_keypress(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
}



void tstcon_open(ldevminor_t minor)
{
  ASSERT(minor < 1);
  set_keypress();
}


void tstcon_close(ldevminor_t minor)
{
  ASSERT(minor < 1);
  reset_keypress();
}



void tstcon_read(ldevminor_t minor, byte_t cl)
{
  ASSERT(minor < 1);
  int c;
  while ((c = getchar()) != EOF)
    clist_push(cl, (char *)&c, 1);
}


void tstcon_write(ldevminor_t minor, byte_t cl)
{
  ASSERT(minor < 1);
  int c;
  while (clist_pop(cl, (char *)&c, 1) == 0)
    putchar(c);
  fflush(stdout);
}



void tstcon_ioctl(ldevminor_t minor, int cmd, void *arg)
{
  ASSERT(minor < 1);
  ASSERT(cmd);
  ASSERT(arg);
}


