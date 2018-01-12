/* cat.h */

/*
 *  lpsk31: An application to transmit and receive
 *  PSK31 signals using a computer's sound card
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details:
 *
 *  http://www.gnu.org/copyleft/gpl.txt
 */


#ifndef CAT_H
#define CAT_H  1

#include "common.h"

/* Command codes for FT847 CAT */
unsigned char
  CAT_ON[]     = {    0, 0, 0, 0, 0x00 },
  CAT_OFF[]    = {    0, 0, 0, 0, 0x80 },
  PTT_ON[]     = {    0, 0, 0, 0, 0x08 },
  PTT_OFF[]    = {    0, 0, 0, 0, 0x88 },
  MODE_LSB[]   = {    0, 0, 0, 0, 0x07 },
  MODE_USB[]   = {    1, 0, 0, 0, 0x07 },
  MODE_CW[]    = {    2, 0, 0, 0, 0x07 },
  MODE_CWR[]   = {    3, 0, 0, 0, 0x07 },
  MODE_CWN[]   = { 0x82, 0, 0, 0, 0x07 },
  MODE_CWNR[]  = { 0x83, 0, 0, 0, 0x07 };

#endif

