/*  screen.c
 *
 *  New Curses display routines for lpsk31
 */

/*
 *  lpsk31: An application to transmit and receive
 *  PSK31 signals using a computer's sound card
 *
 *  Copyright (C) 2001  Neoklis Kyriazis
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


#include "lpsk31.h"

/* QSO data record */
extern qso_record_t qso_record;

extern FILE
  /* File for recording QSO's */
  *qso_record_fp,
  /* File for ADIF QSO record */
  *log_adif_fp,
  /* File for station log */
  *log_fp,
  /* Pointer to samples file */
  *samples_fp;

  /* DSP signal samples buffer */
  extern int buffer_size;

  extern char
  label[9][19], /* Labels for F1-F9 commands */
  *macro[9];    /* Macros attached to F1-F9  */

/* Index to macros */
int macro_idx;

/* QSO data record */
qso_record_t qso_record;

/* Runtime config data */
extern rc_data_t rc_data;

/* Operator and location data */
extern op_data_t op_data;

/* Operator and location data */
extern qso_record_t qso_record;

/* Signal/DSP samples buffer. 2x size for Stereo */
extern unsigned char *samples_buffer;

/* Command codes for FT847 CAT */
extern unsigned char
  CAT_ON[],
  CAT_OFF[],
  PTT_ON[],
  PTT_OFF[],
  MODE_LSB[],
  MODE_USB[],
  MODE_CW[],
  MODE_CWR[],
  MODE_CWN[],
  MODE_CWNR[];

/*------------------------------------------------------------------------*/

/*  Screen_Root()
 *
 *  Function to create the root window
 */

  void
Screen_Root( void )
{
  /* Box and add titles on stdscr */
  attrset( BORDER_COLOR );
  box( stdscr, ACS_VLINE, ACS_HLINE );
  Draw_Hline( stdscr,  5,  0, 79 );
  Draw_Hline( stdscr, 14,  0, 79 );
  Draw_Vline( stdscr, 14, 18, 24 );
  Draw_Hline( stdscr, 16, 18, 79 );

  /* For the form window */
  mvaddch(14, 33, ACS_TTEE );
  mvaddch(14, 37, ACS_TTEE );
  mvaddch(14, 41, ACS_TTEE );
  mvaddch(14, 53, ACS_TTEE );
  mvaddch(14, 67, ACS_TTEE );
  mvaddch(14, 74, ACS_TTEE );
  mvaddch(16, 33, ACS_BTEE );
  mvaddch(16, 37, ACS_BTEE );
  mvaddch(16, 41, ACS_BTEE );
  mvaddch(16, 53, ACS_BTEE );
  mvaddch(16, 67, ACS_BTEE );
  mvaddch(16, 74, ACS_BTEE );

  /* Show various labels */
  attrset( LABEL_COLOR );
  mvaddstr(  0, 28, " LPSK31 Transmit Window " );
  mvaddstr(  5, 28, " LPSK31 Receive Window " );
  mvaddstr( 14,  3, " Phase/Tune " );
  mvaddstr( 14, 23, " Call " );
  mvaddstr( 14, 35, "IN" );
  mvaddstr( 14, 38, "OUT" );
  mvaddstr( 14, 45, " Name " );
  mvaddstr( 14, 58, " QTH " );
  mvaddstr( 14, 69, "Grid" );
  mvaddstr( 14, 76, "Fr" );

  /* Place function key commands */
  attrset( KEYBD_COMMANDS );
  mvprintw( 17, 19, "F1:" );
  mvprintw( 17, 39, "F2:" );
  mvprintw( 17, 59, "F3:" );
  mvprintw( 18, 19, "F4:" );
  mvprintw( 18, 39, "F5:" );
  mvprintw( 18, 59, "F6:" );
  mvprintw( 19, 19, "F7:" );
  mvprintw( 19, 39, "F8:" );
  mvprintw( 19, 59, "F9:" );
  mvaddstr( 20, 19, "F10:" );
  mvaddstr( 20, 39, "F11:" );
  mvaddstr( 20, 59, "F12:" );

  mvaddstr( 21, 19, "E|e:" );
  mvaddstr( 21, 39, "TAB:" );
  mvaddstr( 21, 59, "RET:" );
  mvaddstr( 22, 19, "M|m:" );
  mvaddstr( 22, 39, "C-W:" );
  mvaddstr( 22, 59, "R|r:" );
  mvaddstr( 23, 19, "T|t:" );
  mvaddstr( 23, 39, "K|k:" );
  mvaddstr( 23, 59, "Esc:" );

  /* Show Macro labels */
  attrset( WHITE );
  mvaddstr( 17, 22, label[0] );
  mvaddstr( 17, 42, label[1] );
  mvaddstr( 17, 62, label[2] );
  mvaddstr( 18, 22, label[3] );
  mvaddstr( 18, 42, label[4] );
  mvaddstr( 18, 62, label[5] );
  mvaddstr( 19, 22, label[6] );
  mvaddstr( 19, 42, label[7] );
  mvaddstr( 19, 62, label[8] );

  /* Show keystroke commands */
  mvaddstr( 20, 23, "Open QSO record" );
  mvaddstr( 20, 43, "Save QSO record" );
  mvaddstr( 20, 63, "Toggle Trans|" );
  mvaddstr( 21, 23, "Edit QSO record" );
  mvaddstr( 21, 43, "Next QSO field" );
  mvaddstr( 21, 63, "Enter QSO record" );
  mvaddstr( 22, 23, "Mode: " );
  mvaddstr( 22, 43, "CW Identity" );
  mvaddstr( 23, 23, "Enable Tcvr CAT" );
  mvaddstr( 23, 43, "Capital Letters" );
  mvaddstr( 23, 63, "Quit | Cancel" );

  attrset( STANDOUT_COLOR );
  mvaddstr( 20, 76, "Rcv" );
  mvaddstr( 22, 63, "Record QSO's" );
  mvaddstr( 22, 29, "BPSK" );

  /* Done with root window */
  refresh();
  doupdate();

} /* End of Screen_Root() */

/*------------------------------------------------------------------------*/

  void
Screen_Transmit_Window( void )
{

  int
	kbd_chr,   /* A character entered at the keyboard  */
	char_cnt;  /* Count characters entered at keyboard */

  WINDOW *transmit_win;

  /* Transmit text window */
  transmit_win = newwin( 4, 78, 1, 1 );

  /* Show Transmit mode */
  attrset( WHITE );
  mvaddstr( 20, 63, "Toggle      |Rcv" );
  attrset( STANDOUT_COLOR );
  mvaddstr( 20, 70, "Trans" );
  refresh();
  doupdate();

  /* Enable scrolling */
  scrollok( transmit_win, TRUE );
  idlok( transmit_win, TRUE );

  /* Show cursor */
  wnoutrefresh( transmit_win );
  doupdate();

  /* Set Transmit flag */
  Set_Flag( MODE_TRANSMIT );

  /* Switch to SSB and PTT ON */
  Set_Tcvr_Mode();

  Transmit_Preamble();

  /* Process keystrokes from user */
  kbd_chr = -1;
  char_cnt = 0;
  while( 1 )
  {
	/* Transmit macro if selected */
	if( isFlagSet(TRANSMIT_MACRO) )
	  /* Return to Receive if flaged */
	  if( Transmit_Macro(transmit_win) )
		return;

	/*** Process keystrokes ***/
	kbd_chr = getch();

	/* Globb F1-F9 codes */
	if( (kbd_chr >= KEY_F(1)) &&
		(kbd_chr <= KEY_F(9)) )
	{
	  macro_idx = kbd_chr - KEY_F(1);
	  kbd_chr = MACRO_KEY;
	}

	/* Capitalize letters if enabled */
	if( isFlagSet(CAPITAL_LETTERS) )
	  if( (kbd_chr > 0x60) && (kbd_chr < 0x7b) )
		kbd_chr -= 0x20;

	switch( kbd_chr )
	{
	  case MACRO_KEY:	/* Transmit macros */
		Set_Flag( TRANSMIT_MACRO );
		break;

	  case KEY_F(12): case KEY_ESC: /* Return to receive window */
		if( kbd_chr != KEY_ESC )
		  Transmit_Postamble();
		attrset( WHITE );
		mvaddstr( 20, 63, "Toggle Trans|" );
		attrset( STANDOUT_COLOR );
		mvaddstr( 20, 76, "Rcv" );
		doupdate();

		/* Switch to CWN or CWN-R and PTT OFF */
		Clear_Flag( MODE_TRANSMIT );
		Set_Tcvr_Mode();
		return;

	  case KEY_CTRL_W: /* Send CW Identity */
		attrset( STANDOUT_COLOR );
		mvaddstr( 22, 43, "CW Identity" );
		refresh();
		doupdate();

		Transmit_Postamble();
		Morse_Transmit( "  DE " );
		Morse_Transmit( op_data.call );

		attrset( WHITE );
		mvaddstr( 22, 43, "CW Identity" );

		/* Switch to CWN or CWN-R and PTT OFF */
		Clear_Flag( MODE_TRANSMIT );
		Set_Tcvr_Mode();
		return;

	  default: /* Transmit keybd character */
		if( kbd_chr != -1 )
		{
		  if( kbd_chr == KEY_RET ) /* Add LF to CR */
		  {
			char_cnt = 0; /* Reset word wrap */
			Transmit_Character( CR, transmit_win );
			Transmit_Character( LF, transmit_win );
			break;
		  }

		  /* Do word wrapping */
		  if( (char_cnt >= rc_data.wwrap) && (kbd_chr == ' ') )
		  {
			char_cnt = 0; /* Reset word wrap */
			Transmit_Character( CR, transmit_win );
			Transmit_Character( LF, transmit_win );
			break;
		  }

		  /* Backspace cursor */
		  if( kbd_chr == KEY_BACKSPACE )
		  {
			char_cnt--;
			kbd_chr = BS;
		  }

		  /* Transmit keyboard char and record */
		  char_cnt++;
		  Transmit_Character( kbd_chr, transmit_win );

		} /* if( kbd_chr != -1 ) */

	} /* switch( kbd_chr ) */

	/* Transmit 'null' character in idle periods */
	if( kbd_chr == -1 )
	  Transmit_Character( NULL_CHAR, transmit_win );

  } /* while( 1 ) */

} /* End of Screen_Transmit_Window() */

/*------------------------------------------------------------------------*/

/*  Transmit_Macro()
 *
 *  Transmits a pre-recorded Macro
 */

  int
Transmit_Macro( WINDOW *transmit_win )
{
  int
	macro_cnt, /* Count macro characters transmitted   */
	char_cnt,  /* Count characters entered at keyboard */
	idx;

  /* Tags for entering fields in macros */
  char *tags[NUM_OF_TAGS] = TAG_TABLE;

  char
	tag_str[10], /* String within a tag <> */
	tag_rep[22]; /* Tag replacement string */

  /* Stay in Tx mode if last char is ~ */
  macro_cnt = strlen( macro[macro_idx] );
  if( macro[macro_idx][macro_cnt-2] == '~' )
	Set_Flag( STAY_IN_TX_MODE );
  else
	Clear_Flag( STAY_IN_TX_MODE );

  macro_cnt = 0;

  /* If Macro begins with a ^ transmit "New Line" */
  if( macro[macro_idx][0] == '^' )
  {
	Transmit_Character( CR, transmit_win );
	Transmit_Character( LF, transmit_win );
	macro_cnt++;
  }

  /* Stop transmitting at string terminator */
  while( macro[macro_idx][macro_cnt] != '\0' )
  {
	/* Abort Macro on space keystroke */
	if( getch() == KEY_SPACE )
	{
	  Set_Flag( STAY_IN_TX_MODE );
	  break;
	}

	/*** Look for tags and substitude accordingly ***/
	if( macro[macro_idx][macro_cnt] == '<' )
	{
	  /*** Copy tag string to buffer and identify ***/
	  macro_cnt++; /* Move into tag field */

	  idx = 0;
	  /* Copy line to tag, max 12 chars or EOS */
	  while( (idx < 12) &&
		  (macro[macro_idx][macro_cnt] != '>') )
		tag_str[idx++] = macro[macro_idx][macro_cnt++];

	  /* Terminate tag buffer and advance macro buffer */
	  tag_str[idx] = '\0';
	  macro_cnt++;

	  /* Identify tag and substitude accordingly */
	  idx = 0;
	  while( idx < NUM_OF_TAGS )
	  {
		if( strcmp(tag_str, tags[idx]) == 0 )
		  break;
		idx++;
	  }

	  /* Abort if tag unidentified */
	  if( idx == NUM_OF_TAGS )
		Abort_On_Error( -14 );

	  /* Replace tags */
	  switch( idx )
	  {
		case 0: /* own-call */
		  strncpy( tag_rep, op_data.call, 17 );
		  break;

		case 1: /* own-name */
		  strncpy( tag_rep, op_data.name, 17 );
		  break;

		case 2: /* own-qth */
		  strncpy( tag_rep, op_data.qth, 21 );
		  break;

		case 3: /* own-loc */
		  strncpy( tag_rep, op_data.loc, 7 );
		  break;

		case 4: /* own-rst */
		  strncpy( tag_rep, qso_record.rst_in, 4 );
		  break;

		case 5: /* rem-call */
		  strncpy( tag_rep, qso_record.call, 15 );
		  break;

		case 6: /* rem-name */
		  strncpy( tag_rep, qso_record.name, 12 );
		  break;

		case 7: /* rem-qth */
		  strncpy( tag_rep, qso_record.qth, 14 );
		  break;

		case 8: /* rem-loc */
		  strncpy( tag_rep, qso_record.loc, 7 );
		  break;

		case 9: /* rem-rst */
		  strncpy( tag_rep, qso_record.rst_out, 4 );
		  break;

		case 10: /* date-time */
		  strncpy( tag_rep, qso_record.date, 12 );
		  strcat ( tag_rep, " " );
		  strncat( tag_rep, qso_record.time, 9 );
		  break;

		case 11: /* op-freq */
		  strncpy( tag_rep, qso_record.freq, 5 );
		  break;

		case 12: /* App version */
		  strncpy( tag_rep, VERSION, 21 );
		  tag_rep[21] = 0;

	  } /* switch( idx ) */

	  /*** Transmit tag replacement ***/
	  idx = 0;
	  while( tag_rep[idx] != '\0' )
	  {
		/* Capitalize letters if enabled */
		if( isFlagSet(CAPITAL_LETTERS) )
		  if( (tag_rep[idx] > 0x60) && (tag_rep[idx] < 0x7b) )
			tag_rep[idx] -= 0x20;

		Transmit_Character( tag_rep[idx++], transmit_win );
		char_cnt++;
	  }

	} /* if( macro[macro_idx][macro_cnt] == '<' ) */

	/* Do not transmit '^' */
	if( macro[macro_idx][macro_cnt] == '~' )
	{
	  beep();
	  break;
	}

	/* Capitalize letters if enabled */
	if( isFlagSet(CAPITAL_LETTERS) )
	  if( (macro[macro_idx][macro_cnt] > 0x60) &&
		  (macro[macro_idx][macro_cnt] < 0x7b) )
		macro[macro_idx][macro_cnt] -= 0x20;

	/* Transmit a char from Macro */
	Transmit_Character(macro[macro_idx][macro_cnt], transmit_win);
	macro_cnt++;
	char_cnt++;

  } /* while( macro[macro_idx][macro_cnt] != '\0' ) */

  /*** Finished with macro ***/
  /* Flag last character with a NULL_CHAR */
  Transmit_Character( NULL_CHAR, transmit_win );
  Clear_Flag( TRANSMIT_MACRO );
  fflush( qso_record_fp );

  /* Remain in Tx mode if flagged, else return */
  if( isFlagClear(STAY_IN_TX_MODE) )
  {
	Transmit_Postamble();
	attrset( WHITE );
	mvaddstr( 20, 63, "Toggle Trans|" );
	attrset( STANDOUT_COLOR );
	mvaddstr( 20, 76, "Rcv" );
	doupdate();

	/* Switch to CWN or CWN-R and PTT OFF */
	Clear_Flag( MODE_TRANSMIT );
	Set_Tcvr_Mode();
	return( 1 );
  }

  return(0);

} /* Transmit_Macro() */

/*------------------------------------------------------------------------*/

/*  Transmit_Character()
 *
 *  Transmits a character and also
 *  records it to file if enabled
 */

  void
Transmit_Character( int chr, WINDOW * transmit_win )
{
  /* Record non-null character, if enabled */
  if( chr != NULL_CHAR )
  {
	if( isFlagSet(RECORD_QSO) )
	  fprintf( qso_record_fp, "%c", chr );

	/* Done with transmit_win */
	if( chr != CR )
	  waddch( transmit_win, chr );
	wnoutrefresh( transmit_win );
	doupdate();

  } /* if( chr != NULL_CHAR ) */

  /* Transmit keyboard char and record */
  Transmit_PSK_Character( chr );

} /* Transmit_Character() */

/*------------------------------------------------------------------------*/

  void
Screen_Receive_Window( void )
{
  int
	idx1,    /* Index for loops etc */
	idx2,    /* Index for loops etc */
	done,    /* Form processed flag */
	cols,    /* Columns in form     */
	rows,    /* Rows in form        */
	kbd_chr, /* A character entered at the keyboard    */
	dec_chr; /* A character decoded from PSK31 signals */

  /* Variables for time and date */
  time_t tp;    /* Time type     */
  struct tm dt; /* Date and time */

  char
	/* File path for QSO record  */
	qso_record_fpath[64],
	/* Tcvr mode, frequency and  */
	/* RST read via CAT commands */
	mode,
	freq[5],
	rst[4];

  /* Form for entering Manual Position parameters */
  FORM   *qso_form;
  FIELD  *data_field[8];

  /* Form containing window */
  WINDOW *form_subwin;

  /* Receive window */
  WINDOW *receive_win;
  receive_win = newwin( 8, 78, 6, 1 );


  /* Default mode is BPSK and Record QSO */
  Set_Flag( MODE_BPSK | RECORD_QSO );

  /* Make file path for record file */
  snprintf( qso_record_fpath, 64, "%s/lpsk31/record.txt", getenv("HOME") );

  /* Create a file for recording QSO's */
  if( qso_record_fp == NULL )
  {
	qso_record_fp = fopen( qso_record_fpath, "a" );
	if( qso_record_fp == NULL )
	{
	  Cleanup();
	  perror( "qso_record_fp" );
	  exit(-1);
	}

  } /* if( QSO_record_fp == NULL ) */

  /* Enable scrolling */
  scrollok( receive_win, TRUE );
  idlok( receive_win, TRUE );

  /* Setup QSO fields and form */
  data_field[0] = new_field( 1, 14, 0,  0, 0, 0 );
  data_field[1] = new_field( 1,  3, 0, 15, 0, 0 );
  data_field[2] = new_field( 1,  3, 0, 19, 0, 0 );
  data_field[3] = new_field( 1, 11, 0, 23, 0, 0 );
  data_field[4] = new_field( 1, 13, 0, 35, 0, 0 );
  data_field[5] = new_field( 1,  6, 0, 49, 0, 0 );
  data_field[6] = new_field( 1,  4, 0, 56, 0, 0 );

  data_field[7] = ( FIELD * ) NULL;

  /* Set field types and validation */
  for( idx1 = 0; idx1 < 7; idx1++ )
	field_opts_off( data_field[idx1], O_AUTOSKIP );

  set_field_type( data_field[0], TYPE_REGEXP, "^[A-Z|0-9][A-Z|0-9|/]*[A-Z]* *$");
  set_field_type( data_field[1], TYPE_REGEXP, "[3-5][3-9][6-9]" );
  set_field_type( data_field[2], TYPE_REGEXP, "[3-5][3-9][6-9]" );
  set_field_type( data_field[5], TYPE_REGEXP,"[A-S][A-S][0-9][0-9][A-X][A-X]");
  set_field_type( data_field[6], TYPE_NUMERIC, 2, 0.05, 9999.0 );

  /* Enter defaults in QSO form */
  set_field_buffer( data_field[0], 0, "  Call-sign   " );
  set_field_buffer( data_field[1], 0, "RST" );
  set_field_buffer( data_field[2], 0, "RST" );
  set_field_buffer( data_field[3], 0, "  Op Name   " );
  set_field_buffer( data_field[4], 0, "  QTH Name   " );
  set_field_buffer( data_field[5], 0, " Loct " );
  set_field_buffer( data_field[6], 0, " MHz" );

  /* Declare QSO entries form */
  qso_form = new_form( data_field );

  /* Obtain size of from */
  scale_form( qso_form, &rows, &cols );

  /* Create form sub-window */
  form_subwin = newwin( rows, cols, 15, 19 );

  /* Set form window and sub-window */
  set_form_sub( qso_form, form_subwin );

  post_form( qso_form );

  /* Separate form fields */
  wattrset( form_subwin, BORDER_COLOR );
  mvwaddch( form_subwin, 0, 14, ACS_VLINE );
  mvwaddch( form_subwin, 0, 18, ACS_VLINE );
  mvwaddch( form_subwin, 0, 22, ACS_VLINE );
  mvwaddch( form_subwin, 0, 34, ACS_VLINE );
  mvwaddch( form_subwin, 0, 48, ACS_VLINE );
  mvwaddch( form_subwin, 0, 55, ACS_VLINE );

  wnoutrefresh( receive_win );
  wnoutrefresh( form_subwin );
  doupdate();

  /*** Process keystrokes from user in receive_win ***/
  kbd_chr = -1;
  while( kbd_chr != KEY_ESC )
  {
	kbd_chr = getch();

	/* Globb F1-F9 codes */
	if( (kbd_chr >= KEY_F(1)) &&
		(kbd_chr <= KEY_F(9)) )
	{
	  macro_idx = kbd_chr -  KEY_F(1);
	  kbd_chr = MACRO_KEY;
	}

	switch( kbd_chr )
	{
	  case MACRO_KEY:	/* Transmit macros */
		Set_Flag( TRANSMIT_MACRO );
		Screen_Transmit_Window();
		break;

	  case KEY_F(10): /* Post new QSO record form */
		/* Save record if needed */
		if( isFlagSet(RECORD_EDITED)  &&
			isFlagClear(RECORD_SAVED) &&
			Confirm_Save( receive_win ) )
		  Save_QSO_Record();

		/* Just clear flags if record saved */
		Clear_Flag(RECORD_EDITED);
		Clear_Flag(RECORD_SAVED);

		attrset( BORDER_COLOR );
		Draw_Hline( stdscr, 16, 18, 79 );
		mvaddch(16, 33, ACS_BTEE );
		mvaddch(16, 37, ACS_BTEE );
		mvaddch(16, 41, ACS_BTEE );
		mvaddch(16, 53, ACS_BTEE );
		mvaddch(16, 67, ACS_BTEE );
		mvaddch(16, 74, ACS_BTEE );

		/* Clear fields */
		for( idx1 = 0; idx1 < 7; idx1++ )
		{
		  set_field_buffer( data_field[idx1], 0, "" );
		  *field_buffer( data_field[idx1], 0 ) = '\0';
		}

		/* Read Receiver status if CAT enabled */
		if( isFlagSet(ENABLE_CAT) )
		{
		  Read_Rx_Status( &mode, freq, rst );
		  set_field_buffer( data_field[2], 0, rst );
		  set_field_buffer( data_field[6], 0, freq);
		}

		/* Enter time and date to QSO record */
		time( &tp );
		dt = *gmtime( &tp );
		strftime( qso_record.date, 12, "%d/%b/%Y", &dt );
		strftime( qso_record.time,  9, "%H:%M:%S", &dt );
		strftime( qso_record.date_adif, 9, "%Y%m%d", &dt );
		strftime( qso_record.time_adif, 5, "%H%M", &dt );

	  case 'e': case 'E': /* Edit QSO record */
		/* Prepare for the form's data input loop */
		form_driver( qso_form, REQ_FIRST_FIELD );
		set_field_back( data_field[0], FORM_COLOR );

		wnoutrefresh( form_subwin );
		doupdate();

		Clear_Flag( RECORD_SAVED );

		/*** Process keystrokes from user in qso_form ***/
		kbd_chr = done = 0;
		while( !done )
		{
		  kbd_chr = getch();

		  /* Globb F1-F9 codes */
		  if( (kbd_chr >= KEY_F(1)) &&
			  (kbd_chr <= KEY_F(9)) )
		  {
			macro_idx = kbd_chr -  KEY_F(1);
			kbd_chr = MACRO_KEY;
		  }

		  switch( kbd_chr )
		  {
			case KEY_RET : /* Process form */
			  /* Change field to verify below */
			  set_field_back( current_field(qso_form), WHITE );
			  form_driver( qso_form, REQ_NEXT_FIELD );
			  set_field_back( current_field(qso_form), FORM_COLOR );

			  /* Remove trailing white space */
			  for( idx1 = 0; idx1 < 7; idx1++ )
			  {
				idx2 = strlen( field_buffer(data_field[idx1], 0) ) - 1;
				while( (idx2 >= 0) && (*(field_buffer(data_field[idx1], 0)+idx2) == ' ') )
				  idx2--;
				*(field_buffer(data_field[idx1],0) + ++idx2) = '\0';
			  }

			  /* Request that Call, RST and Freq are valid?? */
			  if( strlen(field_buffer(data_field[0], 0)) < 2 ||
				  strlen(field_buffer(data_field[2], 0)) < 3 ||
				  strlen(field_buffer(data_field[6], 0)) < 1 )
				break;

			  /* Remove highlighting */
			  set_field_back( current_field( qso_form ), WHITE );
			  form_driver( qso_form, REQ_NEXT_FIELD );

			  /* Enter field values to QSO data structure */
			  strncpy(qso_record.call,    field_buffer(data_field[0],0),15);
			  strncpy(qso_record.rst_in,  field_buffer(data_field[1],0), 4);
			  strncpy(qso_record.rst_out, field_buffer(data_field[2],0), 4);
			  strncpy(qso_record.name,    field_buffer(data_field[3],0),12);
			  strncpy(qso_record.qth,     field_buffer(data_field[4],0),14);
			  strncpy(qso_record.loc,     field_buffer(data_field[5],0), 7);
			  strncpy(qso_record.freq,    field_buffer(data_field[6],0), 5);

			  /* Enter operating mode */
			  if( isFlagSet(MODE_BPSK) )
				strcpy( qso_record.mode, "BPSK" );
			  else
				strcpy( qso_record.mode, "QPSK" );

			  /* Mark QSO record as having some entries */
			  Set_Flag( RECORD_EDITED );
			  done = 1; /* Leave form loop */
			  break;

			case KEY_TAB : /* Right a field */
			  set_field_back( current_field(qso_form), WHITE );
			  form_driver( qso_form, REQ_NEXT_FIELD );
			  set_field_back( current_field(qso_form), FORM_COLOR );

			  /* Enter field values to QSO data structure. This  */
			  /* is not normal practice but is needed so that    */
			  /* field values (Call etc) are available to macros */

			  /* Remove trailing white space */
			  for( idx1 = 0; idx1 < 7; idx1++ )
			  {
				idx2 = strlen( field_buffer(data_field[idx1], 0) ) - 1;
				while( (idx2 >= 0) && (*(field_buffer(data_field[idx1], 0)+idx2) == ' ') )
				  idx2--;
				*(field_buffer(data_field[idx1],0)+ ++idx2) = '\0';
			  }

			  /* Enter field values to QSO data structure */
			  strncpy(qso_record.call,    field_buffer(data_field[0],0),15);
			  strncpy(qso_record.rst_in,  field_buffer(data_field[1],0), 4);
			  strncpy(qso_record.rst_out, field_buffer(data_field[2],0), 4);
			  strncpy(qso_record.name,    field_buffer(data_field[3],0),12);
			  strncpy(qso_record.qth,     field_buffer(data_field[4],0),14);
			  strncpy(qso_record.loc,     field_buffer(data_field[5],0), 7);
			  strncpy(qso_record.freq,    field_buffer(data_field[6],0), 5);

			  /* Mark QSO record as having some entries */
			  Set_Flag( RECORD_EDITED );
			  break;

			case KEY_BACKSPACE : /* Back and delete a character */
			  form_driver( qso_form, REQ_PREV_CHAR );
			  form_driver( qso_form, REQ_DEL_CHAR );
			  break;

			case KEY_LEFT : /* Left a character */
			  form_driver( qso_form, REQ_PREV_CHAR );
			  break;

			case KEY_RIGHT : /* Right a character */
			  form_driver( qso_form, REQ_NEXT_CHAR );
			  break;

			case KEY_DC : /* Delete a character */
			  form_driver( qso_form, REQ_DEL_CHAR );
			  break;

			case MACRO_KEY:	/* Transmit macros */
			  Set_Flag( TRANSMIT_MACRO );
			  Screen_Transmit_Window();
			  break;

			case KEY_F(12): /* Switch to transmit window */
			  Screen_Transmit_Window();
			  break;

			case KEY_ESC : /* Cancel operation or Quit */
			  /* Remove highlighting */
			  set_field_back( current_field(qso_form), WHITE );
			  form_driver( qso_form, REQ_NEXT_FIELD );
			  wnoutrefresh( form_subwin );
			  doupdate();
			  break;

			default : /* Enter characters */
			  /* Capitalize letters in all fields */
			  if( (kbd_chr > 0x60) && (kbd_chr < 0x7b) )
				kbd_chr -= 0x20;

			  form_driver( qso_form, kbd_chr );

		  } /* End of switch( kbd_chr )  */

		  /* Abort and exit function (and lpsk31) on ESC */
		  if( kbd_chr == KEY_ESC )
			break;

		  /* Print characters to Receive window */
		  dec_chr = Decode_PSK_Character();
		  if( dec_chr <= 127 )
			waddch( receive_win, dec_chr );

		  /* Done with receive_win */
		  wnoutrefresh( receive_win );
		  wnoutrefresh( form_subwin );
		  doupdate();

		} /* End of while( !done ) */

		kbd_chr = 0;
		break;

	  case KEY_F(11):	/* Save QSO record in ADIF format */
		/* If record has had some entries */
		if( isFlagSet(RECORD_EDITED)  &&
			isFlagClear(RECORD_SAVED) &&
			Confirm_Save( receive_win ) )
		  Save_QSO_Record();
		break;

	  case KEY_F(12): /* Switch to transmit window */
		Screen_Transmit_Window();
		break;

	  case 'm': case 'M': /* Toggle modes */
		attrset( STANDOUT_COLOR );

		if( isFlagSet(MODE_BPSK) )
		{
		  Set_Flag( MODE_QPSK_USB );
		  Clear_Flag( MODE_BPSK | MODE_QPSK_LSB );
		  mvaddstr( 22, 29, "QPSK-USB" );
		  Init_Viterbi();
		}
		else
		  if( isFlagSet(MODE_QPSK_USB) )
		  {
			Set_Flag( MODE_QPSK_LSB );
			Clear_Flag( MODE_QPSK_USB | MODE_BPSK );
			mvaddstr( 22, 29, "QPSK-LSB" );
			Init_Viterbi();
		  }
		  else
		  {
			Set_Flag( MODE_BPSK );
			Clear_Flag( MODE_QPSK_USB | MODE_QPSK_LSB );
			mvaddstr( 22, 29, "BPSK    " );
		  }

		Set_Tcvr_Mode();
		refresh();
		doupdate();
		break;

	  case 't': case 'T': /* Toggle Transceiver CAT */
		if( isFlagClear(ENABLE_CAT) )
		{
		  Set_Flag( ENABLE_CAT );
		  Open_Tcvr_Serial();
		  Write_Tcvr_Command( CAT_ON );
		  Set_Tcvr_Mode();
		  attrset( STANDOUT_COLOR );
		  mvaddstr( 23, 23, "Enable Tcvr CAT" );
		}
		else
		{
		  Write_Tcvr_Command( CAT_OFF );
		  Close_Tcvr_Serial();
		  Clear_Flag( ENABLE_CAT );
		  attrset( WHITE );
		  mvaddstr( 23, 23, "Enable Tcvr CAT" );
		}
		break;

	  case 'k': case 'K': /* Capitalize letters in Transmit */
		attrset( STANDOUT_COLOR );
		if( isFlagClear(CAPITAL_LETTERS) )
		{
		  mvaddstr( 0, 52, "[C] " );
		  Set_Flag( CAPITAL_LETTERS );
		}
		else
		{
		  mvaddstr( 0, 52, "[L] " );
		  Clear_Flag( CAPITAL_LETTERS );
		}
		break;

	  case 'r': case 'R': /* Toggle QSO recording */
		if( isFlagClear(RECORD_QSO) )
		{
		  Set_Flag( RECORD_QSO );

		  /* Create a file for recording QSO */
		  if( qso_record_fp == NULL )
		  {
			qso_record_fp = fopen( qso_record_fpath, "a" );
			if( qso_record_fp == NULL )
			{
			  Cleanup();
			  perror( "qso_record_fp" );
			  exit(-1);
			}

		  } /* if( QSO_record_fp == NULL ) */

		  attrset( STANDOUT_COLOR );
		  mvaddstr( 22, 63, "Record QSO's" );
		}
		else
		{
		  Clear_Flag( RECORD_QSO );
		  fclose( qso_record_fp );
		  qso_record_fp = NULL;
		  attrset( WHITE );
		  mvaddstr( 22, 63, "Record QSO's" );
		}
		break;

	  case 'f': case 'F': /* Read samples from file */
		/* Open file for reading  samples */
		if( samples_fp == NULL )
		  samples_fp = fopen( "lpsk31.bin", "r" );
		if( samples_fp == NULL )
		{
		  Cleanup();
		  perror( "lpsk31.bin" );
		  exit(-1);
		}

		Set_Flag( READ_FILE );
		break;

	  case 'w': case 'W': /* Toggle write samples to file */
		if( samples_fp != NULL )
		  fclose( samples_fp );
		samples_fp = NULL;

		if( isFlagClear(WRITE_FILE) )
		{
		  /* Open file for writing  samples */
		  samples_fp = fopen( "lpsk31.bin", "w+" );
		  if( samples_fp == NULL )
		  {
			Cleanup();
			perror( "lpsk31.bin" );
			exit(-1);
		  }

		  Set_Flag( WRITE_FILE );

		} /* if( isFlagClear(WRITE_FILE) ) */
		else
		{
		  if( samples_fp != NULL )
			fclose( samples_fp );

		  samples_fp = NULL;
		  Clear_Flag( WRITE_FILE );
		  break;
		}
		break;

	  case 's': case 'S': /* Record signal (RX audio) to file */
		Record_Signal();

	} /* switch( kbd_chr ) */

	/* Print characters to Receive window & file */
	dec_chr = Decode_PSK_Character();
	if( dec_chr <= 127 )
	{
	  waddch( receive_win, dec_chr );

	  if( isFlagSet(RECORD_QSO) )
		fprintf( qso_record_fp, "%c", dec_chr );
	}

	/* Done with receive_win */
	wnoutrefresh( receive_win );
	doupdate();

  } /* while( kbd_chr != KEY_ESC ) */

  /* Unpost form and free fields */
  unpost_form( qso_form );
  free_form(   qso_form );
  for ( idx1 = 0; idx1 < 8; idx1++ ) free_field( data_field[idx1] );
  fflush( qso_record_fp );

  Set_Tcvr_Mode();
  Write_Tcvr_Command( CAT_OFF );
  Close_Tcvr_Serial();

} /* End of Screen_Receive_Window() */

/*------------------------------------------------------------------------*/

/*  Screen_Phase_Window()
 *
 *  Displays phase and squelch
 */

  void
Screen_Phase_Window( int carrier_phase, int phase_slip, int squelch )
{
  int
	plot_x,     /* X coordinate for plotting */
	plot_y,     /* Y coordinate for plotting */
	table_idx,  /* Index to phase plot table */
	idx;

  /* Phase plot coordinate table */
  static char plot_table[34] = PLOT_TABLE;

  WINDOW *phase_win;
  phase_win = newwin( 9, 17, 15, 1 );

  mvwaddstr( phase_win, 8, 0, "-----------------" );

  /*** Plot carrier phase on 'scope' ***/
  /* Create a 'dial' ... */
  wattrset( phase_win, GREEN );
  mvwaddstr( phase_win, 0, 0, "     .  .  .     " );
  mvwaddstr( phase_win, 1, 0, "  .           .  " );
  mvwaddstr( phase_win, 2, 0, " .             . " );
  mvwaddstr( phase_win, 3, 0, ".               ." );
  mvwaddstr( phase_win, 4, 0, " .             . " );
  mvwaddstr( phase_win, 5, 0, "  .           .  " );
  mvwaddstr( phase_win, 6, 0, "     .  .  .     " );

  /* Display detector channel */
  if( isFlagSet(CHANNEL_A_SELECT) )
	mvwaddstr( phase_win, 3, 7, "(A)" );
  else
	mvwaddstr( phase_win, 3, 7, "(B)" );

  /* Find plot co-ordinates from table */
  carrier_phase = 360 - carrier_phase;
  table_idx = (2 * carrier_phase) / 45;
  plot_x = 3 - plot_table[(2 * table_idx)];
  plot_y = 8 + plot_table[(2 * table_idx) + 1];

  /* Plot carrier phase */
  wattrset( phase_win, GREEN | A_BOLD );
  mvwaddch( phase_win, plot_x, plot_y, 'o' );

  /*** Create bar graph for displaying phase slip ***/
  /* Plot red sectors */
  wattrset( phase_win, RED );
  for( idx = 0; idx < 7; idx++ )
  {
	mvwaddch( phase_win, 7, idx,    '|' );
	mvwaddch( phase_win, 7, idx+10, '|' );
  }

  /* Plot green sectors */
  wattrset( phase_win, GREEN );
  for( idx = 7; idx < 10; idx++ )
	mvwaddch( phase_win, 7, idx, '|' );

  /* Plot the phase slip pointer */
  plot_x = 8 + (-phase_slip / 5);
  if( plot_x < 0 )
	plot_x = 0;
  else
	if( plot_x > 16 )
	  plot_x = 16;
  wattrset( phase_win, WHITE_ON_GREEN );
  mvwaddch( phase_win, 7, plot_x, ACS_UARROW );

  /*** Make a plot of the squelch level ***/
  if( isFlagSet(SQUELCH_OPEN) )
	wattrset( phase_win, GREEN );
  else
	wattrset( phase_win, RED );

  /* Limit range of plot position */
  plot_x = squelch / 6;
  if( plot_x > 96 )
	plot_x = 96;

  /* Draw squelch bar graph */
  for( idx = 0; idx < plot_x; idx++ )
	mvwaddch( phase_win, 8, idx, '=' );
  mvwaddch( phase_win, 8, idx, '>' );

  /* Done with phase_win */
  wnoutrefresh( phase_win );
  doupdate();

} /* End of Screen_Phase_Window() */

/*------------------------------------------------------------------------*/

/*  Confirm_Save()
 *
 *  Confirm "save record" popup
 */

  int
Confirm_Save( WINDOW *receive_win )
{
  int
	/* PSK31 decoded char and keybd char */
	dec_chr,
	kbd_chr = -1;

  /* Pop-up (sort of) a Save Record question */
  attrset( STANDOUT_COLOR );
  mvaddstr( 16, 32, " [ Save record? (F11=YES | F10=NO) ] " );

  /* Keyboard input loop */
  while( kbd_chr < 0 )
  {
	kbd_chr = getch();
	switch( kbd_chr )
	{
	  case KEY_F(11): /* Save record */
		attrset( WHITE );
		mvaddstr( 16, 32 , " [ Current record SAVED to log.adif ] " );
		refresh();
		doupdate();
		return( 1 );

	  case KEY_F(10): /* Do not save record */
		attrset( BORDER_COLOR );
		Draw_Hline( stdscr, 16, 18, 79 );
		mvaddch(16, 33, ACS_BTEE );
		mvaddch(16, 37, ACS_BTEE );
		mvaddch(16, 41, ACS_BTEE );
		mvaddch(16, 54, ACS_BTEE );
		mvaddch(16, 68, ACS_BTEE );
		mvaddch(16, 75, ACS_BTEE );
		attrset( STANDOUT_COLOR );
		mvaddstr( 16, 31 , " [ Current record has NOT BEEN SAVED ] " );
		refresh();
		doupdate();
		return( 0 );

	  case KEY_ESC: /* Cancel all */
		attrset( BORDER_COLOR );
		Draw_Hline( stdscr, 16, 18, 79 );
		mvaddch(16, 33, ACS_BTEE );
		mvaddch(16, 37, ACS_BTEE );
		mvaddch(16, 41, ACS_BTEE );
		mvaddch(16, 54, ACS_BTEE );
		mvaddch(16, 68, ACS_BTEE );
		mvaddch(16, 75, ACS_BTEE );
		return( 0 );

	  default: /* Print characters to Receive window & file */
		dec_chr = Decode_PSK_Character();
		if( dec_chr <= 127 )
		{
		  waddch( receive_win, dec_chr );

		  if( isFlagSet(RECORD_QSO) )
			fprintf( qso_record_fp, "%c", dec_chr );
		}
		wnoutrefresh( receive_win );
		doupdate();

	} /* switch( kbd_chr = getch() ) */

  } /* while( kbd_chr < 0 ) */

  return( 0 );

} /* Confirm_Save( void ) */

/*------------------------------------------------------------------------*/

/*  Save_QSO_Record()
 *
 *  Saves QSO record to .adif and .txt files
 */

  void
Save_QSO_Record( void )
{
  char
	/* File path for station QSO log */
	log_fpath[64],
	/* File path for ADIF QSO record */
	log_adif_fpath[64];

  /* Create a file for QSO ADIF log if not open */
  if( log_adif_fp == NULL )
  {
	snprintf( log_adif_fpath, 64, "%s/lpsk31/log.adif", getenv("HOME") );
	if( (log_adif_fp = fopen(log_adif_fpath, "r")) == NULL )
	{
	  log_adif_fp = fopen( log_adif_fpath, "a" );
	  if( log_adif_fp == NULL )
	  {
		Cleanup();
		perror( "log_adif_fp" );
		exit(-1);
	  }

	  fprintf( log_adif_fp,
		  "    QSO Log file created in ADIF v1.0 "
		  "format by %s <EOH>\n\n", VERSION );

	} /* if( fopen(log_adif_fpath, "r") == NULL ) */
	else
	{
	  fclose( log_adif_fp );
	  log_adif_fp = fopen( log_adif_fpath, "a" );
	  if( log_adif_fp == NULL )
	  {
		Cleanup();
		perror( "log_adif_fp" );
		exit(-1);
	  }
	} /* else */

  } /* if( log_adif_fp == NULL ) */

  /* Print QSO record to adif format file */
  fprintf( log_adif_fp,
	  "<CALL:%d>%s<QSO_DATE:8>%s<TIME_ON:4>%s\n"
	  "<FREQ:%d>%s<MODE:5>PSK31<RST_SENT:3>%3s<EOR>\n\n",
	  (int)strlen(qso_record.call), qso_record.call,
	  qso_record.date_adif, qso_record.time_adif,
	  (int)strlen(qso_record.freq), qso_record.freq,
	  qso_record.rst_out);
  fflush( log_adif_fp );

  /* Create a file for station log if not open */
  if( log_fp == NULL )
  {
	snprintf( log_fpath, 64, "%s/lpsk31/log.txt", getenv("HOME") );
	if( (log_fp = fopen(log_fpath, "r")) == NULL )
	{
	  log_fp = fopen( log_fpath, "a" );
	  if( log_fp == NULL )
	  {
		Cleanup();
		perror( "log_fp" );
		exit(-1);
	  }

	  fprintf( log_fp,
		  "    Station Log file created in Text "
		  "format by %s\n%s\n", VERSION,
		  "|---------------------------"
		  "---------------------------------------|" );

	} /* if( fopen(log_fpath, "r") == NULL ) */
	else
	{
	  fclose( log_fp );
	  log_fp = fopen( log_fpath, "a" );
	  if( log_fp == NULL )
	  {
		Cleanup();
		perror( "log_fp" );
		exit(-1);
	  }
	} /* else */

  } /* if( log_fp == NULL ) */

  /* Print record in text log file */
  fprintf( log_fp,
	  "| DATE-UTC: %11s | CALLSIGN: %14s | RST-SENT: %3s | \n"
	  "| TIME-UTC: %8s    |  OP NAME:    %11s | RST-RCVD: %3s | \n"
	  "| F-MHz/Md: %4s - %4s |      QTH:  %13s | LOCTR: %6s |\n",
	  qso_record.date, qso_record.call, qso_record.rst_out,
	  qso_record.time, qso_record.name, qso_record.rst_in,
	  qso_record.freq, qso_record.mode, qso_record.qth, qso_record.loc);

  fprintf( log_fp, "|-----------------------|"
	  "--------------------------|---------------|\n" );

  fflush( log_fp );

  Set_Flag( RECORD_SAVED );

} /* Save_QSO_Record() */

/*------------------------------------------------------------------------*/

/*  Draw_Box()
 *
 *  Function to draw a box of given size and position in the root window
 */

  void
Draw_Box( int rows, int cols, int top, int left )
{
  mvvline( top + 1, left, ACS_VLINE, rows - 2 );
  mvvline( top + 1, left + cols - 1, ACS_VLINE, rows - 2 );
  mvhline( top, left + 1, ACS_HLINE, cols - 2 );
  mvhline( top + rows - 1, left + 1, ACS_HLINE, cols - 2 );
  mvaddch( top, left, ACS_ULCORNER );
  mvaddch( top, left + cols - 1, ACS_URCORNER );
  mvaddch( top + rows - 1, left, ACS_LLCORNER );
  mvaddch( top + rows - 1, left + cols - 1, ACS_LRCORNER );

} /* End of Draw_Box() */

/*------------------------------------------------------------------------*/

/*  Draw_Hline()
 *
 *  Draws a horizontal line in a window including Tees at ends
 */

  void
Draw_Hline( WINDOW *win, int top, int left, int right )
{
  mvwhline( win, top, left+1, ACS_HLINE, (right - left - 1) );
  mvwaddch( win, top, left,  ACS_LTEE );
  mvwaddch( win, top, right, ACS_RTEE );

} /* End of Draw_Hline() */

/*------------------------------------------------------------------------*/

/*  Draw_Vline()
 *
 *  Draws a vertical line in a window including Tees at ends
 */

  void
Draw_Vline( WINDOW *win, int top, int left, int bottom )
{
  mvwvline( win, top+1, left, ACS_VLINE, (bottom - top -1) );
  mvwaddch( win, top, left, ACS_TTEE );
  mvwaddch( win, bottom, left, ACS_BTEE );

} /* End of Draw_Vline() */

/*------------------------------------------------------------------------*/
