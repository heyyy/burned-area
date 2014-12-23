#include <getopt.h>
#include "determine_max_extent.h"

/******************************************************************************
MODULE:  determine_max_extent

PURPOSE:  Determine the maximum bounding extent for the input stack of
temporal data products.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred during processing of the max extent
SUCCESS         Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
04/24/2013    Gail Schmidt     Original Development
03/10/2014    Gail Schmidt     Update to work with the ESPA internal file
                               format

NOTES:
******************************************************************************/
int main (int argc, char *argv[])
{
    bool verbose;              /* verbose flag for printing messages */
    char FUNC_NAME[] = "main"; /* function name */
    char errmsg[STR_SIZE];     /* error message */
    char **xml_infile = NULL; /* array to hold list of input XML filenames */
    char *list_infile=NULL;    /* file containing the temporal list of
                                  reflectance products to be processed */
    char *extent_outfile=NULL; /* output file for the maximum extents */

    int i;                     /* looping variable */
    int retval;                /* return status */
    int curr_line;             /* counter of the current line */
    int nlines;                /* number of lines in the input temporal list */
    int nfiles;                /* number of actual files in the input list */
    int count;                 /* count of items read from file */

    double west_coord=-999.0;  /* west bounding coordinate of list */
    double east_coord=-999.0;  /* east bounding coordinate of list */
    double north_coord=-999.0; /* north bounding coordinate of list */
    double south_coord=-999.0; /* south bounding coordinate of list */
    double temp_west_coord;    /* west bounding coordinate of current file */
    double temp_east_coord;    /* east bounding coordinate of current file */
    double temp_north_coord;   /* north bounding coordinate of current file */
    double temp_south_coord;   /* south bounding coordinate of current file */

    FILE *list_fptr=NULL;      /* input file pointer for list of files */
    FILE *extent_fptr=NULL;    /* output file pointer for file extents */

    printf ("Determining maximum extents ...\n");

    /* Read the command-line arguments, including the name of the input
       list of files and the output file to write the bounding extents */
    retval = get_args (argc, argv, &list_infile, &extent_outfile, &verbose);
    if (retval != SUCCESS)
    {   /* get_args already printed the error message */
        exit (ERROR);
    }

    /* Provide user information if verbose is turned on */
    if (verbose)
    {
        printf ("  Input list file: %s\n", list_infile);
        printf ("  Output extents file: %s\n", extent_outfile);
    }

    /* Open the input list of reflectance files */
    list_fptr = fopen (list_infile, "r");
    if (list_fptr == NULL)
    {
        sprintf (errmsg, "Unable to open the input temporal list file: %s",
            list_infile);
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Determine the maximum number of reflectance files in the input file.
       Note the first scanf grabs lines with text.  If that fails, the second
       scanf grabs empty lines.  We'll count all of them for now and then only
       read the non-empty lines. */
    nlines = 0;
    while (EOF != (fscanf (list_fptr, "%*[^\n]"), fscanf (list_fptr, "%*c"))) 
        nlines++;

    /* Allocate memory for the list of filenames */
    xml_infile = (char **) calloc (nlines, sizeof (char *));
    if (xml_infile != NULL)
    {
        for (i = 0; i < nlines; i++)
        {
            xml_infile[i] = (char *) calloc (STR_SIZE, sizeof (char));
            if (xml_infile[i] == NULL)
            {
                sprintf (errmsg, "Error allocating memory for array of %d "
                    "strings to hold the list of XML filenames.", nlines);
                error_handler (true, FUNC_NAME, errmsg);
                fclose (list_fptr);
                exit (ERROR);
            }
        }
    }
    else
    {
        sprintf (errmsg, "Error allocating memory for array of %d strings "
            "to hold the list of reflectance filenames.", nlines);
        error_handler (true, FUNC_NAME, errmsg);
        fclose (list_fptr);
        exit (ERROR);
    }

    /* Read the list of XML files in the input file */
    rewind (list_fptr);
    curr_line = 0;
    for (i = 0; i < nlines; i++)
    {
        count = fscanf (list_fptr, "%s[^\n]", &xml_infile[curr_line][0]);
        if (count == EOF)
            break;
        else if (count == 0)
        { /* blank line so skip and move to the next; don't count the line */
            fscanf (list_fptr, "%*c");
        }
        else
            curr_line++;
    }

    /* Set the nfiles counter if to the actual number of files in the input
       list; this might be different than the count of lines from earlier */
    nfiles = curr_line;

    /* Close the input file */
    fclose (list_fptr);

    /* Loop through each of the input files, read the extent, and compare to
       the current max extent */
    if (verbose)
        printf ("Input list file contains %d filenames\n", nfiles);
    for (i = 0; i < nfiles; i++)
    {
        if (verbose)
            printf ("\nProcessing current file %d: %s\n", i, xml_infile[i]);

        /* Process the current file */
        retval = read_extent (xml_infile[i], &temp_east_coord,
            &temp_west_coord, &temp_north_coord, &temp_south_coord);
        if (retval != SUCCESS)
        {  /* trouble processing this file so skip and go to the next one */
            sprintf (errmsg, "Error processing file %s.  Skipping and moving "
                "to the next file.", xml_infile[i]);
            error_handler (false, FUNC_NAME, errmsg);
            continue;
        }

        /* Determine the maximum bounds, starting with the bounds from the
           first file */
        if (i == 0)
        {
            east_coord = temp_east_coord;
            west_coord = temp_west_coord;
            north_coord = temp_north_coord;
            south_coord = temp_south_coord;
        }
        else
        {
            if (temp_west_coord < west_coord)
                west_coord = temp_west_coord;
            if (temp_east_coord > east_coord)
                east_coord = temp_east_coord;
            if (temp_north_coord > north_coord)
                north_coord = temp_north_coord;
            if (temp_south_coord < south_coord)
                south_coord = temp_south_coord;
        }

        if (verbose)
        {
            printf ("  East: %lf\n", temp_east_coord);
            printf ("  West: %lf\n", temp_west_coord);
            printf ("  North: %lf\n", temp_north_coord);
            printf ("  South: %lf\n", temp_south_coord);
        }
    }

    /* Open the output bounding extents file */
    extent_fptr = fopen (extent_outfile, "w");
    if (extent_fptr == NULL)
    {
        sprintf (errmsg, "Unable to open the output bounding extents file: %s",
            extent_outfile);
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    if (verbose)
    {
        printf ("\nMaximum extents of list --\n");
        printf ("  East: %lf\n", east_coord);
        printf ("  West: %lf\n", west_coord);
        printf ("  North: %lf\n", north_coord);
        printf ("  South: %lf\n", south_coord);
    }

    /* Write the bounding extents */
    fprintf (extent_fptr, "West, North, East, South\n");
    fprintf (extent_fptr, "%f, %f, %f, %f", west_coord, north_coord,
        east_coord, south_coord);

    /* Close the output file */
    fclose (extent_fptr);

    /* Free the filename pointers */
    for (i = 0; i < nlines; i++)
        free (xml_infile[i]);
    free (xml_infile);
    free (list_infile);
    free (extent_outfile);

    /* Indicate successful completion of processing */
    printf ("Maximum extent complete!\n");
    exit (SUCCESS);
}


/******************************************************************************
MODULE:  usage

PURPOSE:  Prints the usage information for this application.

RETURN VALUE:
Type = None

HISTORY:
Date          Programmer       Reason
--------      ---------------  -------------------------------------
04/24/2013    Gail Schmidt     Original Development

NOTES:
******************************************************************************/
void usage ()
{
    printf ("determine_max_extent determines the maximum extent bounds in "
            "projection coordinates for the temporal stack of data.\n\n");
    printf ("usage: determine_max_extent "
            "--list_file=input_list_file "
            "--extent_file=output_extent_filename "
            "[--verbose]\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -list_file: name of the input text file containing the list "
            "of XML files to be processed, one file per line\n");
    printf ("    -extent_file: name of the output file containing the "
            "maximum spatial extents in projection coords\n");
    printf ("\nwhere the following parameters are optional:\n");
    printf ("    -verbose: should intermediate messages be printed? (default "
            "is false)\n");
    printf ("\ndetermine_max_extent --help will print the usage statement\n");
    printf ("\nExample: determine_max_extent "
            "--list_file=input_stack.txt "
            "--extent_file=bounding_box_coordinates.txt "
            "--verbose\n");
}


/******************************************************************************
MODULE:  get_args

PURPOSE:  Gets the command-line arguments and validates that the required
arguments were specified.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           Error getting the command-line arguments or a command-line
                argument and associated value were not specified
SUCCESS         No errors encountered

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
--------      ---------------  -------------------------------------
04/24/2013    Gail Schmidt     Original Development

NOTES:
  1. Memory is allocated for the input and output files.  All of these should
     be character pointers set to NULL on input.  The caller is responsible
     for freeing the allocated memory upon successful return.
******************************************************************************/
short get_args
(
    int argc,             /* I: number of cmd-line args */
    char *argv[],         /* I: string of cmd-line args */
    char **list_infile,   /* O: address of input list filename */
    char **extent_outfile, /* O: address of output extents filename */
    bool *verbose         /* O: verbose flag */
)
{
    int c;                           /* current argument index */
    int option_index;                /* index for the command-line option */
    static int verbose_flag=0;       /* verbose flag */
    char errmsg[STR_SIZE];           /* error message */
    char FUNC_NAME[] = "get_args";   /* function name */
    static struct option long_options[] =
    {
        {"verbose", no_argument, &verbose_flag, 1},
        {"list_file", required_argument, 0, 'i'},
        {"extent_file", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* Loop through all the cmd-line options */
    opterr = 0;   /* turn off getopt_long error msgs as we'll print our own */
    while (1)
    {
        /* optstring in call to getopt_long is empty since we will only
           support the long options */
        c = getopt_long (argc, argv, "", long_options, &option_index);
        if (c == -1)
        {   /* Out of cmd-line options */
            break;
        }

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
     
            case 'h':  /* help */
                usage ();
                return (ERROR);
                break;

            case 'i':  /* list infile */
                *list_infile = strdup (optarg);
                break;
     
            case 'o':  /* extent outfile */
                *extent_outfile = strdup (optarg);
                break;
     
            case '?':
            default:
                sprintf (errmsg, "Unknown option %s", argv[optind-1]);
                error_handler (true, FUNC_NAME, errmsg);
                usage ();
                return (ERROR);
                break;
        }
    }

    /* Make sure the infiles and outfiles were specified */
    if (*list_infile == NULL)
    {
        sprintf (errmsg, "Reflectance list input file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    if (*extent_outfile == NULL)
    {
        sprintf (errmsg, "Extents output file is a required argument");
        error_handler (true, FUNC_NAME, errmsg);
        usage ();
        return (ERROR);
    }

    /* Check the verbose flag */
    if (verbose_flag)
        *verbose = true;
    else
        *verbose = false;

    return (SUCCESS);
}
