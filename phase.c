/*  phase.c
 *
 *  Phase detector functions of lpsk31 application
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

/* Runtime configuration data */
extern rc_data_t rc_data;

/*------------------------------------------------------------------------*/

/*  PSK31_Phase()
 *
 *  Detects phase changes of the PSK carrier
 */

  int
PSK31_Phase( void )
{
  static int
	old_phase,   /* Old carrier phase from detector o/p */
	ave_phs_chg, /* Ave. value of carrier phase changes */
	chA_squelch, /* Channel A squelch (signal) level    */
	chB_squelch, /* Channel B squelch (signal) level    */
	last_lev_i,  /* Last phase detector in-phase signal level   */
	last_lev_q,  /* Last phase detector quadrature signal level */

	/* Four quadrants of a signal cycle (in dsp samples) */
	quadrant_1 = DSP_SPEED/AUDIO_FREQUENCY/4,
	quadrant_2 = DSP_SPEED/AUDIO_FREQUENCY/2,
	quadrant_3 = (3*DSP_SPEED/AUDIO_FREQUENCY)/4,
	quadrant_4 = DSP_SPEED/AUDIO_FREQUENCY,

	/* PSK31 element period in DSP samples */
	elem_half_period = BUFFER_SIZE/2;

  int
	elem_timer,    /* Counter used for producing the baud rate  */
	sample_lev,    /* Sample level calculated from raw sample   */
	det_phase,     /* Phase in samples of phase detector        */
	det_level_i,   /* Phase detector in-phase signal level      */
	det_level_q,   /* Phase detector quadrature signal level    */
	chA_level_i,   /* 'A' channel in-phase signal level         */
	chA_level_q,   /* 'A' channel quadrature signal level       */
	chB_level_i,   /* 'B' channel in-phase signal level         */
	chB_level_q,   /* 'B' channel quadrature signal level       */
	sel_level_i,   /* In-phase level from selected channel      */
	sel_level_q,   /* Quadrature level from selected channel    */
	carrier_phase, /* Phase angle of audio signal from receiver */
	phase_chg,     /* Difference between consecutive phase val. */
	phase_slip,    /* Average and normalized carrier phase slip */
	max_slip,      /* Maximum phase shift taken as phase slip   */
	slice_cnt,     /* Counts slices (= 1/2 PSK31 elements) done */

	/* Normalized values of detector */
	/* channels and selected squelch */
	chA_norm_lev_i,
	chA_norm_lev_q,
	chB_norm_lev_i,
	chB_norm_lev_q,
	norm_squelch;

  /* Initialize levels and psk phase */
  chA_level_i = chA_level_q = 0;
  chB_level_i = chB_level_q = 0;
  det_phase   = 0;

  /*** Following code acts like a synchronous detector,   ***/
  /*** working at a nominal 500 Hz. To avoid the nulling  ***/
  /*** of the o/p levels when a phase reversal occurs in  ***/
  /*** the middle of a PSK31 element, two 'channels' are  ***/
  /*** used separated by a 1/2 element time delay and the ***/
  /*** one with the best squelch level is selected in use ***/

  /* Return psk phase after two PSK31 'slices' eg 1 element period */
  for( slice_cnt = 0; slice_cnt < 2; slice_cnt++ )
  {
	/* Initialize levels and psk phase */
	det_level_i = det_level_q = 0;

	/* Summate samples for each half element */
	for(elem_timer = 0; elem_timer < elem_half_period; elem_timer++ )
	{
	  /* Get next signal sample from buffer */
	  sample_lev = Get_Signal_Sample();

	  /*** Calculate phase detector in-phase level. ***/
	  /* Samples are accumulated over a PSK31 element */

	  /* From 0 to 180 deg use sample level as is */
	  if( det_phase < quadrant_2 )
		det_level_i += sample_lev;
	  else
		/* From 180 to 360 deg use negated (rectified) sample */
		det_level_i -= sample_lev;

	  /*** Calculate phase detector quadrature level. ***/
	  /* Samples are accumulated over a PSK31 element   */

	  /* From 90 to 270 deg use sample level as is */
	  if( (det_phase >= quadrant_1) &&
		  (det_phase < quadrant_3) )
		det_level_q += sample_lev;
	  else
		/* From 270 to 90 deg use negated (rectified) sample */
		det_level_q -= sample_lev;

	  /* Advance detector phase by one DSP sample count */
	  det_phase++;

	  /* At a full signal cycle, reset the */
	  /* phase count of the phase detector */
	  if( det_phase == quadrant_4 )
		det_phase = 0;

	} /* for(elem_timer=0;elem_timer<elem_half_period;elem_timer++) */

	/* Channel A sums current detector levels */
	chA_level_i += det_level_i;
	chA_level_q += det_level_q;

	/* Channel B sums last detector levels */
	chB_level_i += last_lev_i;
	chB_level_q += last_lev_q;

	/* Save current levels */
	last_lev_i = det_level_i;
	last_lev_q = det_level_q;

  } /* for( slice_cnt = 0; slice_cnt < 2; slice_cnt++ ) */

  /* Normalize detector channel levels */
  chA_norm_lev_i = chA_level_i / BUFFER_SIZE;
  chA_norm_lev_q = chA_level_q / BUFFER_SIZE;
  chB_norm_lev_i = chB_level_i / BUFFER_SIZE;
  chB_norm_lev_q = chB_level_q / BUFFER_SIZE;

  /* Calculate sliding window average values for the channel */
  /* squelch. This is the vector magnitude of in-phase and   */
  /* quadrature signals from the phase detector channels     */
  chA_squelch = ( chA_squelch * (SQUELCH_WINDOW - 1) + SQUELCH_WINDOW *
	  Scalar_Mag(chA_norm_lev_i, chA_norm_lev_q) )/SQUELCH_WINDOW;

  chB_squelch = ( chB_squelch * (SQUELCH_WINDOW - 1) + SQUELCH_WINDOW *
	  Scalar_Mag(chB_norm_lev_i, chB_norm_lev_q) )/SQUELCH_WINDOW;

  /* Set best channel select flag according to squelch */
  if( chA_squelch > 2 * chB_squelch )
	Set_Flag( CHANNEL_A_SELECT );
  else
	if( chB_squelch > 2 * chA_squelch )
	  Clear_Flag( CHANNEL_A_SELECT );

  /* Select best channel data according to flag */
  if( isFlagSet(CHANNEL_A_SELECT) )
  {
	/* Normalize squelch value and select level */
	norm_squelch = chA_squelch / SQUELCH_WINDOW;
	sel_level_i  = chA_level_i;
	sel_level_q  = chA_level_q;
  }
  else
  {
	/* Normalize squelch value and select level */
	norm_squelch = chB_squelch / SQUELCH_WINDOW;
	sel_level_i  = chB_level_i;
	sel_level_q  = chB_level_q;
  }

  /* Set squelch open flag */
  if( norm_squelch > rc_data.sqlch_thr )
	Set_Flag( SQUELCH_OPEN );
  else
	Clear_Flag( SQUELCH_OPEN );

  /* Calculate new carrier phase if squelch is  */
  /* open. Calculate carrier phase from arctan  */
  /* of in-phase and quadrature detector levels */
  if( isFlagSet(SQUELCH_OPEN) || isFlagSet(NO_SQUELCH) )
	carrier_phase = Arc_Tan( sel_level_q, sel_level_i );
  else
	carrier_phase = 0;

  /* Phase change between consecutive phase values */
  phase_chg = old_phase - carrier_phase;
  old_phase = carrier_phase;

  /* For BPSK phase slip limit = 90 */
  /* For QPSK phase slip limit = 45 */
  if( isFlagSet(MODE_BPSK) )
	max_slip = 90;
  else
	max_slip = 45;

  /* Limit phase changes to +-max_slip for slip rate calculations. */
  /* Caclulate average phase change with a sliding window averager */
  if( abs(phase_chg) < max_slip )
	ave_phs_chg = ( (ave_phs_chg * (PHASE_AVE_WINDOW-1)) +
		(PHASE_AVE_WINDOW * phase_chg) ) / PHASE_AVE_WINDOW;

  /* Phase slip = normalized value of average phase change */
  phase_slip = ave_phs_chg / PHASE_AVE_WINDOW;

  /* Keep phase change in range */
  if( phase_chg < 0 )
	phase_chg += 360;

  /* Plot carrier phase on 'scope' */
  Screen_Phase_Window( carrier_phase, phase_slip, norm_squelch );

  return( phase_chg );

} /* End of PSK31_Phase() */

/*------------------------------------------------------------------------*/

/*  Arc_Tan()
 *
 *  Calculates an aproximate integer arc tan (in deg)
 *  for a given pair of integer parameters (y,x). The
 *  result is in range 0 - +-180 deg, max error ~.5 deg.
 *  The first approximation used is arctan = 45*y/x for
 *  y<x and for y>x arctan = 90-45*x/y. This is used in
 *  a table to improve the accuracy. The signs of x
 *  and y are used to find arc tan in all quadrants.
 */

  int
Arc_Tan( int y, int x )
{
  int
	angle,   /* Inermediate result  */
	arc_tan; /* Final arctan result */

  /* Arc tan correction table */
  char table[46] = ARC_TAN_TABLE;

  /* Avoid divide by zero */
  if( (x == 0) && (y == 0) )
	return( 0 );

  /* Angle +-(0-45) deg or (135-225) */
  if( abs(y) <= abs(x) )
  {
	/* First approximation */
	angle = (45 * y) / x;

	/* Use correction table for more accuracy */
	if( angle < 0 )
	  angle = -(int) table[-angle];
	else
	  angle =  (int) table[angle];

	/* Set result in correct quadrant */
	if( x < 0 )
	  arc_tan = 180 + angle;
	else
	  if( y < 0 )
		arc_tan = 360 + angle;
	  else
		arc_tan = angle;

  }  /* if( abs(y) <= abs(x) ) */
  else /* Angle (45-135) or (225-315) */
  {
	/* First approximation */
	angle = (45 * x) / y;

	/* Use correction table for more accuracy */
	if( angle < 0 )
	  angle = -(int) table[-angle];
	else
	  angle =  (int) table[angle];

	/* Set result in correct quadrant */
	if( y < 0 )
	  arc_tan = 270 - angle;
	else
	  arc_tan = 90 - angle;

  } /* else */

  return( arc_tan );

} /* End of Arc_Tan() */

/*------------------------------------------------------------------------*/

/*  Scalar_Mag()
 *
 *  sqrt(x^2 + y^2). This is approximated by a Taylor
 *  series expansion in (x+y) e.g: ((x+y)^2-x*y)/(x+y)
 *  to first order, and refined by one iteration of the
 *  Newton-Raphson method. This is rather wierd but
 *  seems to give the most accurate approximation.
 */

  int
Scalar_Mag( int x, int y )
{
  /* Vector magnitude of two values at 90 deg */
  int vector_mag;

  int
	sum,    /* Sum of two consecutive samples    */
	prod,   /* Product of two cnsecutive samples */
	aprox;  /* First approxim. to sqrt(x^2+y^2)  */

  /* Remove sign */
  x = abs( x );
  y = abs( y );

  /* Calculate first approximation to sqrt(x^2+y^2) */
  sum   = x + y;
  prod  = x * y;
  aprox = ( sum == 0 ? 0 : (sum * sum - prod) / sum );

  /* Refine by one iteration using Newton-Raphson method */
  vector_mag = ( aprox == 0 ? 0 :
	  ((aprox * aprox + sum * sum - (2 * prod)) / aprox) / 2 );

  return( vector_mag );

} /* End of Scalar_Mag() */

/*------------------------------------------------------------------------*/
