/*  pskcodec.c
 *
 *  PSK31 coder/decoder functions of lpsk31 application
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

/* Pointer to samples file */
extern FILE *samples_fp;

/* Index to sine wave lookup table */
int sine_idx;

extern int
  num_chan,    /* Stereo(=2) or Mono(=1) selection */
  chan_used,   /* Channel in use: left=0, right=1  */
  buffer_size; /* DSP signal samples buffer        */

/* Signal/DSP samples buffer. 2x size for Stereo */
extern unsigned char *samples_buffer;

/* File for recording QSO's */
extern FILE *qso_record_fp;

/* Runtime configuration data */
extern rc_data_t rc_data;

/* Varicode alphabet in HEX */
int varicode_table[NUM_VARICODE_CHARS] = VARICODE_ALPHABET;


/*------------------------------------------------------------------------*/

/*  Varicode_Bit()
 *
 *  Returns Varicode bits decoded from carrier phase shifts
 */

  int
Varicode_Bit( void )
{
  /* Carrier phase changes */
  int phase_chg;


  /* Get phase PSK31 carrier phase change */
  phase_chg = PSK31_Phase();

  /* If mode is BPSK, a phase change of */
  /* 100-180 deg is considered reversal */
  if( isFlagSet(MODE_BPSK) )
  {
	if( (abs(phase_chg) > 100) &&
		(abs(phase_chg) < 260) )
	  return(0);
	else
	  return(1);
  }
  else /* If in QPSK mode */
	return( Viterbi_Decoder(phase_chg) );

} /* Varicode_Bit( void ) */

/*------------------------------------------------------------------------*/

/*  Decode_PSK_Character()
 *
 *  Decodes a PSK31-coded character from
 *  the sequence of phase reversals in PSK31
 *  signal, producing an Ascii equivalent
 */

  int
Decode_PSK_Character( void )
{
  int
	varicode_bit, /* One bit of a Varicode character */
	ascii_char;   /* New decoded ASCII character     */

  /* Assembles varicode character */
  static int varicode = 0;


  /* Get next Varicode bit */
  varicode_bit = Varicode_Bit();

  /* Enter new varicode bit */
  varicode <<= 1;
  varicode |= varicode_bit;

  /* Return char on two consecutive 0's (char space) */
  if( (varicode & 0x03) == 0 )
  {
	/* Dump trailing 0's of SPACE */
	varicode >>= 2;

	/* Convert varicode to ascii */
	/* Look up varicode hex code in table */
	for( ascii_char = 0; ascii_char < NUM_VARICODE_CHARS; ascii_char++ )
	  if( varicode_table[ascii_char] == varicode )
		break;

	varicode = 0;

	/* Change CR to LF, seems needed */
	if( ascii_char == CR )
	  ascii_char = LF;

	return( ascii_char );

  } /* if( (varicode & 0x03) == 0 ) */
  else
	return( NO_CHARACTER );

} /* Decode_PSK_Character() */

/*------------------------------------------------------------------------*/

/*
 *  Transmit_Preamble()
 *
 *  Transmits a steady tone and then
 *  reversals as the PSK31 pre-amble
 */

  void
Transmit_Preamble( void )
{
  /* Samples buffer. Size is 64 * (tone_duration / 2) */
  /* as we have 64 samples/cycle, tone_duration is in */
  /* msec, and each audio cycle is 2 msec (F=500 Hz), */
  /* + 512 samples for the rising or falling edges.   */
  /* In stereo mode the buffer size must be doubled   */
  unsigned char
	samples_buff[(32 * rc_data.tone_dur + 512) * num_chan];

  int
	duration,   /* Tone duration in samples of audio */
	buff_idx,   /* Index to audio samples buffer     */
	buff_size,  /* Buffer size to be written to DSP  */
	cosine_idx, /* Index to cosine wave lookup table */
	idx;

  /* sine and cosine lookup tables */
  char  sine_table[64]  = SINE_WAVE_TABLE;
  int cosine_table[512] = COSINE_WAVE_TABLE;

  /*** Transmit a steady tone for rc_data.tone_dur msec ***/
  /* Tone duration in samples */
  duration  = 32 * rc_data.tone_dur;

  /* Add number of edge samples (*2 for stereo) */
  buff_size = (duration + 512) * num_chan;

  /* Clean up buffer */
  for( buff_idx = 0; buff_idx < buff_size; buff_idx++ )
	samples_buff[buff_idx] = 128;

  /* Initialize buffer according to stereo/mono mode */
  if( num_chan == 2 )
	buff_idx = chan_used;
  else
	buff_idx = 0;

  /* Fill buffer with cosine shaped rising edge */
  sine_idx = 0;
  for( cosine_idx = 511; cosine_idx >= 0; cosine_idx-- )
  {
	samples_buff[buff_idx] = (cosine_table[cosine_idx] *
		sine_table[sine_idx] / 65536) + 128;
	buff_idx += num_chan;

	if( ++sine_idx == 64 )
	  sine_idx = 0;
  }

  /* Fill buffer with the steady tone samples */
  for( idx = 0; idx < duration; idx++ )
  {
	samples_buff[buff_idx] = sine_table[sine_idx] + 128;
	buff_idx += num_chan;

	if( ++sine_idx == 64 )
	  sine_idx = 0;
  }

  /*** Write samples buffer to DSP ***/
  DSP_Write( samples_buff, buff_size );

  /*** Transmit rc_data.num_rev reversals ***/
  for( idx = 0; idx < rc_data.num_rev; idx++ )
	Transmit_Element( PHASE_SHIFT_180 );

} /* Transmit_Preamble() */

/*------------------------------------------------------------------------*/

/*  Transmit_Postamble()
 *
 *  Transmits reversals followed by steady tone
 *  for the tail end of a PSK 31 transmission
 */

  void
Transmit_Postamble( void )
{
  /* Samples buffer. Size is 64 * (tone_duration / 2)  */
  /* as we have 64 samples/cycle, tone_duration is in  */
  /* msec, and each audio cycle is 2 msec (F = 500 Hz) */
  /* + 512 samples for the rising 0r falling edges.    */
  /* In stereo mode the buffer size must be doubled   */
  unsigned char
	samples_buff[(32 * rc_data.tone_dur + 512) * num_chan];

  int
	duration,   /* Tone duration in samples of audio */
	buff_idx,   /* Index to audio samples buffer     */
	buff_size,  /* Buffer size to be written to DSP  */
	cosine_idx, /* Index to cosine wave lookup table */
	idx;

  /* sine and cosine lookup tables */
  char  sine_table[64]  = SINE_WAVE_TABLE;
  int cosine_table[512] = COSINE_WAVE_TABLE;


  /*** Transmit rc_data.num_rev reversals ***/
  for( idx = 0; idx < rc_data.num_rev; idx++ )
	Transmit_Element( PHASE_SHIFT_180 );

  /*** Transmit a steady tone for rc_data.tone_dur msec ***/
  /* Tone duration in samples */
  duration  = 32 * rc_data.tone_dur;

  /* Add number of edge samples (*2 for stereo) */
  buff_size = (duration + 512) * num_chan;

  /* Clean up buffer */
  for( buff_idx = 0; buff_idx < buff_size; buff_idx++ )
	samples_buff[buff_idx] = 128;

  /* Initialize buffer according to stereo/mono mode */
  if( num_chan == 2 )
	buff_idx = chan_used;
  else
	buff_idx = 0;

  /* Fill buffer with the steady tone samples */
  for( idx = 0; idx < duration; idx++ )
  {
	samples_buff[buff_idx] = sine_table[sine_idx] + 128;
	buff_idx += num_chan;

	if( ++sine_idx == 64 )
	  sine_idx = 0;
  }

  /* Fill buffer with cosine shaped falling edge */
  for( cosine_idx = 0; cosine_idx < 512; cosine_idx++ )
  {
	samples_buff[buff_idx] = (cosine_table[cosine_idx] *
		sine_table[sine_idx] / 65536) + 128;
	buff_idx += num_chan;

	if( ++sine_idx == 64 )
	  sine_idx = 0;
  }

  /*** Write samples buffer to DSP ***/
  DSP_Write( samples_buff, buff_size );

} /* Transmit_Postamble() */

/*------------------------------------------------------------------------*/

/*  Transmit_Element()
 *
 *  Transmits a PSk31 element with the specified phase shift
 */

  void
Transmit_Element( int phase_shift )
{
  static int
	/* Number of audio samples per element */
	num_samples = (4*DSP_SPEED)/125,
	/* Period of audio signal in DSP samples */
	audio_period = DSP_SPEED/AUDIO_FREQUENCY;

  int
	buff_idx,   /* Index to audio samples buffer */
	cosine_idx, /* Index to cosine lookup table  */
	idx;

  /* sine and cosine lookup tables */
  char  sine_table[65]  = SINE_WAVE_TABLE;
  int cosine_table[512] = COSINE_WAVE_TABLE;


  /* Initialize buffer according to stereo/mono mode */
  if( num_chan == 2 )
	buff_idx = chan_used;
  else
	buff_idx = 0;

  /* Phase shift carrier as requested */
  switch( phase_shift )
  {
	case PHASE_SHIFT_0: /*** Send a steady tone ***/

	  /* Fill buffer with the steady tone samples */
	  for( idx = 0; idx < num_samples; idx++ )
	  {
		samples_buffer[buff_idx] = sine_table[sine_idx] + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

	  break;

	case PHASE_SHIFT_180: /*** Send a phase reversal ***/

	  /* Fill 1/2 buffer with cosine shaped falling edge */
	  for( cosine_idx = 0; cosine_idx < 512; cosine_idx++ )
	  {
		samples_buffer[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

	  /* Reverse phase by moving sine index by 1/2 cycle */
	  sine_idx += audio_period / 2;
	  if( sine_idx >= 64 )
		sine_idx -= audio_period;

	  /* Fill rest of buffer with cosine shaped rising edge */
	  for( cosine_idx = 511; cosine_idx >= 0; cosine_idx-- )
	  {
		samples_buffer[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

	  break;

	case PHASE_SHIFT_90: /*** Send a +90 degree phase shift */

	  /* Fill 1/2 buffer with cosine shaped falling edge */
	  for( cosine_idx = 0; cosine_idx < 512; cosine_idx++ )
	  {
		samples_buffer[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

	  /* Advance phase 90 deg by advancing sine index 1/4 cycle */
	  sine_idx += audio_period / 4;
	  if( sine_idx >= 64 )
		sine_idx -= audio_period;

	  /* Fill rest of buffer with cosine shaped rising edge */
	  for( cosine_idx = 511; cosine_idx >= 0; cosine_idx-- )
	  {
		samples_buffer[buff_idx] = ( cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536 ) + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

	  break;

	case PHASE_SHIFT_270: /*** Send a -90 degree phase shift */

	  /* Fill 1/2 buffer with cosine shaped falling edge */
	  for( cosine_idx = 0; cosine_idx < 512; cosine_idx++ )
	  {
		samples_buffer[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

	  /* Retard phase 90 deg by retarding sine index 1/4 cycle */
	  sine_idx -= audio_period / 4;
	  if( sine_idx < 0 )
		sine_idx += audio_period;

	  /* Fill rest of buffer with cosine shaped rising edge */
	  for( cosine_idx = 511; cosine_idx >= 0; cosine_idx-- )
	  {
		samples_buffer[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;

		if( ++sine_idx == 64 )
		  sine_idx = 0;
	  }

  } /* switch( phase_shift ) */

  /*** Write samples buffer to DSP ***/
  DSP_Write( samples_buffer, buffer_size );

} /* Transmit_Element() */

/*------------------------------------------------------------------------*/

  void
Transmit_PSK_Character( int ascii_chr )
{
  int
	varicode_chr, /* Varicode character in Hex   */
	zero_cnt,     /* Count of consecutive zeroes */
	phase_shift,  /* Required PSK31 phase shift  */
	idx;

  /* Phase shift mappings */
  static char conv_code[32] = CONVOLUTIONAL_CODE;

  /* QPSK encoder shift register */
  static int shift_reg = 0;


  /*** If ascii_chr is NULL_CHAR (='last character') flag ***/
  if( ascii_chr == NULL_CHAR )
  {
	/* In BPSK, just transmit reversals */
	if( isFlagSet(MODE_BPSK) )
	{
	  Transmit_Element( PHASE_SHIFT_180 );
	  return;
	}
	else
	{
	  /* Flush shift register */
	  for( idx = 0; idx < 5; idx++ )
	  {
		shift_reg <<= 1;

		/* Only need the last 5 bits */
		shift_reg &= 0x1f;

		/* Find required QPSK phase shift */
		phase_shift = conv_code[ shift_reg ];

		/* Reverse odd phases for LSB */
		if( (isFlagSet(MODE_QPSK_LSB)) && (phase_shift & 1) )
		  phase_shift = 4 - phase_shift;

		Transmit_Element( phase_shift );

	  } /* for( idx = 0; idx < 5; idx++ ) */

	} /* else of: if( isFlagSet(MODE_BPSK) ) */

  } /* if( ascii_chr == NULL_CHAR ) */
  else
  {
	/* Convert ASCII to Varicode */
	varicode_chr = varicode_table[ascii_chr];

	/* Find beginning of varicode character */
	/* including the trailing zeroes below  */
	idx = 1; zero_cnt = 0;
	while( zero_cnt < 2 )
	{
	  if( (varicode_chr & idx) == 0 )
		zero_cnt++;
	  else
		zero_cnt = 0;

	  idx <<= 1;
	}

	/* Add two trailing zeroes */
	varicode_chr <<= 2;

	/* Bring idx back to 1st '1' */
	idx >>= 1;

	/*** Transmit character in BPSK or QPSK ***/
	if( isFlagSet(MODE_BPSK) )
	{
	  /* Transmit varicode character bits as phase reversals */
	  while( idx )
	  {
		if( idx & varicode_chr ) /* A '1' bit */
		  Transmit_Element( PHASE_SHIFT_0 );
		else /* A '0' bit */
		  Transmit_Element( PHASE_SHIFT_180 );

		/* Go down the varicode character */
		idx >>= 1;
	  }
	} /* if( isFlagSet(MODE_BPSK) ) */
	else /* If QPSK */
	{
	  while( idx )
	  {
		shift_reg <<= 1;

		/* Enter bits from Varicode char to shift register */
		if( idx & varicode_chr ) /* A '1' bit */
		  shift_reg |= 1;
		else /* A '0' bit */
		  shift_reg |= 0;

		/* Only need the last 5 bits */
		shift_reg &= 0x1f;

		/* Find required QPSK phase shift */
		phase_shift = conv_code[ shift_reg ];

		/* Reverse odd phases for LSB */
		if( (isFlagSet(MODE_QPSK_LSB)) && (phase_shift & 1) )
		  phase_shift = 4 - phase_shift;

		Transmit_Element( phase_shift );

		/* Go down the varicode character */
		idx >>= 1;

	  } /* while( idx ) */

	} /* else of: if( isFlagSet(MODE_BPSK) ) */

  } /* else of: if( ascii_chr == NULL_CHAR ) */

} /* Transmit_PSK_Character() */

/*------------------------------------------------------------------------*/

/* Viterbi decoder trellis */
trellis_state_t trellis[16];

/* Convolutional code symbols */
int symbols[32];


/*  Viterbi_Decoder()
 *
 *  The Viterbi Decoder
 */

  int
Viterbi_Decoder( int phase_chg )
{
  int
	min_dist,  /* Tracks minimum distance in trellis */
	dists[32], /* Distance metrics for the Viterbi   */
	idx;

  /* Bit sequence esimates */
  long int ests[32];

  unsigned char
	select, /* Select smallest distance   */
	vote;   /* Vote counter for bit guess */


  /* Reverse phase for receiving on LSB */
  if( isFlagSet(MODE_QPSK_LSB) )
	phase_chg = 360 - phase_chg;

  min_dist = 65535;
  /* Calculate distances for all states & both current data values */
  for( idx = 0; idx < 32; idx++ )
  {
	/* Added distance = distance between rcvd phase and predicted symbol */
	dists[idx] = trellis[idx/2].distance + Distance(phase_chg, symbols[idx]);

	/* Keep track of the smallest distance */
	if( dists[idx] < min_dist )
	  min_dist = dists[idx];

	/* New bit sequence estimate */
	ests[idx] = (trellis[idx/2].estimate << 1) + (idx & 1);

  } /* for( idx = 0; idx < 32; idx++ ) */

  /* For each state in the new trellis array */
  for( idx = 0; idx < 16; idx++ )
  {
	/* Select lowest distance */
	if( dists[idx] < dists[16 + idx] )
	  select = 0;
	else
	  select = 16;

	/* Update excess distances */
	trellis[idx].distance = dists[select + idx] - min_dist;

	/* Keep the new estimate */
	trellis[idx].estimate = ests[select + idx];

  } /* for( idx = 0; idx < 16; idx++ ) */

  /* Take a vote of the 20th bits */
  vote = 0;
  for( idx = 0; idx < 16; idx++ )
	if( (trellis[idx].estimate & (1 << 20)) > 0 )
	  vote++;

  /* 'phase_chg' is used as a random number */
  if( vote == 8 )
	return( phase_chg & 1 );
  else
	return( vote < 8 );

} /* Viterbi_Decoder() */


/*------------------------------------------------------------------------*/

/*  Distance()
 *
 *  Distance meteric for the Viterbi decoder
 */

  int
Distance( int A, int B)
{
  int dist;

  /* Shortest distance around the circumference */
  /* (phase circle) between two points A and B  */
  dist = abs(A - B);
  if( dist > 180 )
	dist = 360 - dist;

  return( dist );

} /* Distance() */

/*------------------------------------------------------------------------*/

/*  Parity()
 *
 *  Returns odd (?) parity
 */

  unsigned char
Parity( unsigned char data )
{
  unsigned char count;

  count = 0;
  while( data > 0 )
  {
	if( data & 1 )
	  count++;
	data >>= 1;
  }

  return( count & 1 );

} /* Parity() */

/*------------------------------------------------------------------------*/

/*  Init_Viterbi()
 *
 *  Initializes the Viterbi decoder
 */

  void
Init_Viterbi( void )
{
  int dprime[16] = { 450, 540, 270, 360, 360, 270, 360, 450,
					 360, 270, 360, 270, 270, 180,  90,   0 };
  int idx;

  /* Generate the encode table */
  for( idx = 0; idx < 32; idx++ )
	symbols[idx] = 90*(2*Parity(idx & POLYNOM1)+Parity(idx & POLYNOM2) );

  for( idx = 0; idx < 16; idx++ )
  {
	trellis[idx].distance = dprime[idx];
	trellis[idx].estimate = 0xFFFFFFF0 + idx;
  }

} /* Init_Viterbi( void ) */

/*------------------------------------------------------------------------*/


/*  Morse_Transmit()
 *
 *  Trasnsmits a character string in Morse code
 */

  void
Morse_Transmit( char *mesg )
{
  /* DSP samples buffer.
   * Size is (samples/cycle)*(audio freq)*(dash duration)
   * Dash duration is abt 3/wpm seconds.
   * In stereo mode the buffer size must be doubled
   */
  unsigned char
	samples_buff[3*num_chan*(SAMPLES_PER_CYCLE*AUDIO_FREQUENCY/rc_data.wpm)];

  int
	mesg_len,   /* String length of message to send  */
	mesg_idx,   /* Index into the message string     */
	morse_chr,  /* Morse code character in hex equiv */
	morse_idx,  /* Index into the Morse Hex code     */
	elem_dur,   /* Duration in samples of Morse elem */
	dash_dur,   /* Duration in samples of Morse dash */
	tone_dur,   /* Duration in samples of trans tone */
	buff_idx,   /* Index to audio samples buffer     */
	buff_size,  /* Buffer size to be written to DSP  */
	cosine_idx, /* Index to cosine wave lookup table */
	idx;

  /* sine and cosine lookup tables */
  char  sine_table[] = SINE_WAVE_TABLE;
  int cosine_table[] = COSINE_WAVE_TABLE;

  /* Tables of Morse and ascii characters */
  int
	ascii_char[] = ASCII_CHAR,
	morse_code[] = MORSE_CODE;

  /* Duration in samples of Morse elem */
  elem_dur = (SAMPLES_PER_CYCLE*AUDIO_FREQUENCY) / rc_data.wpm;
  dash_dur = 3 * elem_dur;
  buff_size = dash_dur * num_chan;

  /* Clean up buffer */
  for( buff_idx = 0; buff_idx < buff_size; buff_idx++ )
	samples_buff[buff_idx] = 128;

  /* Convert ascii characters to Morse hex code */
  mesg_len = strlen( mesg );
  for( mesg_idx = 0; mesg_idx < mesg_len; mesg_idx++ )
  {
	/* Find equivalent Hex morse code */
	for( idx = 0; idx < NUM_MORSE_CHARS; idx++ )
	  if( mesg[mesg_idx] == ascii_char[idx] )
		break;
	morse_chr = morse_code[idx];

	/* Find beginning of Morse code (1st '1') */
	for( idx = 8; idx >= 0; idx-- )
	  if( morse_chr & (1 << idx) )
		break;

	morse_idx = idx - 1;
	/* Transmit dots (1's) or dashes (0's) */
	while( morse_idx >= 0 )
	{
	  /* If element is dot ('1') */
	  if( morse_chr & (1 << morse_idx) )
		/* Tone duration in samples - edges */
		tone_dur = elem_dur - 2*COSINE_WAVE_TABLE_SIZE;
	  else /* If element is dash ('0') */
		/* Tone duration in samples is triple */
		tone_dur = dash_dur - 2*COSINE_WAVE_TABLE_SIZE;

	  /* Total samples to be generated */
	  buff_size = (tone_dur + 2*COSINE_WAVE_TABLE_SIZE) * num_chan;

	  /* Initialize buffer according to stereo/mono mode */
	  if( num_chan == 2 )
		buff_idx = chan_used;
	  else
		buff_idx = 0;

	  /* Fill buffer with cosine shaped rising edge */
	  sine_idx = 0;
	  for( cosine_idx=COSINE_WAVE_TABLE_SIZE-1; cosine_idx>=0; cosine_idx-- )
	  {
		samples_buff[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;
		if( ++sine_idx == SINE_WAVE_TABLE_SIZE )
		  sine_idx = 0;
	  }

	  /* Fill buffer with the steady tone samples */
	  for( idx = 0; idx < tone_dur; idx++ )
	  {
		samples_buff[buff_idx] = sine_table[sine_idx] + 128;
		buff_idx += num_chan;
		if( ++sine_idx == SINE_WAVE_TABLE_SIZE )
		  sine_idx = 0;
	  }

	  /* Fill buffer with cosine shaped falling edge */
	  for( cosine_idx=0; cosine_idx<COSINE_WAVE_TABLE_SIZE; cosine_idx++ )
	  {
		samples_buff[buff_idx] = (cosine_table[cosine_idx] *
			sine_table[sine_idx] / 65536) + 128;
		buff_idx += num_chan;
		if( ++sine_idx == SINE_WAVE_TABLE_SIZE )
		  sine_idx = 0;
	  }

	  /*** Write samples buffer to DSP, abort on error ***/
	  DSP_Write( samples_buff, buff_size );

	  /* Initialize buffer according to stereo/mono mode */
	  if( num_chan == 2 )
		buff_idx = chan_used;
	  else
		buff_idx = 0;

	  /* Fill buffer with 0 level for space */
	  /* If inter-word space add 3 extra space elements*/
	  if( morse_chr == 0x01 )
	  {
		for( idx = 0; idx < dash_dur; idx++ )
		{
		  samples_buff[buff_idx] = 128;
		  buff_idx += num_chan;
		}
		buff_size = dash_dur * num_chan;

		/*** Writebuffer to DSP, abort on error ***/
		DSP_Write( samples_buff, buff_size );

	  }/*  if( morse_chr == 0x01 ) */

	  /* Add inter-element space */
	  for( idx = 0; idx < elem_dur; idx++ )
	  {
		samples_buff[buff_idx] = 128;
		buff_idx += num_chan;
	  }
	  buff_size = elem_dur * num_chan;

	  /*** Write buffer to DSP, abort on error ***/
	  DSP_Write( samples_buff, buff_size );
	  morse_idx--;

	} /* while( morse_idx >= 0 ) */

	/* Send inter-character space */
	/* Initialize buffer according to stereo/mono mode */
	if( num_chan == 2 )
	  buff_idx = chan_used;
	else
	  buff_idx = 0;

	for( idx = 0; idx < 2 * elem_dur; idx++ )
	{
	  samples_buff[buff_idx] = 128;
	  buff_idx += num_chan;
	}

	buff_size = 2 * elem_dur * num_chan;

	/*** Write samples buffer to DSP ***/
	DSP_Write( samples_buff, buff_size );

  } /* for( mesg_idx = 0; mesg_idx < mesg_len; mesg_idx++ ) */

} /* Morse_Transmit() */

/*------------------------------------------------------------------------*/
