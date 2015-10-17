#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "csv.h"

enum { NOMEM = -2 };             /* value denotes running out of memory */
static int  max_line    = 0;     /* the size of the line */
static int  max_field   = 0;     /* the size of the field */
static int  nfields     = 0;     /* the number of fields in field[] */
static int  h_count     = 0;     /* counter to check if line is header */
static char *line       = NULL;  /* chars in line */
static char **field     = NULL;  /* field pointers */
static char *sep_line   = NULL;  /* copy of line split by split() */
static char **headers   = NULL;  /* array of header fields */
static char field_sep[] = ",";   /* field separator chars if changed */

static char *advquoted(char *);
static int split(void);

/* Perform a check for end of line or end of file */
static int endofline(FILE *fin, int c) 
{
  int eol;

  eol = (c == '\r' || c == '\n');
  if (c == '\r') {
    c = getc(fin);
    if (c != '\n' && c != EOF) ungetc(c, fin); 
  }

  return eol;
}

/* Reset to starting values */
static void reset(void) 
{
  free(line);           
  free(sep_line);
  free(field);

  line     = NULL;
  sep_line = NULL;
  field    = NULL;
  max_line = max_field = nfields = 0;
}

/* Get one line */
char *csvgetline(FILE *fin)
{
  int i, c;
  char *new_line, *new_sep;

  if (line == NULL) {      /* allocate on first call */
    max_line = max_field = 1;
    line     = (char *)  malloc(max_line);
    sep_line = (char *)  malloc(max_line);
    field    = (char **) malloc(max_field * sizeof(field[0]));

    if (line == NULL || sep_line == NULL || field == NULL) {
      reset();
      return NULL;         /* out of memory */
    }
  }

  for (i = 0; (c = getc(fin)) != EOF && !endofline(fin,c); i++) {
    if (i >= max_line - 1) {  
      max_line *= 2;          
      new_line = (char *) realloc(line, max_line);

      if (new_line == NULL) {
        reset();
        return NULL;
      }

      line = new_line;
      new_sep = (char *) realloc(sep_line, max_line);

      if (new_sep == NULL) {
        reset();
        return NULL;
      }

      sep_line = new_sep;
    }

    line[i] = c;
  }

  line[i] = '\0';
  if (split() == NOMEM) {
    reset();
    return NULL; /* out of memory */
  }

  /* If first line of csv, create the header array */
  if (h_count == 0) {
    headers = (char **) malloc(nfields);
  }

  return (c == EOF && i == 0) ? NULL : line;
}

/* Splitting lines into fields */
static int split(void)
{
  char *p, **new_field;
  char *temp_sep;      /* pointer to temporary separator character */
  int temp_sep_char;   /* temporary separator character */

  nfields = 0;
  if (line[0] == '\0') return 0;
  strcpy(sep_line, line);
  p = sep_line;

  do {
    if (nfields >= max_field) {
      max_field *= 2;         /* double current size */
      new_field  = (char **) realloc(field,
                   max_field * sizeof(field[0]));

      if (new_field == NULL) return NOMEM;
      field = new_field;
    }

    if (*p == '"') {
      temp_sep = advquoted(++p);
    } else {
      temp_sep = p + strcspn(p, field_sep);
    }

    temp_sep_char = temp_sep[0];
    temp_sep[0] = '\0';           /* terminate field */
    field[nfields++] = p;
    p = temp_sep + 1;
  } while (temp_sep_char == ',');

  return nfields;
}

/* Return pointer to next separator */
static char *advquoted(char *p)
{
  int i, j, k;

  for (i = j = 0; p[j] != '\0'; i++, j++) {
    if (p[j] == '"' && p[++j] != '"') {
      /* copy up to next separator or \0 */
      k = strcspn(p+j, field_sep);
      memmove(p+i, p+j, k);
      i += k;
      j += k;
      break;
    }

    p[i] = p[j];
  }

  p[i] = '\0';
  return p + j;
}

/* Return pointer to n-th field */
char *csvfield(int n)
{
  if (n < 0 || n >= nfields) return NULL;

  /* Assigning headers, the header array, if this is the first iteration */
  if (h_count == 0) headers[n] = strdup(field[n]);

  return field[n];
}

/* Get number of fields */
int csvnfield(void)
{
  return nfields;
}

/* Printing in JSON format to console */
void json_print(int x)
{
  int y;

  if (x != 1) printf("\n  },  \n");
  printf("  Row %d: {", x);

  for (y = 0; y < nfields - 2; y++) {
    printf("\n    %s: %s,", headers[y], csvfield(y));
  }

  printf("\n    %s: %s", headers[nfields-1], csvfield(nfields-1));
}

int main(void)
{
  int i;
  int x = 0;
  char *line;

  while ((line = csvgetline(stdin)) != NULL) {
    if (x == 0) {
      for (i = 0; i < csvnfield(); i++) {
        csvfield(i);
      }

      printf("{\n");
    } else {
      json_print(x);
    }

    if (x == 0) h_count = 1;
    x++;
  }

  printf("\n  }  \n}\n");
  return 0;
}
