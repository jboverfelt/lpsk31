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


#ifndef COMMON_H
#define COMMON_H  1

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <form.h>
#include <ncurses.h>
#include <termios.h>
#include <string.h>
#include <time.h>
#include <alsa/asoundlib.h>

/* Some color definitions */
#define COLOR_YELLOW_BLUE        8  /* Yellow fore, blue background   */
#define COLOR_WHITE_BLUE         9  /*  White fore, blue background   */
#define COLOR_WHITE_RED         10  /*  White fore, red background    */
#define COLOR_WHITE_YELLOW      11  /*  White fore, yellow background */
#define COLOR_WHITE_GREEN       12  /*  White fore, green background  */

/* General definitions for signal processing */
#define AUDIO_FREQUENCY			500 /* Revceiver BFO frequency Hz */
#define DSP_RATE			  48000 /* DSP sampling rate */
#define BAUD_RATE			  31.25 /* DSP sampling rate */

/* Control flags */
#define NO_SQUELCH           0x000001 /* Suppress squelch */
#define SQUELCH_OPEN         0x000002 /* Squelch is open  */
#define MODE_BPSK            0x000010 /* Mode is BPSK     */
#define MODE_QPSK_USB        0x000020 /* Mode is QPSK-USB */
#define MODE_QPSK_LSB        0x000040 /* Mode is QPSK-LSB */
#define ENABLE_CAT           0x000080 /* Enable Trcvr CAT */
#define AUTO_QSO_DATA        0x000100 /* Auto collection of qso data */
#define RECORD_QSO           0x000200 /* Enable recording of QSO's   */
#define NCURSES_INIT         0x000400 /* Curses inteface initialized */
#define TRANSMIT_MACRO       0x000800 /* Start transmission of Macro */
#define STAY_IN_TX_MODE      0x001000 /* Remain in Transmisiion mode */
#define CAPITAL_LETTERS      0x002000 /* Capitalize letters in Trans */
#define CHANNEL_A_SELECT     0x004000 /* Detector channel A selected */
#define MODE_TRANSMIT        0x008000 /* lpsk31 is in Transmit mode  */
#define RECORD_EDITED        0x010000 /* Some data entered in record */
#define RECORD_SAVED         0x020000 /* Record was saved to file    */


/* Some character definitions */
#define CR                     0x0d /* Carriage Return */
#define LF                     0x0a /* Line Leed       */
#define BS                     0x08 /* Back Space      */
#define KEY_RET                0x0d /* Keycode of RETURN  */
#define KEY_ESC                0x1b /* Keycode of ESCAPE  */
#define KEY_CTRL_W             0x17 /* Keycode of Ctrl-W  */
#define KEY_TAB                0x09 /* Keycode of TAB     */
#define KEY_SPACE              0x20 /* Keycode of space   */
#define MACRO_KEY              0xff /* Function key F1-F9 */
#define NO_CHARACTER           0x80 /* No psk31 character decoded yet */

/* 'NUL' character used as 'last character' flag */
#define NULL_CHAR          0x00

#define ERROR	-1
#define SUCCESS	 0

/* lpsk31 version string */
#define VERSION  "lpsk31 1.2"

/*-------------------------------------------------------------------*/

/* Configuration parameters structure */
typedef struct
{
  char
	snd_card[32], /* Sound card name */
	cap_src[32],  /* Capture source  */
	cap_vol[32],  /* Capture volume control  */
	mst_vol[32],  /* Master volume control   */
	pcm_vol[32];  /* Playback volume control */

  int
	channel,	/* ALSA "channel" for use in mixer setup */
	num_chn,	/* Number of audio channels (2=stereo, 1=mono) */
	use_chn,	/* Channel in use (frontleft=0, frontright=1 etc) */
	cap_lev,	/* Recording/Capture level */
	pbk_lev;	/* Playback (volume) level */

  char
	mode[7],     /* Mono | Stereo   */
	serial[15];  /* CAT serial port */

  int
	tone_period,
	psk_elem,
	psk_elem2,
	sqlch_thr,  /* Squelch threshold    */
	tone_dur,   /* Preamble tone duration   */
	num_rev,    /* Pre/post-amble reversals */
	wpm,        /* Morse encoder speed: wpm */
	wwrap;      /* Word wrap column number  */

} rc_data_t;

/* Operator and location data */
typedef struct
{
  char
	call[17],
	name[17],
	qth[21],
	loc[7];

} op_data_t;

/*-------------------------------------------------------------------*/

/* Function prototypes produced by cproto */
/* cat.c */
void Read_Rx_Status(char *mode, char *freq, char *rst);
void Open_Tcvr_Serial(void);
int Write_Tcvr_Command(unsigned char *cmnd);
void Set_Tcvr_Mode(void);
int Read_Tcvr_Serial(unsigned char *cmnd, int block_size);
void Close_Tcvr_Serial(void);
/* main.c */
int main(int argc, char *argv[]);
void Load_Config(void);
int Load_Line(char *buff, FILE *pfile);
void Usage(void);
void Cleanup(void);
void Abort_On_Error(int why);
int isFlagSet(int flag);
int isFlagClear(int flag);
void Set_Flag(int flag);
void Clear_Flag(int flag);
void Toggle_Flag(int flag);
/* phase.c */
int PSK31_Phase(void);
int Arc_Tan(int y, int x);
int Scalar_Mag(int x, int y);
/* pskcodec.c */
int Varicode_Bit(void);
int Decode_PSK_Character(void);
void Transmit_Preamble(void);
void Transmit_Postamble(void);
void Transmit_Element(int phase_shift);
void Transmit_PSK_Character(int ascii_chr);
int Viterbi_Decoder(int phase_chg);
int Distance(int A, int B);
unsigned char Parity(unsigned char data);
void Init_Viterbi(void);
void Morse_Transmit(char *mesg);
void Make_Cosine_Table(void);
void Make_Sine_Table(void);
/* screen.c */
void Screen_Root(void);
void Screen_Transmit_Window(void);
int Transmit_Macro(WINDOW *transmit_win);
void Transmit_Character(int chr, WINDOW *transmit_win);
void Screen_Receive_Window(void);
void Screen_Phase_Window(int carrier_phase, int phase_slip, int squelch);
int Confirm_Save(WINDOW *receive_win);
void Save_QSO_Record(void);
void Draw_Box(int rows, int cols, int top, int left);
void Draw_Hline(WINDOW *win, int top, int left, int right);
void Draw_Vline(WINDOW *win, int top, int left, int bottom);
/* sound.c */
void Setup_Sound_Card(void);
int Signal_Sample(void);
void DSP_Write(short *buffer, int buff_size);
void Open_PCM(snd_pcm_t **handle, snd_pcm_stream_t stream);
void Xrun_Recovery(snd_pcm_t *handle, int error);
void Open_Capture(void);
void Open_Playback(void);
void Open_Mixer(void);
void Set_Playback_Level(int level);
void Set_Capture_Level(int level);
void Close_Playback(void);
void Close_Capture(void);
void Close_Mixer(void);

/*-------------------------------------------------------------------*/

#endif

