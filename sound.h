/*
 *  psk31: An application to transmit and receive
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

#ifndef SOUND_H
#define SOUND_H 1

#include "common.h"

#define PERIOD_SIZE 1024 /* PCM period size */
#define NUM_PERIODS 2    /* Number of periods */
#define SND_PCM_ACCESS SND_PCM_ACCESS_RW_INTERLEAVED
#define SND_PCM_FORMAT SND_PCM_FORMAT_S16_LE
#define PCM_BLOCK 0
#define EXACT_VAL 0

#endif
