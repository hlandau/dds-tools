#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#define MAXCHARS 150

FILE *infile = NULL;
FILE *outfile = NULL;
FILE *logfile = NULL;

#ifndef HAVE_STRLWR
static char *
strlwr (char *str)
{
  char *scan = str;
  while (scan && *scan)
    *scan++ = tolower(*scan);
  return str;
}
#endif

#ifndef HAVE_STRSTR
static char *
strstr (char *str, char *substr)
{
  char *scan = str;
  while (scan && strlen(scan) <= strlen(substr))
    if (!memcmp (scan, substr, strlen(substr)))
      return scan;
  return NULL;
}
#endif


/* Check for keywords */

int
keywords (char *string)
{

  if (strstr (string, "DSPSIZ(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping DSPSIZ\n");
      return 1;
    }
  else if (strcmp (string, "R") == 0)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping R\n");
      return 1;
    }
  else if (strstr (string, "DSPATR(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping DSPATR\n");
      return 1;
    }
  else if (strstr (string, "ROLLUP(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping ROLLUP\n");
      return 1;
    }
  else if (strstr (string, "ROLLDOWN(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping ROLLDOWN\n");
      return 1;
    }
  else if (strstr (string, "COLOR(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping COLOR\n");
      return 1;
    }
  else if (strstr (string, "OVERLAY") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping OVERLAY\n");
      return 1;
    }
  else if (strstr (string, "SYSNAME") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping SYSNAME\n");
      return 1;
    }
  else if (strstr (string, "CF") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping CF\n");
      return 1;
    }
  return 0;
}

static void
syntax ()
{
  fprintf (stderr, "dds2curses - Convert AS/400 DDS to C+curses code.\n\
Syntax:\n\
  dds2curses [options] INPUTFILE\n\
\n\
Options:\n\
  -t TRACEFILE      Specify the log file (default: no logging).\n\
  -o OUTPUT         Specify the name of the output file (default: stdout).\n\
\n\
Notes:\n\
  INPUTFILE must be an ASCII file.\n\
\n");
  exit (1);
}


int
main (int argc, char *argv[])
{
#ifdef VERSION
  char version[] = VERSION;
#endif
  char line[MAXCHARS];
  char string[MAXCHARS];
  char teststr[MAXCHARS];
  int length = 0, i, j = 0, cur_line, cur_col, cmpltfunc = 0;
  int fieldlen, fieldcount = 0;
  char *outfilename = NULL, *logfilename = NULL; /* These are used so we can
						  * open output and logging
						  * files after input file is 
						  * opened
						  */
  char *infilename = NULL; /* Use this for output message */
  extern char *optarg;
  extern int optind, opterr, optopt;

  outfile = stdout;

  /* Check command line for correctness */

  while ((i = getopt (argc, argv, "t:o:")) != EOF)
    {
      switch (i)
	{
	case 't':
	  logfilename = (char *) malloc (sizeof (char) * (strlen (optarg) + 1));
	  strcpy (logfilename, optarg);
	  logfilename[strlen (optarg)] = '\0';
	  break;

	case 'o':
	  outfilename = (char *) malloc (sizeof (char) * (strlen (optarg) + 1));
	  strcpy (outfilename, optarg);
	  outfilename[strlen (optarg)] = '\0';
	  break;

	default:
	  syntax ();
	}
    }



  /* Always check to make sure that you succeeded in opening the file.
   * We open first the input file, then output, then log.  This way if
   * the input file does not exist (user made a typo or something) we
   * don't create empty output and log files
   */

  if (argc - optind != 1)
    syntax ();

  infilename = (char *) malloc (sizeof (char) * (strlen (argv[optind]) + 1));
  strcpy (infilename, argv[optind]);
  infilename[strlen (argv[optind])] = '\0';

  if ((infile = fopen (infilename, "r")) == NULL)
    {
      perror (infilename);
      exit (1);
    }

  if (outfilename != NULL)
    {
      if ((outfile = fopen (outfilename, "w")) == NULL)
	{
	  perror (outfilename);
	  fclose (infile);
	  exit (1);
	}
    }

  if (logfilename != NULL)
    {
      if ((logfile = fopen (logfilename, "w")) == NULL)
	{
	  perror (logfilename);
	  fclose (infile);
	  if (outfilename != NULL)
	    {
	      fclose (outfile);
	    }
	  exit (1);
	}
    }

#ifdef VERSION
  fprintf (outfile, "/* dds2curses\tversion %s\tby James Rich\n", version);
#else
  fprintf (outfile, "/* dds2curses\tby James Rich\n");
#endif
  fprintf (outfile, " * This is a generated file!\n");
  fprintf (outfile, " * For things to work you need to have the following\n");
  fprintf (outfile, " * definitions somewhere in your program:\n");
  fprintf (outfile, " * \n");
  fprintf (outfile, " * #define MAXCHARS 150\n");
  fprintf (outfile, " * #define ALPHA 0\n");
  fprintf (outfile, " * #define ZONE 1\n");
  fprintf (outfile, " * \n");
  fprintf (outfile, " * typedef struct field_struct{\n");
  fprintf (outfile, " *   int length;\n");
  fprintf (outfile, " *   int attr;\n");
  fprintf (outfile, " *   char value[MAXCHARS];\n");
  fprintf (outfile, " * }field;\n");
  fprintf (outfile, " * \n");
  fprintf (outfile, " * void %s (field *fields);\n", infilename);
  fprintf (outfile, " */\n");


  while (fgets (line, MAXCHARS, infile) != NULL)
    {

      if (strstr (strncpy (teststr, line, 7), "A*") != NULL)
	goto iter;

      if (logfile != NULL)
	fprintf (logfile, "top:\n%s", line);


      /* Check for record format */

      if (line[16] == 'R')
	{
	  for (i = 0; i < 10; i++)
	    {
	      if (line[i + 18] == ' ' || line[i + 18] == 10 || line[i + 18] == 13)
		string[i] = '\0';
	      else
		string[i] = line[i + 18];
	    }
	  if (cmpltfunc)
	    fprintf (outfile, "}\n");
	  fprintf (outfile, "\n\nvoid %s (field *fields)\n{\n",
		   strlwr (string));
	  fprintf (outfile, "  int i;\n  char string[MAXCHARS];\n\n");
	  cmpltfunc = 1;
	  goto iter;
	}


      /* Check for keywords */

      if (strlen (line) >= 44)
	{
	  for (i = 0; i < 10; i++)
	    {
	      if (line[i + 44] == 10 || line[i + 44] == 13)
		break;
	      string[i] = line[i + 44];
	    }
	  string[i] = '\0';

	  if (keywords (string) && line[18] == ' ')
	    goto iter;
	}


      /* find where to start drawing */

      j = 0;
      for (i = 0; i < 3; i++)
	{
	  if (line[i + 38] != ' ')
	    {
	      string[j] = line[i + 38];
	      j++;
	    }
	}
      string[j] = '\0';
      cur_line = atoi (string);

      j = 0;
      for (i = 0; i < 2; i++)
	{
	  if (line[i + 42] != ' ')
	    {
	      string[j] = line[i + 42];
	      j++;
	    }
	}
      string[j] = '\0';
      cur_col = atoi (string);
      if (logfile != NULL)
	fprintf (logfile, "cur_line: %d, cur_col: %d\n", cur_line, cur_col);
      fprintf (outfile, "  move(%d, %d);\n", cur_line, cur_col);



      /* Find any text to draw */

      if (line[44] == 39)
	{
	  for (i = strlen (line); i > 44; i--)
	    if (line[i] == 39)
	      {
		length = i - 44;
		break;
	      }
	  for (i = 0; i < length - 1; i++)
	    string[i] = line[i + 45];
	  string[i] = '\0';
	  fprintf (outfile, "  addstr(\"%s\\n\");\n", string);
	}

      /* find field to draw */

      else if (line[18] != ' ')
	{
	  j = 0;
	  for (i = 0; i < 2; i++)
	    {
	      if (line[i + 32] != ' ')
		{
		  string[j] = line[i + 32];
		  j++;
		}
	    }
	  string[j] = '\0';
	  fieldlen = atoi (string);
	  fprintf (outfile, "  fields[%d].col = %d;\n",
		   fieldcount, cur_col);
	  fprintf (outfile, "  fields[%d].line = %d;\n",
		   fieldcount, cur_line);
	  fprintf (outfile, "  fields[%d].length = %d;\n",
		   fieldcount, fieldlen);
	  if (line[37] == 'B')
	    fprintf (outfile, "  fields[%d].attr = A_UNDERLINE;\n", fieldcount);
	  if (line[34] == 'S')
	    fprintf (outfile, "  fields[%d].type = ZONE;\n", fieldcount);
	  if (line[34] == 'A')
	    fprintf (outfile, "  fields[%d].type = ALPHA;\n", fieldcount);
	  fprintf (outfile, "  strncpy(string, fields[%d].value, fields[%d].length);\n",
		   fieldcount, fieldcount);
	  fprintf (outfile, "  if (strlen(string) != fields[%d].length)\n",
		   fieldcount);
	  fprintf (outfile, "    {\n");
	  fprintf (outfile, "      for ( i=0; i<fields[%d].length; i++)\n",
		   fieldcount);
	  fprintf (outfile, "        {\n");
	  fprintf (outfile, "          if (string[i] == '\\0')\n");
	  fprintf (outfile, "            {\n");
	  fprintf (outfile, "              string[i] = ' ';\n");
	  fprintf (outfile, "            }\n");
	  fprintf (outfile, "        }\n");
	  fprintf (outfile, "      string[i] = '\\0';\n");
	  fprintf (outfile, "    }\n");

	  if (line[37] == 'B')
	    fprintf (outfile, "  attron(A_UNDERLINE);\n");
	  fprintf (outfile, "  printw(\"%%s\\n\", string);\n");
	  if (line[37] == 'B')
	    fprintf (outfile, "  attroff(A_UNDERLINE);\n");
	  fieldcount++;
	}


    iter:
    }

  fprintf (outfile, "}\n");
  fclose (infile);
  fclose (outfile);
  if (logfilename != NULL)
    {
      fclose (logfile);
    }
  return 0;
}
