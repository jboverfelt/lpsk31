/* rig.h
 *
 *  Rig control functions for lpsk31, built on the Hamlib library.
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

#ifndef RIG_H
#define RIG_H 1

#include "common.h"

/* Rig control (Hamlib-backed) */
int rig_connect(void);
void rig_disconnect(void);
int rig_is_open(void);
void rig_set_mode_and_ptt(void);
int rig_get_status(char* freq, char* rst);
const char* rig_last_error(void);

#endif
