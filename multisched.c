#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define MSG(x...) fprintf (stderr, x)
#define STRERROR  strerror (errno)
#define COMMAND_LEN 256

#define ID_LEN 2
#define TIME_LEN 2
#define PRIORITY_LEN 2

static int check_valid_id(const char*);
static int lookup_task(const char*);
static int check_valid_arrive_time(const char*);
static int check_valid_service_time(const char*);
static int check_valid_priority(const char*);

static int check_valid_id(const char *str) {

  size_t len;
  int    i;

  len = strlen (str);
  if (len != ID_LEN)
    return -1;

  if (!(isupper(str[0]) && isdigit(str[1]))) 
    return -1;

  return 0;
}

static int lookup_task(const char *str) {

  return 0;
}

static int check_valid_arrive_time(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > TIME_LEN)
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < 0 || val > 30)
    return -1;

  return 0;
}
static int check_valid_service_time(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > TIME_LEN)
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < 1 || val > 30)
    return -1;
 
  return 0;
}
static int check_valid_priority(const char *str) {

  size_t len;
  int val;

  len = strlen(str);
  if (len > PRIORITY_LEN)
    return -1;

  for (int i = 0; i < len; i++) {
    if (!isdigit(str[i]))
      return -1;
  }

  val = atoi(str);
  if (val < 1 || val > 10)
    return -1;

  return 0;
}
static char *
strstrip (char *str)
{
  char  *start;
  size_t len;

  len = strlen (str);
  while (len--)
  {
    if (!isspace (str[len]))
      break;
    str[len] = '\0';
  }

  for (start = str; *start && isspace (*start); start++);
  memmove (str, start, strlen (start) + 1);

  return str;
}

static int read_config(const char* filename) {

  FILE *fp;
  char line[COMMAND_LEN * 2];
  int line_nr = 0;

  fp = fopen (filename, "r");
  if (!fp)
    return -1;

  while (fgets(line, sizeof(line), fp)) {
    char* p;
    char* s;
    size_t len;

    line_nr++;

    len = strlen (line);
    if (line[len - 1] == '\n')
      line[len - 1] = '\0';

    /* comment or empty line */
    if (line[0] == '#' || line[0] == '\0')
      continue;

    /* id */
    s = line;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    if (check_valid_id (s))
    {
      MSG ("invalid id '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    if (lookup_task (s))
    {
      MSG ("duplicate id '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    //TODO: make task structure
    //MSG ("id is %s", s);
    //strcpy (task.id, s);

    /* process-type */
    s = p + 1;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    //TODO: assign process-type here
    if (!strcasecmp (s, "H")) {
      //MSG("process-type is H");
      //task.action = ACTION_ONCE;
    }
    else if (!strcasecmp (s, "M")) {
      //MSG("process-type is M");
      //task.action = ACTION_RESPAWN;
    }
    else if (!strcasecmp (s, "L")) {
      //MSG("process-type is L");
      //something
    }
    else
    {
      MSG ("invalid action '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    //TODO: add process-type

    /* arrive-time */
    s = p + 1;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    if (check_valid_arrive_time(s)) {
      MSG ("invalid arrive_time '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    //TODO: add arrive-time
    //MSG(" arrive-time is %s", s);

    /* service-time */
    s = p + 1;
    p = strchr (s, ' ');
    if (!p)
      goto invalid_line;
    *p = '\0';
    strstrip (s);
    if (check_valid_service_time(s)) {
      MSG ("invalid service_time '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    //TODO: add service-time
    //MSG(" service-time is %s", s);

    /* priority */
    s = p + 1;
    strstrip(s);
    if (s[0] == '\0')
    {
      MSG ("empty priority in line %d, ignored\n", line_nr);
      continue;
    }
    if (check_valid_priority(s)) {
      MSG ("invalid priority '%s' in line %d, ignored\n", s, line_nr);
      continue;
    }
    //TODO: add priority
    //MSG(" priority is %s", s);
    //MSG("\n");

    //TODO: append whole information here

    continue;

invalid_line:
    MSG ("invalid format in line %d, ignored\n", line_nr);
  }

  fclose (fp);


  return 0;

}


int main(int argc, char **argv) {

  if (argc <= 1)
  {
    MSG ("usage: %s input file must specified\n", argv[0]);
    return -1; 
  }

  if (read_config (argv[1]))
  {
    MSG ("failed to load input file '%s': %s\n", argv[1], STRERROR);
    return -1; 
  }




  return 0;

}





