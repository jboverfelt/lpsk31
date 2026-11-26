/*  sound.c
 *
 *  Soundcard handling functions of lpsk31 application
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

#include "sound.h"

/* Runtime config data */
extern rc_data_t rc_data;

/* ALSA pcm capture and mixer handles */
static snd_pcm_t* capture_handle = NULL;
static snd_pcm_t* playback_handle = NULL;
static snd_mixer_t* mixer_handle = NULL;
static snd_pcm_hw_params_t* hw_params = NULL;

/* Simple mixer elements for setting up
 * capture amd playback sources and volume */
static snd_mixer_elem_t* cap_elem = NULL;
static snd_mixer_elem_t* pcm_elem = NULL;
static snd_mixer_elem_t* mst_elem = NULL;

int xmit_buf_size, /* Xmit DSP signal samples buffer   */
  recv_buf_size,   /* Recv DSP signal samples buffer   */
  recv_buf_idx;    /* Index to Rx signal samples buffer*/

/* Signal samples buffers */
short* xmit_buffer = NULL;
short* recv_buffer = NULL;

/*--------------------------------------------------------------------*/

/*  Setup_Sound_Card()
 *
 *  Sets up mixer and DSP devices
 */

void Setup_Sound_Card(void) {
  /* Open PCM for capture. (Aborts on error) */
  Open_Capture();

  /* Open PCM for playback. (Aborts on error) */
  Open_Playback();

  /* Open Mixer. (Aborts on error) */
  Open_Mixer();

  Set_Capture_Level(rc_data.cap_lev);
  Set_Playback_Level(rc_data.pbk_lev);

} /* End of Setup_Sound_Card() */

/*--------------------------------------------------------------------*/

/* Open_PCM()
 *
 * Opens a pcm device for a given handle
 */
void Open_PCM(snd_pcm_t** handle, snd_pcm_stream_t stream) {
  int error;

  /* Open pcm */
  error = snd_pcm_open(handle, rc_data.snd_card, stream, PCM_BLOCK);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot open sound device %s\n", rc_data.snd_card);
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Allocate memory to hardware parameters structure */
  error = snd_pcm_hw_params_malloc(&hw_params);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot allocate hw_params struct\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Initialize hardware parameter structure */
  error = snd_pcm_hw_params_any(*handle, hw_params);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot initialize hw_params struct\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set access type */
  error = snd_pcm_hw_params_set_access(*handle, hw_params, SND_PCM_ACCESS);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set PCM access type\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set sample format */
  error = snd_pcm_hw_params_set_format(*handle, hw_params, SND_PCM_FORMAT);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set sample format\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set sample rate */
  error = snd_pcm_hw_params_set_rate(*handle, hw_params, DSP_RATE, EXACT_VAL);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set sample rate to %d\n", DSP_RATE);
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set channel count */
  error = snd_pcm_hw_params_set_channels(*handle, hw_params, rc_data.num_chn);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set channel count to %d\n",
            rc_data.num_chn);
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set number of periods */
  error =
    snd_pcm_hw_params_set_periods(*handle, hw_params, NUM_PERIODS, EXACT_VAL);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set number of periods\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set period size */
  error = snd_pcm_hw_params_set_period_size(*handle, hw_params, PERIOD_SIZE,
                                            EXACT_VAL);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set period size\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Set parameters */
  error = snd_pcm_hw_params(*handle, hw_params);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot set capture parameters\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }
  snd_pcm_hw_params_free(hw_params);
  hw_params = NULL;

  /* Prepare audio interface for use */
  error = snd_pcm_prepare(*handle);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot prepare audio interface\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  return;
} /* Open_PCM() */

/*----------------------------------------------------------------*/

/* Open_Capture()
 *
 * Opens sound card for Capture
 */
void Open_Capture(void) {
  /* Open & setup pcm for Capture */
  Open_PCM(&capture_handle, SND_PCM_STREAM_CAPTURE);

  /* Size of receive samples buffer in 'shorts' */
  recv_buf_size = PERIOD_SIZE * rc_data.num_chn;

  /* Index to recv samples buffer (set to end) */
  recv_buf_idx = recv_buf_size;

  /* Allocate memory to receive samples buffer */
  if(recv_buffer == NULL)
    recv_buffer = malloc((size_t)recv_buf_size * sizeof(short));
  if(recv_buffer == NULL) {
    fprintf(stderr, "lpsk31: receive buffer memory allocation failed\n");
    exit(ERROR);
  }
  memset(recv_buffer, 0, (size_t)recv_buf_size * sizeof(short));

  return;
} /* Open_Capture() */

/*----------------------------------------------------------------*/

/* Open_Playback()
 *
 * Opens sound card for Playback
 */
void Open_Playback(void) {
  /* Open & setup pcm for Playback */
  Open_PCM(&playback_handle, SND_PCM_STREAM_PLAYBACK);

  /* Size of transmit samples buffer in frames */
  if(rc_data.tone_dur > 0)
    xmit_buf_size = (DSP_RATE * rc_data.tone_dur + rc_data.psk_elem2);
  else
    xmit_buf_size = rc_data.psk_elem;

  /* Allocate memory to transmit samples buffer */
  if(xmit_buffer == NULL)
    xmit_buffer = malloc(rc_data.num_chn * xmit_buf_size * sizeof(short));
  if(xmit_buffer == NULL) {
    fprintf(stderr, "lpsk31: memory allocation for xmit buffer failed\n");
    exit(ERROR);
  }
  memset(xmit_buffer, 0, (size_t)xmit_buf_size * sizeof(short));

  return;
} /* Open_Playback() */

/*----------------------------------------------------------------*/

/* Open_Mixer()
 *
 * Opens mixer interface
 */
void Open_Mixer(void) {
  snd_mixer_elem_t* elem;
  snd_mixer_selem_id_t* sid;
  int error;

  /* Open mixer handle */
  error = snd_mixer_open(&mixer_handle, 0);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot open mixer handle\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Attach mixer */
  error = snd_mixer_attach(mixer_handle, rc_data.snd_card);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot attach mixer to %s\n", rc_data.snd_card);
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Register mixer */
  error = snd_mixer_selem_register(mixer_handle, NULL, NULL);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot register mixer\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Load mixer */
  error = snd_mixer_load(mixer_handle);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot load mixer\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Allocate selem_id structure */
  error = snd_mixer_selem_id_malloc(&sid);
  if(error < 0) {
    fprintf(stderr, "lpsk31: cannot allocate selem_id struct\n");
    fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    exit(ERROR);
  }

  /* Find capture source selem if specified */
  elem = NULL;
  if(strcmp(rc_data.cap_src, "--") != 0) {
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, rc_data.cap_src);
    elem = snd_mixer_find_selem(mixer_handle, sid);
    if(!elem) {
      fprintf(stderr, "lpsk31: cannot find capture source element %s\n",
              rc_data.cap_src);
      snd_mixer_selem_id_free(sid);
      exit(ERROR);
    }

    if(snd_mixer_selem_has_capture_switch(elem)) {
      error = snd_mixer_selem_set_capture_switch(elem, rc_data.channel, 1);
      if(error < 0) {
        fprintf(stderr, "lpsk31: cannot set capture source to %s\n",
                rc_data.cap_src);
        fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
        snd_mixer_selem_id_free(sid);
        exit(ERROR);
      }
    } else {
      fprintf(stderr, "lpsk31: device %s does not have Capture capability\n",
              rc_data.cap_src);
      snd_mixer_selem_id_free(sid);
      exit(ERROR);
    }
  }

  /* Find capture volume selem if not -- */
  if(strcmp(rc_data.cap_vol, "--") != 0) {
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, rc_data.cap_vol);
    cap_elem = snd_mixer_find_selem(mixer_handle, sid);
    if(!cap_elem) {
      fprintf(stderr, "lpsk31: cannot find Capture volume element %s\n",
              rc_data.cap_vol);
      fprintf(stderr, "lpsk31: Capture volume not set\n");
    }
  }

  /* Find Master playback volume selem if not -- */
  if(strcmp(rc_data.mst_vol, "--") != 0) {
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, rc_data.mst_vol);
    mst_elem = snd_mixer_find_selem(mixer_handle, sid);
    if(!mst_elem) {
      fprintf(stderr, "lpsk31: cannot find Playback volume element %s\n",
              rc_data.mst_vol);
      fprintf(stderr, "lpsk31: Playback volume not set\n");
    }
  }

  /* Find PCM playback volume selem if not -- */
  if(strcmp(rc_data.pcm_vol, "--") != 0) {
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, rc_data.pcm_vol);
    pcm_elem = snd_mixer_find_selem(mixer_handle, sid);
    if(!pcm_elem) {
      fprintf(stderr, "lpsk31: cannot find Playback volume element %s\n",
              rc_data.pcm_vol);
      fprintf(stderr, "lpsk31: Playback volume not set\n");
    }
    // FIXME set volume

    /* Set PCM playback switch for playback source */
    if(pcm_elem && snd_mixer_selem_has_playback_switch(pcm_elem)) {
      error = snd_mixer_selem_set_playback_switch(pcm_elem, rc_data.channel, 1);
      if(error < 0) {
        fprintf(stderr, "lpsk31: cannot set playback channel %d\n",
                rc_data.channel);
        fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
        exit(ERROR);
      }
    }
  }
  snd_mixer_selem_id_free(sid);

  return;
} /* Open_Mixer() */

/*----------------------------------------------------------------*/

/* Set_Playback_Level()
 *
 * Sets Playback Control level
 */
void Set_Playback_Level(int level) {
  long cmin, cmax;
  int error;

  /* Set PCM playback volume */
  if(pcm_elem && snd_mixer_selem_has_playback_volume(pcm_elem)) {
    /* Change from % volume to sound card value */
    long lev;
    snd_mixer_selem_get_playback_volume_range(pcm_elem, &cmin, &cmax);
    lev = cmin + ((cmax - cmin) * level) / 100;

    /* Set PCM playback volume */
    error = snd_mixer_selem_set_playback_volume(pcm_elem, rc_data.channel, lev);
    if(error < 0) {
      fprintf(stderr, "lpsk31: cannot set PCM Playback volume to %d\n", level);
      fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    }
  }

  /* Set Master playback volume */
  if(mst_elem && snd_mixer_selem_has_playback_volume(mst_elem)) {
    /* Change from % volume to sound card value */
    long lev;
    snd_mixer_selem_get_playback_volume_range(mst_elem, &cmin, &cmax);
    lev = cmin + ((cmax - cmin) * level) / 100;

    /* Set playback volume */
    error = snd_mixer_selem_set_playback_volume(mst_elem, rc_data.channel, lev);
    if(error < 0) {
      fprintf(stderr, "lpsk31: cannot set Master Playback volume to %d\n",
              level);
      fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    }
  }

  return;
} /* Set_Playback_Level() */

/*----------------------------------------------------------------*/

/* Set_Capture_Level()
 *
 * Sets Capture Control level
 */
void Set_Capture_Level(int level) {
  long cmin, cmax;

  /* Set capture volume */
  if(cap_elem && snd_mixer_selem_has_capture_volume(cap_elem)) {
    /* Change from % volume to sound card value */
    long lev;
    snd_mixer_selem_get_capture_volume_range(cap_elem, &cmin, &cmax);
    lev = cmin + ((cmax - cmin) * level) / 100;

    /* Set capture volume */
    int error =
      snd_mixer_selem_set_capture_volume(cap_elem, rc_data.channel, lev);
    if(error < 0) {
      fprintf(stderr, "lpsk31: cannot set Capture volume to %d\n", level);
      fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
    }
  }

  return;
} /* Set_Capture_Level() */

/*----------------------------------------------------------------*/

/*
 * Following 3 functions close sound card interfaces
 *
  void
Close_Playback( void )
{
  if( playback_handle != NULL )
        snd_pcm_close( playback_handle );
  playback_handle = NULL;

  if( hw_params != NULL )
        snd_pcm_hw_params_free( hw_params );
  hw_params = NULL;
}


  void
Close_Capture( void )
{
  if( capture_handle != NULL )
        snd_pcm_close( capture_handle );
  capture_handle = NULL;

  if( hw_params != NULL )
        snd_pcm_hw_params_free( hw_params );
  hw_params = NULL;
}


  void
Close_Mixer( void )
{
  if( mixer_handle != NULL )
        snd_mixer_close( mixer_handle );
  mixer_handle = NULL;
}

*----------------------------------------------------------------*/

/*  Signal_Sample()
 *
 *  Gets the next DSP sample of the signal input.
 */

int Signal_Sample(void) {
  snd_pcm_sframes_t error;
  int sample_val;

  /* Refill recv DSP samples buffer when needed */
  if(recv_buf_idx >= recv_buf_size) {
    /* Read audio samples from DSP, abort on error */
    error = snd_pcm_readi(capture_handle, recv_buffer, PERIOD_SIZE);
    if(error != PERIOD_SIZE) {
      fprintf(stderr, "lpsk31: Signal_Sample(): %s\n", snd_strerror(error));

      /* Try to recover from error */
      Xrun_Recovery(capture_handle, error);
    } /* if( error  ) */

    recv_buf_idx = rc_data.use_chn;
  } /* End of if( recv_buf_idx >= recv_buf_size ) */

  sample_val = (int)recv_buffer[recv_buf_idx] / 256;

  /* Increment according to mono/stereo mode */
  recv_buf_idx += rc_data.num_chn;

  return (sample_val);
} /* End of Signal_Sample() */

/*----------------------------------------------------------------*/

/*  DSP_Write()
 *
 *  Write a samples buffer to DSP
 */

void DSP_Write(short* buffer, int buff_size) {
  snd_pcm_sframes_t error;
  int num_writes, idx;
  short* buff_ptr;
  snd_pcm_uframes_t num_frames;

  /* Number of writes needed to write all data in buffer */
  num_writes = buff_size / PERIOD_SIZE + 1;

  /* Index (poiner) to samples buffer */
  buff_ptr = buffer;

  /* Write samples buffer to DSP in chunks of PERIOD_SIZE */
  for(idx = 0; idx < num_writes; idx++) {
    /* Calculate number of frames to write. It should be = PERIOD_SIZE
     * if we have >= PERIOD_SIZE samples left to write, or =
     * buff_size - (num_writes-1) * PERIOD_SIZE otherwise */
    num_frames = buff_size - idx * PERIOD_SIZE;
    if(num_frames > PERIOD_SIZE)
      num_frames = PERIOD_SIZE;

    /* Write samples buffer to DSP */
    error = snd_pcm_writei(playback_handle, buff_ptr, num_frames);
    if(error != (snd_pcm_sframes_t)num_frames) {
      if(error < 0)
        fprintf(stderr, "lpsk31: DSP_Write(): %s\n", snd_strerror(error));
      else
        fprintf(stderr,
                "lpsk31: DSP_Write(): Written only %d frames out of %d\n",
                (int)error, (int)num_frames);

      /* Try to recover from error */
      Xrun_Recovery(playback_handle, error);
    } /* if( error != (snd_pcm_sframes_t)buff_size ) */

    buff_ptr += PERIOD_SIZE * rc_data.num_chn;
  } /* for( idx = 0; idx < num_writes; idx++ ) */

  return;
} /* DSP_Write() */

/*----------------------------------------------------------------*/

/* Xrun_Recovery()
 *
 * Recover from underrrun (broken pipe) and suspend
 */
void Xrun_Recovery(snd_pcm_t* handle, int error) {
  if(error == -EPIPE) {
    error = snd_pcm_prepare(handle);
    if(error < 0) {
      fprintf(stderr, "lpsk31: cannot recover from underrun, prepare failed\n");
      fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
      return;
    }
  } else if(error == -ESTRPIPE) {
    while((error = snd_pcm_resume(handle)) == -EAGAIN)
      sleep(1);
    if(error < 0) {
      error = snd_pcm_prepare(handle);
      if(error < 0) {
        fprintf(stderr,
                "lpsk31: cannot recover from suspend, prepare failed\n");
        fprintf(stderr, "lpsk31: %s\n", snd_strerror(error));
        return;
      }
    }
  }

  return;
} /* Xrun_Recovery() */

/*----------------------------------------------------------------*/
