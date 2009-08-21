/*  sound.c
 *
 *  Soundcard handling functions of lpsk31 application
 */

/*
 *  lpsk31: An application to transmit and receive
 *  PSK31 signals using a computer's sound card
 *
 *  Copyright (C) 2002  Neoklis Kyriazis
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

int
  dsp_out_fd, /* File descriptor of output DSP device */
  dsp_in_fd,  /* File descriptor of input DSP device  */
  mix_fd;     /* File descriptor of mixer device      */

extern int
  num_chan,    /* Stereo(=2) or Mono(=1) selection */
  chan_used,   /* Channel in use: left=0, right=1  */
  buffer_size, /* DSP signal samples buffer        */
  buffer_idx;  /* Index to signal samples buffer   */

/* Signal/DSP samples buffer. Double size for Stereo */
extern unsigned char *samples_buffer;

extern rc_data_t rc_data;

/* Pointer to samples file */
extern FILE *samples_fp;

/*------------------------------------------------------------------------*/

/*  Setup_Sound_Card()
 *
 *  Sets up mixer and DSP devices
 */

  void
Setup_Sound_Card( void )
{
  int
	rec_src,    /* New recording source (RECORD_SRC) */
	level,      /* Rec/Volume level request to mixer */
	itmp;

  /* Mixer device to be set-up (input or reclev) */
  unsigned int device;

  /* Sound device names from souncard.h */
  char *dev_name[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;

  /* File lock information */
  struct flock lockinfo;


  /*** Open and lock mixer device ***/
  /* Open mixer device, abort on error */
  if( (mix_fd = open(rc_data.mixer, O_RDWR, 0)) == -1 )
  {
	Cleanup();
	perror( rc_data.mixer );
	exit( -1 );
  }

  /* Attempt to lock entire mixer device file */
  lockinfo.l_type   = F_WRLCK;
  lockinfo.l_whence = SEEK_SET;
  lockinfo.l_start  = 0;
  lockinfo.l_len    = 0;
  lockinfo.l_pid    = 0;

  /* If mixer device is already locked, abort */
  if( fcntl( mix_fd, F_SETLK, &lockinfo ) < 0 )
  {
	Cleanup();
	perror( rc_data.mixer );
	fcntl( mix_fd, F_GETLK, &lockinfo );
	fprintf( stderr, "lpsk31: Lock %s: Device locked by pid %d - aborting\n",
		rc_data.mixer, lockinfo.l_pid );
	exit( -1 );
  }

  /*** Setup sound card parameters ***/
  /* Find number of recording source matching source name */
  for( rec_src = 0; rec_src < SOUND_MIXER_NRDEVICES; rec_src++ )
	if( strcmp( dev_name[rec_src], rc_data.rec_src ) == 0 )
	  break;

  if( rec_src ==  SOUND_MIXER_NRDEVICES )
	Abort_On_Error( -2 );

  /* Make recording source mask */
  itmp = 1 << rec_src;

  /* Attempt to select recording source, abort on error */
  if( ioctl(mix_fd, SOUND_MIXER_WRITE_RECSRC, &itmp) == -1 )
  {
	Cleanup();
	perror("lpsk31: SOUND_MIXER_WRITE_RECSRC");
	exit( -1 );
  }
  if( itmp != 1 << rec_src )
	Abort_On_Error( -2 );

  /* Zero level of recording source to silence (my) speakers */
  level = itmp = 0;
  if( ioctl(mix_fd, SOUND_MIXER_WRITE_LINE, &level) == -1 )
  {
	perror("lpsk31: Setting Recording Source level");
	exit( -1 );
  }
  if( level != itmp )
	Abort_On_Error( -8 );

  /* Find recording level device */
  for( device = 0; device < SOUND_MIXER_NRDEVICES; device++ )
	if( strcmp( dev_name[device], rc_data.lev_dev ) == 0 )
	  break;

  if( device ==  SOUND_MIXER_NRDEVICES )
	Abort_On_Error( -7 );

  /* Write recording level to mixer, abort on error */
  level = itmp = rc_data.rec_lev | (rc_data.rec_lev << 8);
  if( ioctl(mix_fd, SOUND_MIXER_WRITE_IGAIN, &level) == -1 )
  {
	Cleanup();
	perror("lpsk31: Setting Recording level");
	exit( -1 );
  }
  if( level != itmp )
	Abort_On_Error( -5 );

  /* Write level to Volume, abort on error */
  level = itmp = rc_data.vol_lev | (rc_data.vol_lev << 8);
  if( ioctl(mix_fd, SOUND_MIXER_WRITE_VOLUME, &level) == -1 )
  {
	Cleanup();
	perror("lpsk31: Setting Volume level");
	exit( -1 );
  }
  if( level != itmp )
	Abort_On_Error( -15 );

  /* Write level to PCM, abort on error */
  level = itmp = rc_data.vol_lev | (rc_data.vol_lev << 8);
  if( ioctl(mix_fd, SOUND_MIXER_WRITE_PCM, &level) == -1 )
  {
	Cleanup();
	perror("lpsk31: Setting PCM level");
	exit( -1 );
  }
  if( level != itmp )
	Abort_On_Error( -15 );

  /*** Open and setup dsp device(s) ***/
  /* If only one device specified, open input dsp r/w */
  if( strcmp(rc_data.dsp_in, rc_data.dsp_out) == 0 )
  {
	Setup_Dsp_Dev(rc_data.dsp_in, O_RDWR, &dsp_in_fd);
	dsp_out_fd = dsp_in_fd;
  }
  else /* Setup separate dsp in/out */
  {
	Setup_Dsp_Dev(rc_data.dsp_in,  O_RDONLY, &dsp_in_fd );
	Setup_Dsp_Dev(rc_data.dsp_out, O_WRONLY, &dsp_out_fd);
  }

} /* End of Setup_Sound_Card() */

/*------------------------------------------------------------------------*/

/*  Setup_Dsp_Dev()
 *
 *  Sets up a given dsp device
 */

  void
Setup_Dsp_Dev( char *dsp_dev, int mode, int *dsp_fd )
{
  int
	dsp_speed,   /* DSP sampling speed as in lpsk31.h */
	frag_size,   /* DSP buffer (fragment) size in kb  */
	sample_fmt,  /* DSP sample format, 8-bit unsigned */
	itmp;

  /* File lock information   */
  struct flock lockinfo;


  /* Open input DSP device, abort on error */
  if( (*dsp_fd = open(dsp_dev, mode, 0)) == -1 )
  {
	Cleanup();
	perror( dsp_dev );
	fprintf( stderr, "lpsk31: Unable to open %s", dsp_dev);
	exit(-1);
  }

  /* Attempt to lock entire DSP device file */
  if( mode == O_RDONLY )
  {
	lockinfo.l_type   = F_RDLCK;
	lockinfo.l_whence = SEEK_SET;
	lockinfo.l_start  = 0;
	lockinfo.l_len    = 0;
  }
  else
	if( mode == O_WRONLY )
	{
	  lockinfo.l_type   = F_WRLCK;
	  lockinfo.l_whence = SEEK_SET;
	  lockinfo.l_start  = 0;
	  lockinfo.l_len    = 0;
	}
	else
	{
	  lockinfo.l_type   = F_RDLCK | F_WRLCK;
	  lockinfo.l_whence = SEEK_SET;
	  lockinfo.l_start  = 0;
	  lockinfo.l_len    = 0;
	}


  /* If DSP device is already locked, abort */
  if( fcntl( *dsp_fd, F_SETLK, &lockinfo ) < 0 )
  {
	Cleanup();
	perror( dsp_dev );
	fcntl( *dsp_fd, F_GETLK, &lockinfo );
	fprintf( stderr, "lpsk31: Lock %s: Device is locked by pid %d",
		dsp_dev, lockinfo.l_pid );
	exit(-1);
  }

  /* Set fragment size according to stereo/mono mode */
  itmp = 1; frag_size = 0;
  while( (itmp << frag_size) != BUFFER_SIZE ) frag_size++;
  itmp += num_chan << 18;
  ioctl( *dsp_fd, SNDCTL_DSP_SETFRAGMENT, &frag_size );

  /* Set sample format to unsigned 8-bit, abort on error */
  sample_fmt = AFMT_U8;
  if( ioctl(*dsp_fd, SNDCTL_DSP_SETFMT, &sample_fmt) == -1 )
  {
	Cleanup();
	perror( "lpsk31: SNDCTL_DSP_SETFMT");
	exit(-1);
  }
  if( sample_fmt != AFMT_U8 )
  {
	Cleanup();
	fprintf( stderr, "Failed to set dsp sample format: AFMT_U8" );
	exit(-1);
  }

  /* Set stereo/mono mode */
  itmp = num_chan;
  if( ioctl(*dsp_fd, SNDCTL_DSP_CHANNELS, &itmp) == -1 )
	perror("lpsk31: SNDCTL_DSP_CHANNELS");

  /* Setup DSP sampling speed, abort on error */
  dsp_speed = DSP_SPEED;
  if( ioctl(*dsp_fd, SNDCTL_DSP_SPEED, &dsp_speed) == -1 )
  {
	Cleanup();
	perror( "lpsk31: SNDCTL_DSP_SPEED");
	exit(-1);
  }
  if( dsp_speed != DSP_SPEED )
  {
	Cleanup();
	fprintf( stderr, "Failed to set DSP speed to %d.", DSP_SPEED );
	exit(-1);
  }

} /* Setup_Dsp_Dev */

/*------------------------------------------------------------------------*/

/*  Get_Signal_Sample()
 *
 *  Gets the next DSP sample of the signal input.
 */

  int
Get_Signal_Sample(void)
{
  int
	sample_val, /* Signal sample value */
	sample_cnt; /* Number of samples read from DSP */

  /* Refill DSP samples buffer when needed */
  if( buffer_idx >= buffer_size )
  {
	/* Start buffer index according to stereo/mono mode */
	if( num_chan == 2 ) /* Stereo */
	  buffer_idx = chan_used;
	else
	  buffer_idx = 0;

	/* Read audio samples from file */
	if( isFlagSet(READ_FILE) )
	{
	  /* Fill in the samples buffer from file, return -1 on error */
	  if( (sample_cnt = fread( samples_buffer, 1,
			  buffer_size, samples_fp )) == -1 )
	  {
		perror( "lpsk31: fread()" );
		Cleanup();
		exit( -1 );
	  }
	  if( sample_cnt != buffer_size )
	  {
		Clear_Flag( READ_FILE );
		fclose( samples_fp );
		samples_fp = NULL;
	  }

	} /* if( isFlagSet(READ_FILE) ) */
	else
	{
	  /* Read audio samples from DSP, abort on error */
	  if( (sample_cnt = read(dsp_in_fd, samples_buffer, buffer_size)) == -1 )
	  {
		perror( "lpsk31: dsp read()" );
		exit(-1);
	  }
	  if( sample_cnt != buffer_size )
		Abort_On_Error( -6 );

	} /* else */

  } /* End of if( buffer_idx == buffer_size ) */

  /* Remove neutral value (128) from samples */
  sample_val = samples_buffer[buffer_idx] - 128;

  /* Increment according to mono/stereo mode */
  buffer_idx += num_chan;

  return( sample_val );

} /* End of Get_Signal_Sample() */

/*------------------------------------------------------------------------*/

/*  DSP_Write()
 *
 *  Write a samples buffer to DSP
 */

  void
DSP_Write( unsigned char *buffer, int buff_size )
{
  int sample_cnt;

  /*** Write samples buffer to DSP or file ***/
  if( (sample_cnt = write(dsp_out_fd, buffer, buff_size)) == -1 )
  {
	perror( "lpsk31: dsp read()" );
	exit(-1);
  }
  if( sample_cnt != buff_size )
	Abort_On_Error( -11 );

  /* Write samples to file */
  if( isFlagSet(WRITE_FILE) )
  {
	if( (sample_cnt = fwrite(
			samples_buffer, 1, buff_size, samples_fp )) == -1 )
	{
	  perror( ": fwrite()" );
	  exit(-1);
	}
	if( (sample_cnt != buff_size) )
	  Abort_On_Error(-10);

  } /* else of: if( isFlagClear(WRITE_FILE) ) */

} /* DSP_Write() */

/*------------------------------------------------------------------------*/

  void
Record_Signal( void )
{
  /* Count of samples returned by read()/write() */
  ssize_t sample_cnt;


  /* Create a file for writing samples */
  samples_fp = fopen( "lpsk31.bin", "w+" );
  if( samples_fp == NULL )
  {
	perror( "lpsk31.bin" );
	exit(-1);
  }

  /* Stop on Escape */
  while( getch() != 's' )
  {
	/* Fill in the samples buffer from DSP, return -1 on error */
	if( (sample_cnt = read(dsp_in_fd, samples_buffer, buffer_size)) == -1 )
	{
	  perror( "lpsk31: read()" );
	  exit(-1);
	}
	if( sample_cnt != buffer_size )
	  Abort_On_Error(-1);

	/* Write to file */
	if( (sample_cnt = fwrite(samples_buffer, 1, buffer_size, samples_fp )) == -1 )
	{
	  perror( ": fwrite()" );
	  exit(-1);
	}
	if( (sample_cnt != buffer_size) )
	  Abort_On_Error(-10);

  } /* while( getch() != KEY_ESC ) */

  fclose( samples_fp );
  samples_fp = NULL;

  return;

} /* End of Record_Signal() */

/*------------------------------------------------------------------------*/
