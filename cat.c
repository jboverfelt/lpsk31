/*  cat.c
 *
 *  Yaesu FT847 CAT control functions for the lpsk31 application
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

/* Runtime config data */
rc_data_t rc_data;

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

/*------------------------------------------------------------------------*/

/*  Read_Rx_Status()
 *
 *  Reads the transceiver's status (Tx/Rx freq, mode,
 *  po/s-meter etc) from the Yaesu FT-847 transceiver.
 */

  void
Read_Rx_Status( char *mode, char *freq, char *rst )
{
  /* CAT commands */
  unsigned char READ_RX_STATUS[] = { 0, 0, 0, 0, 0xE7 };
  unsigned char READ_MAIN_VFO[]  = { 0, 0, 0, 0, 0x03 };

  /* Mask for Rx S-Meter status */
  char S_METER = 0x1F;

  /* A 5-char string for sending and receiving */
  /* 5-byte strings to and from the transceiver */
  unsigned char cmnd_parm[5];

  int temp;

  bzero( cmnd_parm, 5 );

  if( isFlagSet(ENABLE_CAT) )
  {
	/* Read Rx status (S-meter) */
	Write_Tcvr_Command( READ_RX_STATUS );
	Read_Tcvr_Serial( cmnd_parm, 1 );
	temp = cmnd_parm[0] & S_METER;

	/* Keep S-meter report in range 3-9 */
	temp = temp/4 + 3;
	if( temp > 9 )
	  temp = 9;
	temp = 500 + 10*temp + 9;
	snprintf( rst, 4, "%3d", temp );

	/* Read Rx mode and frequency */
	Write_Tcvr_Command( READ_MAIN_VFO );
	Read_Tcvr_Serial( cmnd_parm, 5 );

	/* Decode frequency data to .1MHz */
	temp = 0;
	temp += (cmnd_parm[0] & 0xF0) >> 4;
	temp *= 10;
	temp += cmnd_parm[0] & 0x0F;
	temp *= 10;
	temp += (cmnd_parm[1] & 0xF0) >> 4;
	temp *= 10;
	temp += cmnd_parm[1] & 0x0F;

	/* If freq > 100, enter only MHz value */
	if( temp >= 1000 )
	{
	  snprintf( freq, 4, "%3d", temp/10 );
	  freq[3] = '.';
	  freq[4] = '\0';
	}
	else /* Enter to .1 Mhz */
	{
	  snprintf( freq, 4, "%3d", temp );
	  freq[3] = freq[2];
	  freq[2] = '.';
	  freq[4] = '\0';
	}

	/* Enter operating mode (LSB/USB etc) */
	*mode = cmnd_parm[4];

  } /* if( isFlagSet( ENABLE_CAT ) */

} /* End of Read_Rx_Status() */

/*------------------------------------------------------------------------*/

/* Serial port File descriptor */
static int tcvr_serial_fd = 0;

/* Original serial port options */
struct termios  tcvr_serial_old_options;


/*  Open_Tcvr_Serial()
 *
 *  Opens Tcvr's Serial Port device, returns the file
 *  descriptor tcvr_serial_fd on success or exits on error
 */

  void
Open_Tcvr_Serial( void )
{
  struct termios new_options; /* New serial port options */
  struct flock lockinfo;      /* File lock information   */

  /* Open Serial device, exit on error */
  tcvr_serial_fd = open( rc_data.serial, O_RDWR | O_NOCTTY );
  if( tcvr_serial_fd < 0 )
  {
	Cleanup();
	perror( rc_data.serial );
	exit(-1);
  }

  /* Attempt to lock entire Serial port device file */
  lockinfo.l_type   = F_WRLCK;
  lockinfo.l_whence = SEEK_SET;
  lockinfo.l_start  = 0;
  lockinfo.l_len    = 0;

  /* If Serial device is already locked, abort */
  if( fcntl( tcvr_serial_fd, F_SETLK, &lockinfo ) < 0 )
  {
	Cleanup();
	fcntl( tcvr_serial_fd, F_GETLK, &lockinfo );
	fprintf( stderr, "lpsk31: %s: Device locked by pid %d - aborting\n",
		rc_data.serial, lockinfo.l_pid );
	exit( -1 );
  }

  /* Save the current serial port options */
  tcgetattr( tcvr_serial_fd, &tcvr_serial_old_options );

  /* Read the current serial port options */
  tcgetattr( tcvr_serial_fd, &new_options );

  /* Set the i/o baud rates to 57600 */
  cfsetispeed( &new_options, B57600 );
  cfsetospeed( &new_options, B57600 );

  /* Set options for 'raw' I/O mode */
  new_options.c_iflag &=
	~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );
  new_options.c_oflag &= ~OPOST;
  new_options.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
  new_options.c_cflag &= ~( CSIZE | PARENB );
  new_options.c_cflag |= ( CS8 | CLOCAL | CREAD | CSTOPB );

  /* Setup read() timeout to .2 sec */
  new_options.c_cc[ VMIN ]  = 0;
  new_options.c_cc[ VTIME ] = 2;

  /* Setup the new options for the port */
  tcsetattr( tcvr_serial_fd, TCSAFLUSH, &new_options );

} /* End of Open_Tcvr_Serial() */

/*-------------------------------------------------------------------------*/

/*  Write_Tcvr_Command()
 *
 *  Writes a command (status change)
 *  to the Yaesu FT-847 Tranceiver
 */

  int
Write_Tcvr_Command( unsigned char *cmnd )
{
  if( isFlagSet(ENABLE_CAT) )
  {
	tcflush( tcvr_serial_fd, TCIOFLUSH );
	if( write( tcvr_serial_fd, cmnd, 5 ) < 5 ) /* Error condition */
	  return( -1 );

	/* Give time to CAT to do its thing */
	usleep(1000);
  }

  return( 0 );

} /* End of Write_Tcvr_Command() */

/*-------------------------------------------------------------------------*/

/*  Set_TCVR_Mode()
 *
 *  Sets the FT847 operating mode
 */

  void
Set_Tcvr_Mode( void )
{
  /* Set appropriate tcvr mode if CAT enabled */
  if( isFlagSet(ENABLE_CAT) )
  {
	if( isFlagSet(MODE_TRANSMIT) )
	{
	  if( isFlagSet(MODE_QPSK_LSB) )
		Write_Tcvr_Command( MODE_LSB );
	  else
		Write_Tcvr_Command( MODE_USB );

	  Write_Tcvr_Command( PTT_ON );
	}
	else
	{
	  if( isFlagSet(MODE_QPSK_LSB) )
		Write_Tcvr_Command( MODE_CWNR );
	  else
		Write_Tcvr_Command( MODE_CWN );

	  Write_Tcvr_Command( PTT_OFF );
	}

  } /* if( isFlagSet(ENABLE_CAT) ) */

} /* Set_Tcvr_Mode() */

/*-------------------------------------------------------------------------*/

/*  Read_Tcvr_Serial()
 *
 *  Reading Data from the Tcvr's Serial Port
 */

  int
Read_Tcvr_Serial( unsigned char *cmnd, int block_size )
{
  /* Error condition */
  if( read( tcvr_serial_fd, cmnd, block_size ) < block_size )
  {
	tcflush( tcvr_serial_fd, TCIOFLUSH );
	return( -1 );
  }
  else
	return( 0 );

} /* End of Read_Tcvr_Serial() */

/*-------------------------------------------------------------------------*/

/*  Close_Tcvr_Serial()
 *
 *  Restore old options and close the Serial port
 */

  void
Close_Tcvr_Serial( void )
{
  if( tcvr_serial_fd > 0 )
  {
	tcsetattr( tcvr_serial_fd, TCSANOW, &tcvr_serial_old_options );
	close( tcvr_serial_fd );
	tcvr_serial_fd = 0;
  }

} /* End of Close_Tcvr_Serial() */

/*-------------------------------------------------------------------------*/
