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

char label[9][19], /* Labels for F1-F9 commands */
  *macro[9];       /* Macros attached to F1-F9  */

/* Runtime config data */
rc_data_t rc_data;

/* Operator and location data */
op_data_t op_data;

/* Signal handler */
static void sig_handler(int signal);

/*------------------------------------------------------------------------*/

int main(int argc, char* argv[]) {
  /* Command line option returned by getopt() */
  int option;

  /* New and old actions for sigaction() */
  struct sigaction sa_new, sa_old;

  /* Initialize new actions */
  sa_new.sa_handler = sig_handler;
  sigemptyset(&sa_new.sa_mask);
  sa_new.sa_flags = 0;

  /* Register function to handle signals */
  sigaction(SIGINT, &sa_new, &sa_old);
  sigaction(SIGSEGV, &sa_new, 0);
  sigaction(SIGFPE, &sa_new, 0);
  sigaction(SIGTERM, &sa_new, 0);
  sigaction(SIGABRT, &sa_new, 0);

  /*** Process command line options ***/
  while((option = getopt(argc, argv, "hv")) != -1)

    switch(option) {
    case 'h': /* Print usage and exit */
      Usage();
      exit(0);

    case 'v': /* Print version and exit */
      fprintf(stderr, "%s\n", VERSION);
      exit(0);

    default: /* Print usage and exit */
      Usage();
      exit(-1);

    } /* End of switch( option ) */

  /* Load runtime config file, aborts on error */
  Load_Config();

  /* Auto-enable rig control if configured */
  if(rc_data.rig_enabled_default) {
    if(rig_connect() == 0) {
      Set_Flag(ENABLE_RIG);
    } else {
      const char *err = rig_last_error();
      fprintf(stderr, "lpsk31: Rig not available - %s\n",
              (err != NULL && err[0] != '\0') ? err : "unknown error");
    }
  }

  /* Prepare sound card, aborts on error */
  Setup_Sound_Card();

  /* Initialize ncurses */
  initscr();
  keypad(stdscr, TRUE);
  nonl();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  curs_set(TRUE);
  Set_Flag(NCURSES_INIT);

  /* Assuming terminal has color! */
  if(has_colors()) {
    start_color();

    init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_YELLOW_BLUE, COLOR_YELLOW, COLOR_BLUE);
    init_pair(COLOR_WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_WHITE_RED, COLOR_WHITE, COLOR_RED);
    init_pair(COLOR_WHITE_YELLOW, COLOR_WHITE, COLOR_YELLOW);
    init_pair(COLOR_WHITE_GREEN, COLOR_WHITE, COLOR_GREEN);
  }

  /* Go to ncurses interface until user quits */
  Screen_Root();
  Screen_Receive_Window();

  /* Cleanup and exit */
  Cleanup();

  return (0);

} /* End of  main() */

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

/*  config_resolve()
 *
 *  Locates the lpsk31rc file in the conventional locations:
 *
 *    1. $XDG_CONFIG_HOME/lpsk31/lpsk31rc
 *       (default $HOME/.config/lpsk31/lpsk31rc)
 *    2. <dir>/lpsk31/lpsk31rc for each dir in $XDG_CONFIG_DIRS
 *       (default /etc/xdg)
 *    3. The legacy ~/lpsk31/lpsk31rc location (with a notice)
 *
 *  Returns 1 and sets *origin with 1 (user config), 2 (system
 *  config) or 3 (legacy). Returns 0 if no file found.
 */

static int config_resolve(char *out_path, size_t out_len, int *origin) {
  const char *xdg, *dirs, *home;
  char xdg_path[4096], legacy_path[4096];
  const char *p;
  int rc;

  xdg = getenv("XDG_CONFIG_HOME");
  dirs = getenv("XDG_CONFIG_DIRS");
  home = getenv("HOME");

  /* 1. User config */
  if(xdg == NULL || xdg[0] == '\0') {
    if(home == NULL || home[0] == '\0')
      return 0;
    rc = snprintf(xdg_path, sizeof(xdg_path), "%s/.config/lpsk31/lpsk31rc",
                  home);
  } else {
    rc = snprintf(xdg_path, sizeof(xdg_path), "%s/lpsk31/lpsk31rc", xdg);
  }
  if(rc > 0 && (size_t)rc < sizeof(xdg_path) && access(xdg_path, F_OK) == 0) {
    snprintf(out_path, out_len, "%s", xdg_path);
    *origin = 1;
    return 1;
  }

  /* 2. System config dirs */
  if(dirs == NULL || dirs[0] == '\0')
    dirs = "/etc/xdg";

  p = dirs;
  while(*p) {
    const char *end = strchr(p, ':');
    size_t len;
    char dir_path[4096];

    if(end == NULL)
      len = strlen(p);
    else
      len = (size_t)(end - p);

    if(len > 0) {
      rc = snprintf(dir_path, sizeof(dir_path), "%.*s/lpsk31/lpsk31rc",
                    (int)len, p);
      if(rc > 0 && (size_t)rc < sizeof(dir_path) &&
         access(dir_path, F_OK) == 0) {
        snprintf(out_path, out_len, "%s", dir_path);
        *origin = 2;
        return 1;
      }
    }

    if(end == NULL)
      break;
    p = end + 1;
  }

  /* 3. Legacy location */
  if(home != NULL && home[0] != '\0') {
    rc = snprintf(legacy_path, sizeof(legacy_path), "%s/lpsk31/lpsk31rc",
                  home);
    if(rc > 0 && (size_t)rc < sizeof(legacy_path) &&
       access(legacy_path, F_OK) == 0) {
      snprintf(out_path, out_len, "%s", legacy_path);
      *origin = 3;
      return 1;
    }
  }

  return 0;
}

/*------------------------------------------------------------------*/

/* Whitespace helpers */

static void line_trim(char *line) {
  size_t n = strlen(line);
  while(n > 0) {
    char c = line[n - 1];
    if((c == '\r') || (c == '\n') || (c == ' ') || (c == '\t'))
      line[--n] = '\0';
    else
      break;
  }
}

static char *line_lstrip(char *s) {
  while((*s == ' ') || (*s == '\t'))
    s++;
  return s;
}

/* Bounded string copy: copies at most dst_size - 1 chars and always
   NUL-terminates. */
static void bounded_strncpy(char *dst, size_t dst_size, const char *src) {
  size_t n = strlen(src);
  if(n >= dst_size)
    n = dst_size - 1;
  memcpy(dst, src, n);
  dst[n] = '\0';
}

/*------------------------------------------------------------------*/

/*  legacy_next_line()
 *
 *  Reads the next meaningful line from the legacy config file:
 *  skips blank lines and comment lines (starting with '#').
 *  Returns 0 with *out filled and NUL-terminated, EOF at end of file.
 *  Lines longer than 1023 chars abort the program.
 */

static int
legacy_next_line(FILE *fp, char *out, size_t cap)
{
  char *p;
  size_t len;

  while(fgets(out, (int)cap, fp) != NULL)
  {
    len = strlen(out);
    if(len > 0 && out[len - 1] != '\n' && len >= cap - 1)
    {
      fprintf(stderr, "lpsk31: Legacy config file line too long\n");
      exit(-12);
    }
    line_trim(out);
    if(out[0] == '\0')
      continue;
    p = line_lstrip(out);
    if(*p == '\0' || *p == '#')
      continue;
    strncpy(out, p, cap);
    out[cap - 1] = '\0';
    return 0;
  }
  return EOF;
}

/*  read_required_field()
 *
 *  Reads the next non-blank, non-comment line and aborts if EOF.
 */

static const char *
read_required_field(FILE *fp, char *out, size_t cap, const char *what)
{
  if(legacy_next_line(fp, out, cap) == EOF)
  {
    fprintf(stderr, "lpsk31: Legacy config file is missing '%s'\n", what);
    exit(-12);
  }
  return out;
}

/*------------------------------------------------------------------*/

/*  legacy_parse()
 *
 *  Parses the old fixed-positional lpsk31rc file. The legacy format
 *  consists of 20 fixed fields followed by macro sections. Each macro
 *  section starts with a line of the form "[Label]" and is followed by
 *  macro text lines.
 *
 *  Uses the same runtime config data and macro arrays as the new
 *  format.
 */

static void
legacy_parse(FILE *fp, const char *fpath)
{
  char line[1024];

  /* Fixed-position fields */
  bounded_strncpy(rc_data.snd_card, sizeof(rc_data.snd_card),
                  read_required_field(fp, line, sizeof(line), "sound card"));

  bounded_strncpy(rc_data.cap_src, sizeof(rc_data.cap_src),
                  read_required_field(fp, line, sizeof(line),
                                      "capture source"));

  bounded_strncpy(rc_data.cap_vol, sizeof(rc_data.cap_vol),
                  read_required_field(fp, line, sizeof(line),
                                      "capture volume element"));

  bounded_strncpy(rc_data.mst_vol, sizeof(rc_data.mst_vol),
                  read_required_field(fp, line, sizeof(line),
                                      "master volume element"));

  bounded_strncpy(rc_data.pcm_vol, sizeof(rc_data.pcm_vol),
                  read_required_field(fp, line, sizeof(line),
                                      "PCM volume element"));

  rc_data.cap_lev = atoi(read_required_field(fp, line, sizeof(line),
                                             "capture level"));
  rc_data.pbk_lev = atoi(read_required_field(fp, line, sizeof(line),
                                             "playback level"));
  rc_data.channel = atoi(read_required_field(fp, line, sizeof(line),
                                             "ALSA channel"));
  rc_data.num_chn = atoi(read_required_field(fp, line, sizeof(line),
                                             "number of channels"));
  rc_data.use_chn = atoi(read_required_field(fp, line, sizeof(line),
                                             "channel in use"));
  rc_data.sqlch_thr = atoi(read_required_field(fp, line, sizeof(line),
                                               "squelch threshold"));
  rc_data.tone_dur = atoi(read_required_field(fp, line, sizeof(line),
                                              "tone duration"));
  rc_data.num_rev = atoi(read_required_field(fp, line, sizeof(line),
                                             "number of reversals"));
  rc_data.wpm = atoi(read_required_field(fp, line, sizeof(line), "wpm"));
  rc_data.wwrap = atoi(read_required_field(fp, line, sizeof(line),
                                           "word wrap column"));
  bounded_strncpy(rc_data.rig_device, sizeof(rc_data.rig_device),
                  read_required_field(fp, line, sizeof(line), "rig device"));

  bounded_strncpy(op_data.call, sizeof(op_data.call),
                  read_required_field(fp, line, sizeof(line),
                                      "operator callsign"));

  bounded_strncpy(op_data.name, sizeof(op_data.name),
                  read_required_field(fp, line, sizeof(line),
                                      "operator name"));

  bounded_strncpy(op_data.qth, sizeof(op_data.qth),
                  read_required_field(fp, line, sizeof(line), "QTH"));

  bounded_strncpy(op_data.loc, sizeof(op_data.loc),
                  read_required_field(fp, line, sizeof(line),
                                      "QTH locator"));

  /* Collect all remaining meaningful lines into an in-memory buffer */
  {
    const size_t buf_cap = 256 * 1024;
    char *buf;
    size_t buf_len = 0;
    size_t line_starts[256];
    int line_count = 0;

    buf = malloc(buf_cap);
    if(buf == NULL)
    {
      perror("lpsk31");
      exit(-13);
    }

    for(;;)
    {
      char line[1024];
      size_t len;

      if(legacy_next_line(fp, line, sizeof(line)) == EOF)
        break;
      len = strlen(line);

      if(buf_len + len + 2 >= buf_cap || line_count >= 256)
      {
        fprintf(stderr, "lpsk31: Legacy config file is too large\n");
        free(buf);
        exit(-12);
      }

      line_starts[line_count++] = buf_len;
      memcpy(buf + buf_len, line, len);
      buf_len += len;
      buf[buf_len++] = LF;
    }

    /* Parse the macro sections */
    {
      int idx = 0;
      int lb_idx = 0;

      while(idx < line_count && lb_idx < NUM_OF_LABELS)
      {
        size_t cur = line_starts[idx];
        size_t k = cur;
        size_t line_len;
        char line_copy[1024];

        while(k < buf_len && buf[k] != LF)
          k++;
        line_len = k - cur;

        if(line_len >= sizeof(line_copy))
        {
          fprintf(stderr, "lpsk31: Line too long in legacy macro section\n");
          free(buf);
          exit(-12);
        }
        memcpy(line_copy, buf + cur, line_len);
        line_copy[line_len] = '\0';

        /* Only lines starting with '[' begin macro sections */
        if(line_copy[0] != '[')
        {
          fprintf(stderr,
                  "lpsk31: Unexpected line in legacy macro section: %s\n",
                  line_copy);
          free(buf);
          exit(-12);
        }

        {
          char *close = strchr(line_copy, ']');
          size_t label_len;
          char label_buf[64];
          int macro_idx;
          size_t text_len;
          char *text_buf;
          int tg_cnt;

          if(close == NULL)
          {
            fprintf(stderr,
                    "lpsk31: Invalid macro label line: %s\n", line_copy);
            free(buf);
            exit(-12);
          }
          *close = '\0';
          label_len = (size_t)(close - line_copy - 1);
          if(label_len == 0 || label_len > 16)
          {
            fprintf(stderr,
                    "lpsk31: Invalid macro label length: %s\n", line_copy);
            free(buf);
            exit(-12);
          }
          memcpy(label_buf, line_copy + 1, label_len);
          label_buf[label_len] = '\0';

          macro_idx = lb_idx;
          snprintf(label[macro_idx], sizeof(label[macro_idx]), "%.16s",
                   label_buf);

          /* Skip the label line */
          idx++;

          /* Accumulate text */
          text_len = 0;
          text_buf = malloc(16384);
          if(text_buf == NULL)
          {
            perror("lpsk31");
            free(buf);
            exit(-13);
          }
          tg_cnt = 0;

          while(idx < line_count)
          {
            size_t scur = line_starts[idx];
            size_t sk = scur;
            size_t line_end;
            size_t line_len2;
            char line2[1024];

            while(sk < buf_len && buf[sk] != LF)
              sk++;
            line_end = sk;
            line_len2 = line_end - scur;

            if(line_len2 >= sizeof(line2))
            {
              fprintf(stderr,
                      "lpsk31: Line too long in legacy macro text\n");
              free(text_buf);
              free(buf);
              exit(-12);
            }
            memcpy(line2, buf + scur, line_len2);
            line2[line_len2] = '\0';

            /* If this line starts with '[', it's the next label */
            if(line2[0] == '[')
              break;

            if(text_len + line_len2 + 2 >= 16384)
            {
              fprintf(stderr,
                      "lpsk31: Macro text too long in legacy config\n");
              free(text_buf);
              free(buf);
              exit(-12);
            }

            memcpy(text_buf + text_len, line2, line_len2);
            text_len += line_len2;
            text_buf[text_len++] = LF;

            for(size_t p2 = 0; p2 < line_len2; p2++)
              if(line2[p2] == '<' || line2[p2] == '>')
                tg_cnt++;

            idx++;
          }

          if(tg_cnt & 1)
          {
            fprintf(stderr,
                    "lpsk31: Bad tag balance in macro '%s' in %s\n",
                    label[macro_idx], fpath);
            free(text_buf);
            free(buf);
            exit(-12);
          }

          text_buf[text_len] = '\0';
          macro[macro_idx] = strdup(text_buf);
          if(macro[macro_idx] == NULL)
          {
            perror("lpsk31");
            free(text_buf);
            free(buf);
            exit(-13);
          }
          free(text_buf);

          lb_idx++;
        }
      }

      /* Leftover label lines (more than NUM_OF_LABELS) are ignored */

      free(buf);
    }
  }
}

/*------------------------------------------------------------------*/

/*  mkdir_p()
 *
 *  Creates the directory (and any missing parents). Returns 0 on
 *  success, -1 on failure.
 */

static int
mkdir_p(const char *path, mode_t mode)
{
  size_t len = strlen(path);
  char tmp[4096];

  if(len >= sizeof(tmp))
    return -1;
  strcpy(tmp, path);

  for(char *p = tmp + 1; *p != '\0'; p++)
  {
    if(*p == '/')
    {
      *p = '\0';
      if(mkdir(tmp, mode) != 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
  }
  if(mkdir(tmp, mode) != 0 && errno != EEXIST)
    return -1;
  return 0;
}

/*------------------------------------------------------------------*/

/*  xdg_config_path()
 *
 *  Returns the canonical XDG user config path for lpsk31rc
 *  (or NULL if it cannot be computed).
 */

static int
xdg_config_path(char *out, size_t cap)
{
  const char *xdg = getenv("XDG_CONFIG_HOME");
  const char *home = getenv("HOME");

  if(xdg == NULL || xdg[0] == '\0')
  {
    if(home == NULL || home[0] == '\0')
      return 0;
    return snprintf(out, cap, "%s/.config/lpsk31/lpsk31rc", home) > 0;
  }
  return snprintf(out, cap, "%s/lpsk31/lpsk31rc", xdg) > 0;
}

/*------------------------------------------------------------------*/

/*  migrate_legacy_config()
 *
 *  Best-effort migration of a legacy ~/lpsk31/lpsk31rc file to the
 *  XDG user config location. Writes a new-format config file.
 *  Silently continues (with a notice) if anything fails.
 */

static void
migrate_legacy_config(const char *legacy_path, const char *xdg_path)
{
  char dir[4096];
  FILE *out;

  /* Compute the directory and create it if needed */
  bounded_strncpy(dir, sizeof(dir), xdg_path);
  {
    char *slash = strrchr(dir, '/');
    if(slash == NULL)
      return;
    *slash = '\0';
  }

  /* Create intermediate directories as needed */
  if(mkdir_p(dir, 0755) != 0)
  {
    fprintf(stderr,
            "lpsk31: Could not create config directory %s: %s\n",
            dir, strerror(errno));
    return;
  }

  out = fopen(xdg_path, "w");
  if(out == NULL)
  {
    fprintf(stderr, "lpsk31: Could not write config file %s: %s\n",
            xdg_path, strerror(errno));
    return;
  }

  fprintf(out,
          "# lpsk31 runtime configuration file\n"
          "# (migrated from legacy config file %s)\n"
          "#\n"
          "# See README / doc for format details.\n\n",
          legacy_path);

  fprintf(out, "[sound]\n");
  fprintf(out, "card = %s\n", rc_data.snd_card);
  fprintf(out, "capture-source = %s\n",
          rc_data.cap_src[0] ? rc_data.cap_src : "--");
  fprintf(out, "capture-volume = %s\n",
          rc_data.cap_vol[0] ? rc_data.cap_vol : "--");
  fprintf(out, "master-volume = %s\n",
          rc_data.mst_vol[0] ? rc_data.mst_vol : "--");
  fprintf(out, "pcm-volume = %s\n",
          rc_data.pcm_vol[0] ? rc_data.pcm_vol : "--");
  fprintf(out, "capture-level = %d\n", rc_data.cap_lev);
  fprintf(out, "playback-level = %d\n", rc_data.pbk_lev);
  fprintf(out, "channel = %d\n", rc_data.channel);
  fprintf(out, "channels = %d\n", rc_data.num_chn);
  fprintf(out, "use-channel = %d\n\n", rc_data.use_chn);

  fprintf(out, "[runtime]\n");
  fprintf(out, "squelch = %d\n", rc_data.sqlch_thr);
  fprintf(out, "tone-duration = %d\n", rc_data.tone_dur);
  fprintf(out, "num-reversals = %d\n", rc_data.num_rev);
  fprintf(out, "wpm = %d\n", rc_data.wpm);
  fprintf(out, "word-wrap = %d\n\n", rc_data.wwrap);

  fprintf(out, "[rig]\n");
  fprintf(out, "device = %s\n",
          rc_data.rig_device[0] ? rc_data.rig_device : "127.0.0.1:4532");
  if(rc_data.rig_model[0]) {
    fprintf(out, "model = %s\n", rc_data.rig_model);
  } else {
    fprintf(out, "# No hamlib model set. If using a rigctld server, leave\n");
    fprintf(out, "# device as host:port and model blank. If using a direct\n");
    fprintf(out, "# connection, set model to the hamlib model ID for your rig\n");
    fprintf(out, "# (e.g. 3073 for Icom IC-7300).\n");
  }
  fprintf(out, "enabled = false\n\n");

  fprintf(out, "[operator]\n");
  fprintf(out, "call = %s\n", op_data.call);
  fprintf(out, "name = %s\n", op_data.name);
  fprintf(out, "qth = %s\n", op_data.qth);
  fprintf(out, "loc = %s\n\n", op_data.loc);

  for(size_t i = 0; i < NUM_OF_LABELS; i++)
  {
    size_t text_len;
    char *text;

    if(macro[i] == NULL)
      continue;

    fprintf(out, "[macro-%zu]\n", i + 1);
    fprintf(out, "label = %s\n", label[i]);
    fprintf(out, "text =\n");

    /* Emit the text line by line, stripping the trailing LF */
    text = macro[i];
    text_len = strlen(text);
    {
      size_t pos = 0;
      while(pos < text_len)
      {
        size_t end = pos;
        while(end < text_len && text[end] != '\n')
          end++;
        fprintf(out, "%.*s\n", (int)(end - pos), text + pos);
        pos = end + 1;
      }
    }
    fprintf(out, "\n");
  }

  fclose(out);

  fprintf(stderr,
          "lpsk31: Migrated legacy config to %s\n"
          "       Legacy file left intact at %s.\n",
          xdg_path, legacy_path);
}

/*------------------------------------------------------------------*/


/*------------------------------------------------------------------*/

/*  Macro commit state */

static char pending_label[128] = "";
static char pending_text[16384] = "";
static int pending_macro_idx = -1; /* 0..8, -1 = no macro section in progress */
static int pending_text_active = 0;

/* Commit the pending macro text (and label) to macro[]/label[].
 * Called when a new section starts or at end of file. */
static void macro_text_finalize(const char *fpath) {
  size_t idx;
  int tg_cnt = 0;
  char *p;

  if(pending_macro_idx < 0)
    return;

  idx = (size_t)pending_macro_idx;

  if(!pending_text_active) {
    fprintf(stderr,
            "lpsk31: Macro '%s' in %s has no text - aborting\n",
            pending_label, fpath);
    exit(-12);
  }

  pending_text_active = 0;

  /* Validate tag balance over the text */
  for(p = pending_text; *p != '\0'; p++)
    if(*p == '<' || *p == '>')
      tg_cnt++;

  if(tg_cnt & 1) {
    fprintf(stderr,
            "lpsk31: Bad tag balance in macro '%s' in %s - aborting\n",
            pending_label, fpath);
    exit(-12);
  }

  /* Record label */
  snprintf(label[idx], sizeof(label[idx]), "%.16s", pending_label);

  /* Free old buffer if any */
  if(macro[idx] != NULL)
    free(macro[idx]);

  macro[idx] = strdup(pending_text);
  if(macro[idx] == NULL) {
    perror("lpsk31");
    fprintf(stderr, "lpsk31: memory allocation failed - aborting\n");
    exit(-13);
  }

  pending_label[0] = '\0';
  pending_text[0] = '\0';
  pending_macro_idx = -1;
}

/*------------------------------------------------------------------*/

/*  set_value()
 *
 *  Applies a parsed "key = value" pair in the current section to
 *  the runtime config data. Returns 0 on success, -1 on error.
 */

static int set_value(const char *section, const char *key, const char *val,
                     const char *fpath) {
  if(strcmp(section, "sound") == 0) {
    if(strcmp(key, "card") == 0)
      bounded_strncpy(rc_data.snd_card, sizeof(rc_data.snd_card), val);
    else if(strcmp(key, "capture-source") == 0)
      bounded_strncpy(rc_data.cap_src, sizeof(rc_data.cap_src), val);
    else if(strcmp(key, "capture-volume") == 0)
      bounded_strncpy(rc_data.cap_vol, sizeof(rc_data.cap_vol), val);
    else if(strcmp(key, "master-volume") == 0)
      bounded_strncpy(rc_data.mst_vol, sizeof(rc_data.mst_vol), val);
    else if(strcmp(key, "pcm-volume") == 0)
      bounded_strncpy(rc_data.pcm_vol, sizeof(rc_data.pcm_vol), val);
    else if(strcmp(key, "capture-level") == 0)
      rc_data.cap_lev = atoi(val);
    else if(strcmp(key, "playback-level") == 0)
      rc_data.pbk_lev = atoi(val);
    else if(strcmp(key, "channel") == 0)
      rc_data.channel = atoi(val);
    else if(strcmp(key, "channels") == 0)
      rc_data.num_chn = atoi(val);
    else if(strcmp(key, "use-channel") == 0)
      rc_data.use_chn = atoi(val);
    else {
      fprintf(stderr, "lpsk31: Unknown sound key '%s' in %s\n", key, fpath);
      return -1;
    }
    return 0;
  }

  if(strcmp(section, "runtime") == 0) {
    if(strcmp(key, "squelch") == 0)
      rc_data.sqlch_thr = atoi(val);
    else if(strcmp(key, "tone-duration") == 0)
      rc_data.tone_dur = atoi(val);
    else if(strcmp(key, "num-reversals") == 0)
      rc_data.num_rev = atoi(val);
    else if(strcmp(key, "wpm") == 0)
      rc_data.wpm = atoi(val);
    else if(strcmp(key, "word-wrap") == 0)
      rc_data.wwrap = atoi(val);
    else {
      fprintf(stderr, "lpsk31: Unknown runtime key '%s' in %s\n", key, fpath);
      return -1;
    }
    return 0;
  }

  if(strcmp(section, "rig") == 0) {
    if(strcmp(key, "device") == 0) {
      bounded_strncpy(rc_data.rig_device, sizeof(rc_data.rig_device), val);
    } else if(strcmp(key, "model") == 0) {
      bounded_strncpy(rc_data.rig_model, sizeof(rc_data.rig_model), val);
    } else if(strcmp(key, "enabled") == 0) {
      if(strcmp(val, "true") == 0 || strcmp(val, "yes") == 0 ||
         strcmp(val, "1") == 0 || strcmp(val, "on") == 0)
        rc_data.rig_enabled_default = 1;
      else
        rc_data.rig_enabled_default = 0;
    } else {
      fprintf(stderr, "lpsk31: Unknown rig key '%s' in %s\n", key, fpath);
      return -1;
    }
    return 0;
  }

  if(strcmp(section, "operator") == 0) {
    if(strcmp(key, "call") == 0)
      bounded_strncpy(op_data.call, sizeof(op_data.call), val);
    else if(strcmp(key, "name") == 0)
      bounded_strncpy(op_data.name, sizeof(op_data.name), val);
    else if(strcmp(key, "qth") == 0)
      bounded_strncpy(op_data.qth, sizeof(op_data.qth), val);
    else if(strcmp(key, "loc") == 0)
      bounded_strncpy(op_data.loc, sizeof(op_data.loc), val);
    else {
      fprintf(stderr, "lpsk31: Unknown operator key '%s' in %s\n", key,
              fpath);
      return -1;
    }
    return 0;
  }

  fprintf(stderr, "lpsk31: Unknown section '[%s]' in %s\n", section, fpath);
  return -1;
}

/*------------------------------------------------------------------*/

/*  finalize_runtime()
 *
 *  Called after config parsing. Computes derived values, makes sine/
 *  cosine tables and prints a warning if operator fields are missing.
 */

static void finalize_runtime(const char *fpath) {
  /* Operator fields missing — warn (not fatal) */
  if(op_data.call[0] == '\0' || op_data.name[0] == '\0' ||
     op_data.qth[0] == '\0' || op_data.loc[0] == '\0') {
    fprintf(stderr,
            "lpsk31: Warning: operator fields not set in %s\n"
            "        Macros using <own-call> etc. will expand to empty strings.\n",
            fpath);
  }

  /* Duration of half of a PSK pulse, in DSP samples */
  rc_data.psk_elem = (int)((double)DSP_RATE / BAUD_RATE);
  rc_data.psk_elem2 = rc_data.psk_elem / 2;

  /* Duration of preamble tone in dsp samples */
  rc_data.tone_period = DSP_RATE / AUDIO_FREQUENCY;

  /* Make cosine & sine wave table */
  Make_Cosine_Table();
  Make_Sine_Table();
}

/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/

/*  Load_Config()
 *
 *  Loads the lpsk31rc configuration file (key = value format) and
 *  initializes runtime configuration data.
 */

void Load_Config(void) {
  int origin;
  size_t i;
  char rc_fpath[4096], section[64], line[4096], key[128], val[4096];
  FILE *lpsk31rc;
  /* Local defaults */
  rc_data.snd_card[0] = '\0';
  rc_data.cap_src[0] = '\0';
  rc_data.cap_vol[0] = '\0';
  rc_data.mst_vol[0] = '\0';
  rc_data.pcm_vol[0] = '\0';
  rc_data.channel = 1;
  rc_data.num_chn = 2;
  rc_data.use_chn = 1;
  rc_data.cap_lev = 50;
  rc_data.pbk_lev = 50;
  rc_data.sqlch_thr = 5;
  rc_data.tone_dur = 1;
  rc_data.num_rev = 24;
  rc_data.wpm = 20;
  rc_data.wwrap = 70;
  rc_data.rig_device[0] = '\0';
  rc_data.rig_model[0] = '\0';
  rc_data.rig_enabled_default = 0;

  op_data.call[0] = '\0';
  op_data.name[0] = '\0';
  op_data.qth[0] = '\0';
  op_data.loc[0] = '\0';

  for(i = 0; i < NUM_OF_LABELS; i++)
    macro[i] = NULL;

  pending_label[0] = '\0';
  pending_text[0] = '\0';
  pending_macro_idx = -1;
  pending_text_active = 0;

  /* Resolve and open the config file */
  if(!config_resolve(rc_fpath, sizeof(rc_fpath), &origin)) {
    fprintf(stderr,
            "lpsk31: No config file found.\n"
            "  Looked in:\n"
            "    $XDG_CONFIG_HOME/lpsk31/lpsk31rc\n"
            "    ${XDG_CONFIG_DIRS:-/etc/xdg}/lpsk31/lpsk31rc\n"
            "    ~/lpsk31/lpsk31rc (legacy)\n"
            "  Run 'make install-config' to create one, then edit it.\n");
    exit(-1);
  }

  // Legacy config remappet: parse in-memory, then migrate best-effort,
  // then finalize and return.
  if(origin == 3) {
    char xdg_path[4096];

    if(!xdg_config_path(xdg_path, sizeof(xdg_path))) {
      /* Cannot compute XDG path; fall back to legacy-only parsing
         and skip migration. */
      fprintf(stderr,
              "lpsk31: Using legacy config file %s.\n"
              "       Could not determine the XDG user config path.\n"
              "       Migrate manually by copying it to ~/.config/lpsk31/lpsk31rc\n",
              rc_fpath);
    } else {
      fprintf(stderr,
              "lpsk31: Legacy config file %s detected.\n"
              "       Migrating to %s (legacy file kept intact).\n",
              rc_fpath, xdg_path);
    }

    /* Parse legacy file in memory */
    {
      FILE *fp = fopen(rc_fpath, "r");
      if(fp == NULL) {
        perror(rc_fpath);
        exit(-1);
      }
      legacy_parse(fp, rc_fpath);
      fclose(fp);
    }

    /* Best-effort migration to the XDG config path */
    if(xdg_config_path(xdg_path, sizeof(xdg_path)))
      migrate_legacy_config(rc_fpath, xdg_path);

    finalize_runtime(rc_fpath);
    return;
  }

  lpsk31rc = fopen(rc_fpath, "r");
  if(lpsk31rc == NULL) {
    perror(rc_fpath);
    exit(-1);
  }

  section[0] = '\0';

  while(fgets(line, sizeof(line), lpsk31rc) != NULL) {
    char *p;
    size_t line_len;

    /* Detect long lines that would be truncated */
    line_len = strlen(line);
    if(line_len > 0 && line[line_len - 1] != '\n' &&
       line[line_len - 1] != '\r') {
      fprintf(stderr,
              "lpsk31: Line too long in %s (lines limited to %d chars)\n",
              rc_fpath, (int)(sizeof(line) - 1));
      goto bad_format;
    }

    line_trim(line);
    p = line_lstrip(line);

    if(*p == '\0' || *p == '#')
      continue;

    /* Section header? */
    if(*p == '[') {
      char *close = strchr(p, ']');
      char *name;

      if(close == NULL) {
        fprintf(stderr, "lpsk31: Bad section header in %s: %s\n", rc_fpath,
                line);
        goto bad_format;
      }
      *close = '\0';

      name = line_lstrip(p + 1);
      line_trim(name);

      /* Finalize any pending macro text before leaving a section */
      macro_text_finalize(rc_fpath);

      strncpy(section, name, sizeof(section) - 1);
      section[sizeof(section) - 1] = '\0';
      line_trim(section);

      /* Validate section name */
      if(strcmp(section, "sound") == 0 ||
         strcmp(section, "runtime") == 0 ||
         strcmp(section, "rig") == 0 ||
         strcmp(section, "operator") == 0) {
        /* known sections */
      } else if(strncmp(section, "macro-", 6) == 0 &&
                strlen(section) == 7) {
        const char *digits = section + 6;
        if(!(digits[0] >= '1' && digits[0] <= '9' && digits[1] == '\0')) {
          fprintf(stderr,
                  "lpsk31: Invalid macro section name '[%s]' in %s\n",
                  section, rc_fpath);
          goto bad_format;
        }
      } else {
        fprintf(stderr, "lpsk31: Unknown section '[%s]' in %s\n", section,
                rc_fpath);
        goto bad_format;
      }

      continue;
    }

    /* Inside a macro text block: every non-directive line is text */
    if(pending_text_active) {
      const char *q = line_lstrip(line);

      if((strncmp(q, "label", 5) == 0 &&
          (q[5] == ' ' || q[5] == '\t' || q[5] == '=')) ||
         (strncmp(q, "text", 4) == 0 &&
          (q[4] == ' ' || q[4] == '\t' || q[4] == '='))) {
        fprintf(stderr, "lpsk31: Unexpected key '%s' inside macro text in %s\n",
                q, rc_fpath);
        goto bad_format;
      }

      {
        size_t cur_len = (size_t)strlen(pending_text);
        size_t add_len = strlen(q);

        if(cur_len + add_len + 2 >= sizeof(pending_text)) {
          fprintf(stderr, "lpsk31: Macro text too long in %s\n", rc_fpath);
          goto bad_format;
        }
        strcpy(pending_text + cur_len, q);
        pending_text[cur_len + add_len] = LF;
        pending_text[cur_len + add_len + 1] = '\0';
      }

      continue;
    }

    /* Parse "key = value" */
    {
      char *eq = strchr(p, '=');
      size_t key_len;
      char *val_ptr;

      if(eq == NULL) {
        fprintf(stderr,
                "lpsk31: Invalid line in %s (expected key = value): %s\n",
                rc_fpath, line);
        goto bad_format;
      }

      key_len = (size_t)(eq - p);
      if(key_len == 0 || key_len >= sizeof(key)) {
        fprintf(stderr, "lpsk31: Invalid key in %s: %s\n", rc_fpath, line);
        goto bad_format;
      }
      memcpy(key, p, key_len);
      key[key_len] = '\0';
      line_trim(key);
      {
        char *kp = line_lstrip(key);
        if(kp != key)
          memmove(key, kp, strlen(kp) + 1);
      }

      /* Value: trim, then extract to val */
      val_ptr = line_lstrip(eq + 1);
      line_trim(val_ptr);
      strncpy(val, val_ptr, sizeof(val) - 1);
      val[sizeof(val) - 1] = '\0';

      /* Known sections */
      if(strcmp(section, "sound") == 0 ||
         strcmp(section, "runtime") == 0 ||
         strcmp(section, "rig") == 0 ||
         strcmp(section, "operator") == 0) {
        if(set_value(section, key, val, rc_fpath) != 0)
          goto bad_format;
        continue;
      }

      /* Macro sections */
      if(strncmp(section, "macro-", 6) == 0) {
        int mi = (int)(section[6] - '0') - 1;
        if(strcmp(key, "label") == 0) {
          if(pending_macro_idx != -1) {
            fprintf(stderr,
                    "lpsk31: Label set twice for macro %s in %s\n", section,
                    rc_fpath);
            goto bad_format;
          }
          if(strlen(val) == 0) {
            fprintf(stderr,
                    "lpsk31: Empty label in macro section %s\n", section);
            goto bad_format;
          }
          if(strlen(val) > 16) {
            fprintf(stderr,
                    "lpsk31: Label too long in macro section %s (max 16 chars)\n",
                    section);
            goto bad_format;
          }
          snprintf(pending_label, sizeof(pending_label), "%s", val);
          pending_macro_idx = mi;
          continue;
        } else if(strcmp(key, "text") == 0) {
          if(pending_macro_idx == -1) {
            fprintf(stderr,
                    "lpsk31: Macro text in %s but no label was set first\n",
                    section);
            goto bad_format;
          }
          if(pending_text_active) {
            fprintf(stderr,
                    "lpsk31: Macro text in %s but text block already started\n",
                    section);
            goto bad_format;
          }
          pending_text_active = 1;
          pending_text[0] = '\0';
          /* Initial value on the text = line becomes the first text line */
          if(strlen(val) > 0) {
            size_t vl = strlen(val);
            if(vl + 2 >= sizeof(pending_text)) {
              fprintf(stderr, "lpsk31: Macro text too long in %s\n",
                      rc_fpath);
              goto bad_format;
            }
            strcpy(pending_text, val);
            pending_text[vl] = LF;
            pending_text[vl + 1] = '\0';
          }
          continue;
        } else {
          fprintf(stderr,
                  "lpsk31: Unknown macro key '%s' in section %s (expected label or text)\n",
                  key, section);
          goto bad_format;
        }
      }

      /* Empty or unknown section — should not happen */
      fprintf(stderr,
              "lpsk31: Unexpected line in section '[%s]': %s\n",
              (section[0] != '\0') ? section : "(none)", line);
      goto bad_format;
    }
  } /* while fgets() */

  /* Finalize any pending macro text */
  macro_text_finalize(rc_fpath);

  pending_label[0] = '\0';
  pending_text[0] = '\0';
  pending_macro_idx = -1;
  pending_text_active = 0;

  fclose(lpsk31rc);

  finalize_runtime(rc_fpath);

  return;

bad_format:
  fclose(lpsk31rc);
  fprintf(stderr, "lpsk31: Bad config file format - aborting\n");
  exit(-12);
}

/*------------------------------------------------------------------*/
/*------------------------------------------------------------------*/

/*  Usage()
 *
 *  Prints usage information
 */

void Usage(void) {
  fprintf(stderr, "%s\n\n", "Usage: lpsk31 [-hv]");

  fprintf(stderr, "%s\n\n",
          "       -h: Print this usage information and exit.");

  fprintf(stderr, "%s\n\n", "       -v: Print version number and exit.");

} /* End of Usage() */

/*------------------------------------------------------------------------*/

/*  Cleanup()
 *
 *  Cleans up screen and terminates ncurses
 */

void Cleanup(void) {
  /* Cleanup screen and terminate ncurses */
  if(isFlagSet(NCURSES_INIT)) {
    clear();
    refresh();
    endwin();
  }

  /* Disconnect from the rig */
  if(isFlagSet(ENABLE_RIG))
    rig_disconnect();

  /* Close open files */
  if(qso_record_fp != NULL)
    fclose(qso_record_fp);
  if(log_adif_fp != NULL)
    fclose(log_adif_fp);
  if(log_fp != NULL)
    fclose(log_fp);

} /*  Cleanup( void ) */

/*------------------------------------------------------------------------*/

/*  sig_handler()
 *
 *  Signal Action Handler function
 */

static void sig_handler(int signal) {
  /* Cleanup and abort */
  Cleanup();

  fprintf(stderr, "\n");
  switch(signal) {
  case SIGINT:
    fprintf(stderr, "%s\n", "lpsk31: Exiting via User Interrupt");
    exit(signal);

  case SIGSEGV:
    fprintf(stderr, "%s\n", "lpsk31: Segmentation Fault");
    exit(signal);

  case SIGFPE:
    fprintf(stderr, "%s\n", "lpsk31: Floating Point Exception");
    exit(signal);

  case SIGTERM:
    fprintf(stderr, "%s\n", "lpsk31: Termination Request received");
    exit(signal);

  case SIGABRT:
    fprintf(stderr, "%s\n", "lpsk31: Abort Signal received");
    exit(signal);

  } /* switch( signal ) */

} /* End of sig_handler() */

/*------------------------------------------------------------------------*/

/*  Abort_On_Error()
 *
 *  Prints an error message and exits
 */

void Abort_On_Error(int why) {
  /* Cleanup and abort */
  Cleanup();

  switch(why) {
  case -1: /* Abort if samples buffer cannot be filled */
    fprintf(stderr, "%s\n",
            "lpsk31: Error filling signal samples buffer - aborting");
    break;

  case -2: /* Abort if recording source selection fails */
    fprintf(stderr, "%s\n",
            "lpsk31: Recording source selection failed - aborting");
    break;

  case -3: /* Abort sample format setting fails */
    fprintf(stderr, "%s\n", "lpsk31: Sample format setting failed - aborting");
    break;

  case -4: /* Abort if sampling speed setting fails */
    fprintf(stderr, "%s\n",
            "lpsk31: DSP sampling speed setting failed - aborting");
    break;

  case -5: /* Abort if recording level setting fails */
    fprintf(stderr, "%s\n",
            "lpsk31: Recording level setting failed - aborting");
    break;

  case -6: /* Abort if samples buffer cannot be filled */
    fprintf(stderr, "%s\n",
            "lpsk31: Recording buffer not filled by read() - aborting");
    break;

  case -7: /* Abort if no recording level device found */
    fprintf(stderr, "%s\n",
            "lpsk31: Recording level device not found - aborting");
    break;

  case -8: /* Abort if Line level setting fails */
    fprintf(stderr, "%s\n",
            "lpsk31: Recording source level setting failed - aborting");
    break;

  case -9: /* Abort if Stereo/Mono mode setting fails */
    fprintf(stderr, "%s\n",
            "lpsk31: Stereo/Mono mode setting failed - aborting");
    break;

  case -10: /* Abort if DSP samples cannot be written to file */
    fprintf(stderr, "%s\n",
            "lpsk31: Error writing DSP samples buffer to file - aborting");
    break;

  case -11: /* Abort if audio samples cannot be written to DSP */
    fprintf(stderr, "%s\n",
            "lpsk31: Error writing audio samples buffer to DSP - aborting");
    break;

  case -12: /* Abort if error reading the configuration file */
    fprintf(stderr, "%s\n",
            "lpsk31: Error reading ~/lpsk31/lpsk31rc - aborting");
    break;

  case -13: /* Abort if malloc() for samples buffer fails */
    fprintf(stderr, "%s\n",
            "lpsk31: A call for memory allocation failed - aborting");
    break;

  case -14: /* Abort if a tag is not identified */
    fprintf(stderr, "%s\n", "lpsk31: Unknown tag in macro  - aborting");
    break;

  case -15: /* Abort if setting PCM or Volume level fails */
    fprintf(stderr, "%s\n",
            "lpsk31: PCM or Volume level setting failed - aborting");

  } /* switch( why ) */

  exit(why);

} /* End of Abort_On_Error() */

/*------------------------------------------------------------------------*/

/* Functions for testing and setting/clearing flow control flags
 *
 *  See lpsk31.h for definition of flow control flags
 */

/* An int variable holding the single-bit flags */
static int Flags = 0;

int isFlagSet(int flag) { return ((Flags & flag) == flag); }

int isFlagClear(int flag) { return ((~Flags & flag) == flag); }

void Set_Flag(int flag) { Flags |= flag; }

void Clear_Flag(int flag) { Flags &= ~flag; }

/*------------------------------------------------------------------------*/
