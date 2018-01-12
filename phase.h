/*  lpsk31.h
 *
 *  Header file for lpsk31
 */

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


#ifndef PHASE_H
#define PHASE_H  1

#include "common.h"

/* Phase detector parameter definitions */
#define PHASE_AVE_WINDOW          5 /* Length of phase averager window    */
#define SQUELCH_WINDOW           10 /* Length of squelch averager window  */

/* Arc tan correction table */
#define ARC_TAN_TABLE \
{ \
   0,  1,  3,  4,  5,  6,  8,  9, 10, 11, 13, 14, \
  15, 16, 17, 18, 20, 21, 22, 23, 24, 25, 26, 27, \
  28, 29, 30, 31, 32, 33, 34, 35, 35, 36, 37, 38, \
  39, 39, 40, 41, 42, 42, 43, 44, 44, 45          \
}

#endif

