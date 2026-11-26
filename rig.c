/*  rig.c
 *
 *  Rig control for lpsk31, built on top of the Hamlib library.
 *
 *  This replaces the FT-847-only CAT implementation found in cat.c.
 *  Any rig supported by hamlib works via a direct device connection
 *  (model + device path), or any rig served by a rigctld server via
 *  the NET rigctl backend (radio model 2) with a host:port address.
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

#include "rig.h"

#include <hamlib/rig.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define RIG_ERROR_BUF_SIZE 256

/* Runtime config data */
extern rc_data_t rc_data;

/* Shared hamlib handle */
static RIG* rig = NULL;

/* Last error message */
static char last_error[RIG_ERROR_BUF_SIZE] = "";

static void set_error(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(last_error, sizeof(last_error), fmt, ap);
  va_end(ap);
}

/*----------------------------------------------------------------------*/

/*  rig_connect()
 *
 *  Connects to the rig at the device specified by the configuration.
 *
 *  Uses the NET rigctl backend (radio model 2) when the configured
 *  device looks like a host:port address and no explicit hamlib
 *  model is given; otherwise connects directly to the device with
 *  the configured hamlib model (e.g. 3073 for the Icom IC-7300).
 *
 *  Returns 0 on success or a non-zero value on failure.
 */

int rig_connect(void) {
  rig_model_t model;
  char* colon;

  if(rig != NULL)
    return 0;

  last_error[0] = '\0';

  /* Silence hamlib trace output; keep only errors */
  rig_set_debug(RIG_DEBUG_ERR);

  if(rc_data.rig_model[0] != '\0') {
    /* Direct connection with explicit hamlib model */
    char* end;
    long m;

    m = strtol(rc_data.rig_model, &end, 10);
    if(*end != '\0' || m <= 0) {
      set_error("Invalid hamlib model '%s' in config", rc_data.rig_model);
      return -1;
    }

    model = (rig_model_t)m;

    if(rc_data.rig_device[0] == '\0') {
      set_error("No rig device specified in config");
      return -1;
    }
  } else {
    /* NET rigctl connection (rigctld server) */
    if(rc_data.rig_device[0] == '\0') {
      set_error("No rig device/host:port specified in config");
      return -1;
    }

    colon = strchr(rc_data.rig_device, ':');
    if(colon == NULL || colon == rc_data.rig_device || colon[1] == '\0') {
      set_error("Rig device '%s' is not a host:port address "
                "(or set the hamlib model in config for a direct connection)",
                rc_data.rig_device);
      return -1;
    }

    model = RIG_MODEL_NETRIGCTL;
  }

  rig = rig_init(model);
  if(rig == NULL) {
    set_error("rig_init() failed");
    return -1;
  }

  /* Set the device path/address ("rig_pathname") */
  {
    token_t tok = rig_token_lookup(rig, "rig_pathname");
    if(tok == 0) {
      set_error("rig_token_lookup(rig_pathname) failed");
      rig_close(rig);
      rig = NULL;
      return -1;
    }

    if(rig_set_conf(rig, tok, rc_data.rig_device) != RIG_OK) {
      set_error("Setting rig device '%s' failed", rc_data.rig_device);
      rig_close(rig);
      rig = NULL;
      return -1;
    }
  }

  {
    int err = rig_open(rig);
    if(err != RIG_OK) {
      set_error("rig_open(): %s", rigerror(err));
      rig_close(rig);
      rig = NULL;
      return -1;
    }
  }

  last_error[0] = '\0';
  return 0;
}

/*----------------------------------------------------------------------*/

/*  rig_disconnect()
 *
 *  Closes the rig connection if open.
 */

void rig_disconnect(void) {
  if(rig != NULL) {
    rig_close(rig);
    rig = NULL;
  }
}

/*----------------------------------------------------------------------*/

/*  rig_is_open() */

int rig_is_open(void) { return rig != NULL; }

/*----------------------------------------------------------------------*/

/*  rig_set_mode_and_ptt()
 *
 *  Switches the rig into transmit (USB or LSB) or receive (CW) mode
 *  according to the MODE_TRANSMIT flag, and asserts/deasserts PTT.
 *  This mirrors the behavior of the old FT-847-only Set_Tcvr_Mode().
 */

/*  choose_mode()
 *
 *  Picks the mode to set on the rig. Prefers the digital USB/LSB
 *  modes (USB-D / PKTUSB in rigctl terms, where available) over the
 *  analog SSB modes for TX and RX alike. Falls back to the analog
 *  modes silently if the digital modes are not supported.
 */

static rmode_t
choose_mode(void)
{
  rmode_t digital, analog;

  if(isFlagSet(MODE_QPSK_LSB)) {
    digital = RIG_MODE_PKTLSB;   /* USB-D/PKTUSB for LSB: PKTLSB */
    analog = RIG_MODE_LSB;
  } else {
    digital = RIG_MODE_PKTUSB;   /* USB-D digital USB */
    analog = RIG_MODE_USB;
  }

  {
    int ret;
    pbwidth_t width = rig_passband_narrow(rig, digital);

    ret = rig_set_mode(rig, RIG_VFO_CURR, digital, width);
    if(ret == RIG_OK) {
      last_error[0] = '\0';
      return digital;
    }
    /* Digital mode not available for this rig; fall back to analog */
    set_error("Digital mode not supported; using analog %s",
              isFlagSet(MODE_QPSK_LSB) ? "LSB" : "USB");
  }

  {
    pbwidth_t width = rig_passband_narrow(rig, analog);
    int ret = rig_set_mode(rig, RIG_VFO_CURR, analog, width);
    if(ret != RIG_OK) {
      set_error("Could not set mode: %s", rigerror(ret));
    }
    return analog;
  }
}

void rig_set_mode_and_ptt(void) {
  if(!rig_is_open())
    return;

  (void)choose_mode();

  if(isFlagSet(MODE_TRANSMIT))
    rig_set_ptt(rig, RIG_VFO_CURR, RIG_PTT_ON);
  else
    rig_set_ptt(rig, RIG_VFO_CURR, RIG_PTT_OFF);
}

/*----------------------------------------------------------------------*/

/*  rig_get_status()
 *
 *  Reads the operating frequency and an optional RST-style signal
 *  report from the rig.
 *
 *  freq is filled with e.g. "14.050" (MHz, 3 decimals).
 *  rst is filled with an optional 3-digit S-meter-ish report when
 *  the rig reports signal strength; otherwise it is left empty.
 *
 *  Returns 0 on success (even if strength is unavailable), or
 *  non-zero if the frequency could not be read.
 */

int rig_get_status(char* freq, char* rst) {
  freq_t f;
  value_t strength = {0};

  freq[0] = '\0';
  rst[0] = '\0';

  if(!rig_is_open())
    return -1;

  if(rig_get_freq(rig, RIG_VFO_CURR, &f) != RIG_OK)
    return -1;

  /* MHz with 3 decimal places, e.g. "14.050" or "145.050" */
  snprintf(freq, 8, "%.3f", (double)f / 1000000.0);

  /* Signal strength is best-effort: not all rigs/backends support it */
  if(rig_get_level(rig, RIG_VFO_CURR, RIG_LEVEL_STRENGTH, &strength) ==
       RIG_OK &&
     strength.i != 0) {
    /* STRENGTH is reported relative to S9 (dB, negative = weaker).
     * Map to a coarse 3-digit S-like report in the same style as the
     * old FT-847 CAT code (500 + 10*s + 9, s in 3..9). */
    int s = 9 + strength.i / 6;
    if(s < 3)
      s = 3;
    if(s > 9)
      s = 9;
    int val = 500 + 10 * s + 9;
    snprintf(rst, 4, "%3d", val);
  }

  return 0;
}

/*----------------------------------------------------------------------*/

/*  rig_last_error() */

const char* rig_last_error(void) { return last_error; }

/*----------------------------------------------------------------------*/
