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


#ifndef SCREEN_H
#define SCREEN_H  1

#include "common.h"

/************ CUSTOMIZE THESE DEFINITIONS FOR YOUR NEEDS *************/

#define LABEL_COLOR      CYAN            /* Color of window labels   */
#define BORDER_COLOR     CYAN            /* Color of window boxes    */
#define KEYBD_COMMANDS   MAGENTA         /* Color of key commands    */
#define STANDOUT_COLOR   YELLOW          /* Standout (bold) color    */
#define FORM_COLOR       YELLOW_ON_BLUE  /* Color in QSO record form */

/* Definitions of some color pairs and color numbers */
#define WHITE   		COLOR_PAIR( COLOR_WHITE )
#define RED				COLOR_PAIR( COLOR_RED )
#define GREEN			COLOR_PAIR( COLOR_GREEN )
#define BLUE			COLOR_PAIR( COLOR_BLUE )
#define CYAN			COLOR_PAIR( COLOR_CYAN )
#define MAGENTA			COLOR_PAIR( COLOR_MAGENTA )
#define YELLOW			COLOR_PAIR( COLOR_YELLOW ) | A_BOLD
#define WHITE_ON_BLUE  	COLOR_PAIR( COLOR_WHITE_BLUE )
#define YELLOW_ON_BLUE	COLOR_PAIR( COLOR_YELLOW_BLUE ) | A_BOLD
#define RED_ON_BLUE   	COLOR_PAIR( COLOR_WHITE_RED )   | A_BOLD
#define WHITE_ON_YELLOW	COLOR_PAIR( COLOR_WHITE_YELLOW )| A_BOLD
#define WHITE_ON_GREEN 	COLOR_PAIR( COLOR_WHITE_GREEN ) | A_BOLD

/***************** END OF CUSTOMIZABLE SECTION ***********************/

/* Number of recognizable tags */
#define NUM_OF_TAGS       13

/* Table of tag strings. !!MAX LENGTH 12 char!! */
#define TAG_TABLE \
{   \
  "own-call", "own-name" , "own-qth" ,\
  "own-loc" , "own-rst"  , "rem-call",\
  "rem-name", "rem-qth"  , "rem-loc" ,\
  "rem-rst" , "date-time", "op-freq" ,\
  "app-version" \
}

/* QSO record type definition */
typedef struct
{
  char
	call[15],     /* Incoming station call-sign   */
	rst_in[4],    /* Incoming station RST report  */
	rst_out[4],   /* Outgoing station RST report  */
	name[12],     /* Remote station operator name */
	qth[14],      /* Remote station QTH loc. name */
	loc[7],       /* Remote station QTH locator   */
	freq[5],      /* QSO operating frequency MHz  */
	date[12],     /* Calendar QSO date */
	time[9],      /* Calendar QSO time */
	date_adif[9], /* QSO date in ADIF format */
	time_adif[5], /* QSO time in ADIF format */
	mode[5];      /* Mode: BPSK/QPSK-USB/LSB */

} qso_record_t;

/* Table for plotting carrier phase */
#define PLOT_TABLE \
{ \
  0,  8,  1,  7,  2,  6,  3,  3, \
  3,  0,  3, -3,  2, -6,  1, -7, \
  0, -8, -1, -7, -2, -6, -3, -3, \
 -3,  0, -3,  3, -2,  6, -1,  7, \
  0,  8                          \
}

#endif

