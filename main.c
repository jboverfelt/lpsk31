/*  lpsk31.c
 *
 *  Main body of lpsk31 application
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

#include "main.h"

FILE
  /* File for recording QSO's */
  *qso_record_fp = NULL,
  /* File for ADIF QSO record */
  *log_adif_fp = NULL,
  /* File for staion QSO log  */
  *log_fp = NULL;

char
  label[9][19], /* Labels for F1-F9 commands */
  *macro[9];    /* Macros attached to F1-F9  */

/* Runtime config data */
rc_data_t rc_data;

/* Operator and location data */
op_data_t op_data;

/* Signal handler */
static void sig_handler( int signal );

/*------------------------------------------------------------------------*/

  int
main( int argc, char *argv[] )
{
  /* Command line option returned by getopt() */
  int option;

  /* New and old actions for sigaction() */
  struct sigaction sa_new, sa_old;


  /* Initialize new actions */
  sa_new.sa_handler = sig_handler;
  sigemptyset( &sa_new.sa_mask );
  sa_new.sa_flags = 0;

  /* Register function to handle signals */
  sigaction( SIGINT,  &sa_new, &sa_old );
  sigaction( SIGSEGV, &sa_new, 0 );
  sigaction( SIGFPE,  &sa_new, 0 );
  sigaction( SIGTERM, &sa_new, 0 );
  sigaction( SIGABRT, &sa_new, 0 );

  /*** Process command line options ***/
  while( (option = getopt(argc, argv, "hv") ) != -1 )

	switch( option )
	{
	  case 'h': /* Print usage and exit */
		Usage();
		exit( 0 );

	  case 'v': /* Print version and exit */
		fprintf( stderr, "%s\n", VERSION );
		exit( 0 );

	  default: /* Print usage and exit */
		Usage();
		exit( -1 );

	} /* End of switch( option ) */

  /* Load runtime config file, aborts on error */
  Load_Config();

  /* Prepare sound card, aborts on error */
  Setup_Sound_Card();

  /* Initialize ncurses */
  initscr();
  keypad( stdscr, TRUE );
  nonl();
  cbreak();
  noecho();
  nodelay( stdscr, TRUE );
  curs_set( TRUE );
  Set_Flag( NCURSES_INIT );

  /* Assuming terminal has color! */
  if( has_colors() )
  {
	start_color();

	init_pair( COLOR_WHITE,        COLOR_WHITE,   COLOR_BLACK  );
	init_pair( COLOR_RED,		   COLOR_RED,     COLOR_BLACK  );
	init_pair( COLOR_GREEN,		   COLOR_GREEN,   COLOR_BLACK  );
	init_pair( COLOR_BLUE,		   COLOR_BLUE,    COLOR_BLACK  );
	init_pair( COLOR_YELLOW,	   COLOR_YELLOW,  COLOR_BLACK  );
	init_pair( COLOR_CYAN,		   COLOR_CYAN,    COLOR_BLACK  );
	init_pair( COLOR_MAGENTA,	   COLOR_MAGENTA, COLOR_BLACK  );
	init_pair( COLOR_YELLOW_BLUE,  COLOR_YELLOW,  COLOR_BLUE   );
	init_pair( COLOR_WHITE_BLUE,   COLOR_WHITE,   COLOR_BLUE   );
	init_pair( COLOR_WHITE_RED,    COLOR_WHITE,   COLOR_RED    );
	init_pair( COLOR_WHITE_YELLOW, COLOR_WHITE,   COLOR_YELLOW );
	init_pair( COLOR_WHITE_GREEN,  COLOR_WHITE,   COLOR_GREEN  );
  }

  /* Go to ncurses interface until user quits */
  Screen_Root();
  Screen_Receive_Window();

  /* Cleanup and exit */
  Cleanup();

  return(0);

} /* End of  main() */

/*------------------------------------------------------------------------*/

/*  Load_Config()
 *
 *  Loads the lpsk31rc configuration file
 */

  void
Load_Config( void )
{
  int
	lb_idx, /* Label buffer index */
	mc_idx, /* Macro buffer index */
	ln_idx, /* Line buffer index  */
	ln_len, /* Line buffer length */
	tg_cnt, /* Count '<' and '>'  */
	eof,    /* EOF flag */
	idx;

  char
	rc_fpath[4096], /* File path to lpsk31rc */
	line[81];       /* Buffer for Load_Line  */

  const char *home;

  /* Config file pointer */
  FILE *lpsk31rc;


  /* Setup file path to lpsk31rc */
  home = getenv( "HOME" );
  if( home == NULL || home[0] == '\0' )
  {
	fprintf( stderr, "lpsk31: HOME is not set\n" );
	exit( -1 );
  }
  if( snprintf(rc_fpath, sizeof rc_fpath, "%s/lpsk31/lpsk31rc", home) >=
	  (int)sizeof rc_fpath )
  {
	fprintf( stderr, "lpsk31: configuration path is too long\n" );
	exit( -1 );
  }

  /* Open lpsk31rc file */
  lpsk31rc = fopen( rc_fpath, "r" );
  if( lpsk31rc == NULL )
  {
	perror( rc_fpath );
	fprintf( stderr,
		 "Run 'make install-config', then edit %s for your station and sound card.\n",
		 rc_fpath );
	exit( -1 );
  }

  /*** Read runtime configuration data ***/

  /* Read ALSA sound card name, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( rc_data.snd_card, line, 31 );
  rc_data.snd_card[31] = '\0';

  /* Read Capture source, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( rc_data.cap_src, line, 31 );
  rc_data.cap_src[31] = '\0';

  /* Read Capture volume element, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( rc_data.cap_vol, line, 31 );
  rc_data.cap_vol[31] = '\0';

  /* Read Master volume element, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( rc_data.mst_vol, line, 31 );
  rc_data.mst_vol[31] = '\0';

  /* Read PCM volume element, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( rc_data.pcm_vol, line, 31 );
  rc_data.pcm_vol[31] = '\0';

  /* Read Capture level, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.cap_lev = atoi( line );

  /* Read Playback level, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.pbk_lev = atoi( line );

  /* Read ALSA channel used, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.channel = atoi( line );

  /* Read number of channels (stereo/mono), abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.num_chn = atoi( line );

  /* Read channel used (left/right), abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.use_chn = atoi( line );

  /* Read squelch threshold, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.sqlch_thr = atoi( line );

  /* Read tone duration, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.tone_dur = atoi( line );

  /* Read number of reversals, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -13 );
  rc_data.num_rev = atoi( line );

  /* Read CW encoder speed, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.wpm = atoi( line );

  /* Read word wrap column, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  rc_data.wwrap = atoi( line );

  /* Read serial port device, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( rc_data.serial, line, 14 );
  rc_data.serial[14] = '\0';

  /* Read operator callsign, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( op_data.call, line, 16 );
  op_data.call[16] = '\0';

  /* Read operator name, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( op_data.name, line, 16 );
  op_data.name[16] = '\0';

  /* Read QTH, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( op_data.qth, line, 20 );
  op_data.qth[20] = '\0';

  /* Read QTH locator, abort if EOF */
  if( Load_Line(line, lpsk31rc) == EOF )
	Abort_On_Error( -12 );
  strncpy( op_data.loc, line, 6 );
  op_data.loc[6] = '\0';

  /* Null buffer pointers just in case */
  for( idx = 0; idx < NUM_OF_LABELS; idx++ )
	macro[idx] = NULL;

  /*** Read macro labels from rc file ***/
  if( Load_Line(line, lpsk31rc) != EOF )
	for( lb_idx = 0; lb_idx < NUM_OF_LABELS; lb_idx++ )
	{
	  /* Abort if first line is not a label */
	  if( line[0] != '[' )
		Abort_On_Error( -12 );

	  /* Look for closing ']' */
	  ln_idx = 1;
	  while( (ln_idx < 18) && (line[ln_idx] != ']') )
		ln_idx++;

	  /* Abort if label string > 16 char + [] */
	  if( ln_idx > 17 )
		Abort_On_Error( -12 );

	  /* Enter label string if all OK */
	  memcpy( label[lb_idx], &line[1], ln_idx - 1 );
	  label[lb_idx][ln_idx - 1] = '\0';

	  /*** Read macros ***/
	  mc_idx = tg_cnt = 0;
	  do
	  {
		/* Stop reading lines at EOF */
		eof = Load_Line( line, lpsk31rc );

		/* Read labels if line starts with '[' */
		if( line[0] == '[' )
		  break;

		/* Allocate memory to macro buffer */
		ln_len = strlen( line );
		macro[lb_idx] = (char *)realloc(macro[lb_idx], ln_len + mc_idx + 2);
		if( macro[lb_idx] == NULL )
		{
		  perror( "lpsk31" );
		  fprintf( stderr, "lpsk31: memory re-allocation failed - aborting\n" );
		  exit( -1 );
		}

		/* Concatenate lines to macro */
		for( ln_idx = 0; ln_idx < ln_len; ln_idx++ )
		{
		  macro[lb_idx][mc_idx++] = line[ln_idx];

		  /* Count tag delimiters */
		  if( (line[ln_idx] == '<') || (line[ln_idx] == '>') )
			tg_cnt++;
		}

		/* Terminate Macro with LF and null */
		macro[lb_idx][mc_idx++] = LF;
		macro[lb_idx][mc_idx] = '\0';

	  } /* do */
	  while( eof != EOF );

	  /* Abort if tag delimiters are odd number */
	  if( tg_cnt & 1 )
		Abort_On_Error( -12 );

	} /* for( lb_idx = 0; lb_idx < NUM_OF_LABELS; lb_idx++ ) */

  /* Duration of half of a PSK pulse, in DSP samples */
  rc_data.psk_elem  = (int)( (double)DSP_RATE / BAUD_RATE );
  rc_data.psk_elem2 = rc_data.psk_elem/2;

  /* Duration of preamble tone in dsp samples */
  rc_data.tone_period = DSP_RATE / AUDIO_FREQUENCY;

  /* Make cosine & sine wave table */
  Make_Cosine_Table();
  Make_Sine_Table();

  fclose( lpsk31rc );

} /* End of Load_Config */

/*------------------------------------------------------------------*/


/*  Load_Line()
 *
 *  Loads a line from a file, aborts on failure. Lines beginning
 *  with a '#' are ignored as comments. At the end of file EOF is
 *  returned. Lines assumed maximum 80 characters long.
 */

  int
Load_Line( char *buff, FILE *pfile )
{
  int
	num_chr, /* Number of characters read, excluding lf/cr */
	chr;     /* Character read by getc */

  num_chr = 0;

  /* Clear buffer at start */
  buff[0] = '\0';

  /* Ignore commented lines, white spaces and eol/cr */
  if( (chr = fgetc(pfile)) == EOF )
	return( EOF );

  while(
	  (chr=='#') ||
	  (chr==' ') ||
	  (chr==CR ) ||
	  (chr==LF ) )
  {
	/* Go to the end of line (look for LF or CR) */
	while( (chr != CR) &&
		(chr != LF) )
	  if( (chr = fgetc(pfile)) == EOF )
		return( EOF );

	/* Dump any CR/LF remaining */
	while( (chr == CR) ||
		(chr == LF) )
	  if( (chr = fgetc(pfile)) == EOF )
		return( EOF );

  } /* End of while( (chr == '#') || ... */

  while( num_chr < 80 )
  {
	/* If LF/CR reached before filling buffer, return */
	if( (chr == LF) ||
		(chr == CR) )
	  break;

	buff[num_chr++] = chr;

	/* Terminate buffer on EOF */
	if( (chr = fgetc(pfile)) == EOF )
	{
	  buff[num_chr] = '\0';
	  return( EOF );
	}

	/* Abort if end of line not reached at 80 char. */
	if( (num_chr == 80) &&
		(chr != LF) 	&&
		(chr != CR) )
	  Abort_On_Error( -12 );

  } /* End of while( num_chr < max_chr ) */

  /* Terminate buffer as a string */
  buff[num_chr] = '\0';

  return( 0 );

} /* End of Load_Line() */

/*------------------------------------------------------------------*/

/*  Usage()
 *
 *  Prints usage information
 */

  void
Usage( void )
{
  fprintf( stderr, "%s\n\n",
	  "Usage: lpsk31 [-hv]" );

  fprintf( stderr, "%s\n\n",
	  "       -h: Print this usage information and exit.");

  fprintf( stderr, "%s\n\n",
	  "       -v: Print version number and exit.");

} /* End of Usage() */

/*------------------------------------------------------------------------*/

/*  Cleanup()
 *
 *  Cleans up screen and terminates ncurses
 */

  void
Cleanup( void )
{
  /* Cleanup screen and terminate ncurses */
  if( isFlagSet(NCURSES_INIT) )
  {
	clear();
	refresh();
	endwin();
  }

  /* Close open files */
  if( qso_record_fp != NULL )
	fclose( qso_record_fp );
  if( log_adif_fp != NULL )
	fclose( log_adif_fp );
  if( log_fp != NULL )
	fclose( log_fp );

}/*  Cleanup( void ) */

/*------------------------------------------------------------------------*/

/*  sig_handler()
 *
 *  Signal Action Handler function
 */

static void sig_handler( int signal )
{
  /* Cleanup and abort */
  Cleanup();

  fprintf( stderr, "\n" );
  switch( signal )
  {
	case SIGINT :
	  fprintf( stderr, "%s\n", "lpsk31: Exiting via User Interrupt" );
	  exit( signal );

	case SIGSEGV :
	  fprintf( stderr, "%s\n", "lpsk31: Segmentation Fault" );
	  exit( signal );

	case SIGFPE :
	  fprintf( stderr, "%s\n", "lpsk31: Floating Point Exception" );
	  exit( signal );

	case SIGTERM :
	  fprintf( stderr, "%s\n", "lpsk31: Termination Request received" );
	  exit( signal );

	case SIGABRT :
	  fprintf( stderr, "%s\n", "lpsk31: Abort Signal received" );
	  exit( signal );

  } /* switch( signal ) */

} /* End of sig_handler() */

/*------------------------------------------------------------------------*/

/*  Abort_On_Error()
 *
 *  Prints an error message and exits
 */

  void
Abort_On_Error( int why )
{
  /* Cleanup and abort */
  Cleanup();

  switch( why )
  {
	case -1 : /* Abort if samples buffer cannot be filled */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Error filling signal samples buffer - aborting" );
	  break;

	case -2 : /* Abort if recording source selection fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Recording source selection failed - aborting" );
	  break;

	case -3 : /* Abort sample format setting fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Sample format setting failed - aborting" );
	  break;

	case -4 : /* Abort if sampling speed setting fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: DSP sampling speed setting failed - aborting" );
	  break;

	case -5 : /* Abort if recording level setting fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Recording level setting failed - aborting" );
	  break;

	case -6 : /* Abort if samples buffer cannot be filled */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Recording buffer not filled by read() - aborting" );
	  break;

	case -7 : /* Abort if no recording level device found */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Recording level device not found - aborting" );
	  break;

	case -8 : /* Abort if Line level setting fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Recording source level setting failed - aborting" );
	  break;

	case -9 : /* Abort if Stereo/Mono mode setting fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Stereo/Mono mode setting failed - aborting" );
	  break;

	case -10 : /* Abort if DSP samples cannot be written to file */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Error writing DSP samples buffer to file - aborting" );
	  break;

	case -11 : /* Abort if audio samples cannot be written to DSP */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Error writing audio samples buffer to DSP - aborting" );
	  break;

	case -12 : /* Abort if error reading the configuration file */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Error reading ~/lpsk31/lpsk31rc - aborting" );
	  break;

	case -13 : /* Abort if malloc() for samples buffer fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: A call for memory allocation failed - aborting" );
	  break;

	case -14 : /* Abort if a tag is not identified */
	  fprintf( stderr, "%s\n",
		  "lpsk31: Unknown tag in macro  - aborting" );
	  break;

	case -15 : /* Abort if setting PCM or Volume level fails */
	  fprintf( stderr, "%s\n",
		  "lpsk31: PCM or Volume level setting failed - aborting" );

  }  /* switch( why ) */

  exit( why );

} /* End of Abort_On_Error() */

/*------------------------------------------------------------------------*/

/* Functions for testing and setting/clearing flow control flags
 *
 *  See lpsk31.h for definition of flow control flags
 */

/* An int variable holding the single-bit flags */
static int Flags = 0;

  int
isFlagSet( int flag )
{
  return( (Flags & flag) == flag );
}

  int
isFlagClear( int flag )
{
  return( (~Flags & flag) == flag );
}

  void
Set_Flag( int flag )
{
  Flags |= flag;
}

  void
Clear_Flag( int flag )
{
  Flags &= ~flag;
}

/*------------------------------------------------------------------------*/
