#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define MAXCHARS 100

/*#define TESTING */

char keylist[50][50];
int keyfields = 0;
int unique = 0;
FILE *infile = NULL;
FILE *outfile = NULL;
FILE *logfile = NULL;

int keywords (char *string);

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

/* Position after 'A' on next line */

void 
newline ()
{
  char string[100];

  while (fscanf (infile, "%s", string) != EOF)
    {
      if (strcmp (string, "A") == 0)
	return;
      if (strcmp (string, "A*") == 0)
	fgets (string, 100, infile);
      else
	keywords (string);
    }
  return;
}

/* Store key fields */

void 
get_keylist ()
{
  int i, j;
  char string[100];

  j = 0;

  while (fscanf (infile, "%s", string) != EOF)
    {
      if (strlen (string) == 1 && strchr (string, 'K') != NULL)
	{
	  fscanf (infile, "%s", string);

	  if (strchr (string, '#') != NULL)
	    {
	      for (i = 0; i < MAXCHARS; i++)
		{
		  if (string[i] == '#')
		    string[i] = '\0';
		}
	    }
	  strcpy (keylist[j], string);
	  j++;
	  keyfields++;
	}
    }
  if (logfile != NULL)
    {
      fprintf (logfile, "Keylist: ");
      for (i = 0; i < j; i++)
	fprintf (logfile, "%s ", keylist[i]);
      fprintf (logfile, "\n");
    }
  rewind (infile);
}



/* Check for keywords */

int 
keywords (char *string)
{
  int i, length;

  if (strcmp (string, "UNIQUE") == 0)
    {
      unique = 1;
      return 1;
    }
  else if (strcmp (string, "R") == 0)
    {
      return 1;
    }
  else if (strstr (string, "TEXT(") != NULL)
    {
      fgets (string, 100, infile);
      length = strlen (string);
      for (i = length; i >= 0; i--)
	if (string[i] == '+')
	  {
	    newline (infile, outfile, logfile);
	    if (logfile != NULL)
	      fprintf (logfile, "Skipping line...\n");
	  }
      if (logfile != NULL)
	fprintf (logfile, "Skipping TEXT\n");
      return 1;
    }
  else if (strstr (string, "COLHDG(") != NULL)
    {
      fgets (string, 100, infile);
      length = strlen (string);
      for (i = length; i >= 0; i--)
	if (string[i] == '+')
	  {
	    newline (infile, outfile, logfile);
	    if (logfile != NULL)
	      fprintf (logfile, "Skipping line...\n");
	  }
      if (logfile != NULL)
	fprintf (logfile, "Skipping COLHDG\n");
      return 1;
    }
  else if (strstr (string, "VALUES(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping VALUES\n");
      return 1;
    }
  else if (strstr (string, "RANGE(") != NULL)
    {
      if (logfile != NULL)
	fprintf (logfile, "Skipping RANGE\n");
      return 1;
    }
  return 0;
}

/*
 *  Show a usage message and exit.
 */
static void
syntax ()
{
  fprintf (stderr, "dds2sql - Convert AS/400 DDS Source to SQL statements.\n\
Syntax:\n\
  dds2sql [options] INPUTFILE ...\n\
\n\
Options:\n\
  -t TRACEFILE      Specify trace file (default: no logging).\n\
  -o OUTPUT         Specify output file (default: standard out).\n\
\n\
Notes:
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
  char string[MAXCHARS];
  int length, dpt, i, j = 0, keyflag = 0;
  long position;
  char *outfilename = NULL, *logfilename = NULL; /* These are used so we can
                                                  * open output and logging
                                                  * files after input file is 
                                                  * opened
                                                  */
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
	  if ((outfile = fopen (optarg,"w")) == NULL)
	    {
	      perror (optarg);
	      exit (1);
	    }
	  break;

	case '?':
	case ':':
	default:
	  syntax ();
	}
    }

  if (optind == argc)
    syntax ();

  for (; optind < argc; optind++)
    {
      if ((infile = fopen (argv[optind], "r")) == NULL)
	{
	  perror (argv[optind]);
	  continue;
	}

      get_keylist (infile, logfile);
      fprintf (outfile, "create table %s (\n", argv[optind]);

      newline (infile, outfile, logfile);

      while (fscanf (infile, "%s", string) != EOF)
	{
	  if (logfile != NULL)
	    fprintf (logfile, "top\n");

	  if (keywords (string))
	    goto iter;

	  if (strchr (string, '#') != NULL)
	    {
	      for (i = 0; i < MAXCHARS; i++)
		{
		  if (string[i] == '#')
		    string[i] = '\0';
		}
	    }


	  /* If this is a key field make it NOT NULL */

	  for (i = 0; i < keyfields; i++)
	    {
	      if (strcmp (string, keylist[i]) == 0)
		{
		  keyflag = 1;
		  break;
		}
	      else
		keyflag = 0;
	    }


	  /* If the is a key field add it the the KEY statement */

	  if (strlen (string) == 1 && strchr (string, 'K') != NULL)
	    {
	      fscanf (infile, "%s", string);

	      if (strchr (string, '#') != NULL)
		{
		  for (i = 0; i < MAXCHARS; i++)
		    {
		      if (string[i] == '#')
			string[i] = '\0';
		    }
		}
	      if (unique)
		fprintf (outfile, "UNIQUE(%s", strlwr (string));
	      else
		fprintf (outfile, "KEY(%s", strlwr (string));
	      j = 1;
	      newline (infile, outfile, logfile);

	      while (fscanf (infile, "%s", string) != EOF)
		{
		  if (strlen (string) == 1 && strchr (string, 'K') != NULL);
		  else
		    {

		      if (strchr (string, '#') != NULL)
			{
			  for (i = 0; i < MAXCHARS; i++)
			    {
			      if (string[i] == '#')
				string[i] = '\0';
			    }
			}
		      fprintf (outfile, ", %s", strlwr (string));
		      newline (infile, outfile, logfile);
		      j++;
		    }
		}
	      fprintf (outfile, "))\n");
	      fclose (infile);
	      fclose (outfile);
	      return 0;
	    }



	  /* print out the field information */

	  fprintf (outfile, "%s", strlwr (string));
	  position = ftell (infile);
	  fscanf (infile, "%s", string);

	  /* Check to see if this is a date type */

	  if (strcmp (string, "L") == 0)
	    fprintf (outfile, " DATE");
	  else
	    {
	      fseek (infile, position, SEEK_SET);

	      if (fscanf (infile, "%d", &length) != EOF)
		{
		  fscanf (infile, "%s", string);

		  /* If this is a decimal or packed field use DECIMAL type */

		  if (strcmp (string, "S") == 0 || strcmp (string, "P") == 0)
		    {
		      fscanf (infile, "%d", &dpt);
		      fprintf (outfile, " DECIMAL(%d, %d)", length, dpt);
		    }

		  /* If this is an alpha field use CHAR type */

		  else if (strcmp (string, "A") == 0)
		    {
		      fprintf (outfile, " CHAR(%d)", length);
		    }
		}
	    }

	  if (keyflag)
	    fprintf (outfile, " NOT NULL,\n");
	  else
	    fprintf (outfile, ",\n");
	iter:newline (infile, outfile, logfile);
	}

      fclose (infile);
    }

  if (outfile != stdout)
    fclose (outfile);
  return 0;
}
