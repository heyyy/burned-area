#include <getopt.h>
#include "generate_stack.h"

/******************************************************************************
MODULE:  generate_stack

PURPOSE:  Generates the CSV stack file for the input list of XML files.

RETURN VALUE:
Type = int
Value           Description
-----           -----------
ERROR           An error occurred during processing of the files
SUCCESS         Processing was successful

PROJECT:  Land Satellites Data System Science Research and Development (LSRD)
at the USGS EROS

HISTORY:
Date          Programmer       Reason
----------    ---------------  -------------------------------------
03/12/2014    Gail Schmidt     Original Development

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
    char *stack_file=NULL;     /* output CSV file for the XML stack */

    int i;                     /* looping variable */
    int retval;                /* return status */
    int curr_line;             /* counter of the current line */
    int nlines;                /* number of lines in the input temporal list */
    int nfiles;                /* number of actual files in the input list */
    int count;                 /* count of items read from file */

    Ba_scene_meta_t scene_meta; /* structure to contain the desired metadata
                                   for the current XML file */
    FILE *list_fptr=NULL;      /* output file pointer for list of files */
    FILE *stack_fptr=NULL;     /* output file pointer for CSV stack */

    printf ("Generating CSV stack file ...\n");

    /* Read the command-line arguments, including the name of the input
       list of files and the output file to write the stack */
    retval = get_args (argc, argv, &list_infile, &stack_file, &verbose);
    if (retval != SUCCESS)
    {   /* get_args already printed the error message */
        exit (ERROR);
    }

    /* Provide user information if verbose is turned on */
    if (verbose)
    {
        printf ("  Input list file: %s\n", list_infile);
        printf ("  Output stack file: %s\n", stack_file);
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

    /* Open the output CSV stack file */
    stack_fptr = fopen (stack_file, "w");
    if (stack_fptr == NULL)
    {
        sprintf (errmsg, "Unable to open the output CSV stack file: %s",
            stack_file);
        error_handler (true, FUNC_NAME, errmsg);
        exit (ERROR);
    }

    /* Write the header for the stack file */
    fprintf (stack_fptr, "file, year, season, month, day, julian, path, row, "
        "satellite, west, east, north, south, nrow, ncol, dx, dy, utm_zone\n");

    /* Loop through each of the input files, read the metadata, and write to
       the stack file */
    if (verbose)
        printf ("Input list file contains %d filenames\n", nfiles);
    for (i = 0; i < nfiles; i++)
    {
        if (verbose)
            printf ("\nProcessing current file %d: %s\n", i, xml_infile[i]);

        /* Process the current file */
        retval = read_xml (xml_infile[i], &scene_meta);
        if (retval != SUCCESS)
        {  /* trouble processing this file so skip and go to the next one */
            sprintf (errmsg, "Error processing file %s.  Skipping and moving "
                "to the next file.", xml_infile[i]);
            error_handler (false, FUNC_NAME, errmsg);
            continue;
        }

        /* Write the stack information */
        fprintf (stack_fptr, "%s, %d, %s, %d, %d, %d, %d, %d, %s, "
            "%lf, %lf, %lf, %lf, %d, %d, %f, %f, %d\n",
            scene_meta.filename, scene_meta.acq_date.year, scene_meta.season,
            scene_meta.acq_date.month, scene_meta.acq_date.day,
            scene_meta.acq_date.doy, scene_meta.wrs_path, scene_meta.wrs_row,
            scene_meta.satellite, scene_meta.bounding_coords[ESPA_WEST],
            scene_meta.bounding_coords[ESPA_EAST],
            scene_meta.bounding_coords[ESPA_NORTH],
            scene_meta.bounding_coords[ESPA_SOUTH],
            scene_meta.nlines, scene_meta.nsamps,
            scene_meta.pixel_size[0], scene_meta.pixel_size[1],
            scene_meta.utm_zone);
    }

    /* Close the output files */
    fclose (stack_fptr);

    /* Free the filename pointers */
    for (i = 0; i < nlines; i++)
        free (xml_infile[i]);
    free (xml_infile);

    free (list_infile);
    free (stack_file);

    /* Indicate successful completion of processing */
    printf ("Stack file generation complete!\n");
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
    printf ("generate_stack generates the CSV file which contains the "
            "stack of input files along with their associated metadata "
            "needed for processing burned area products.\n\n");
    printf ("usage: generate_stack "
            "--list_file=input_list_file "
            "--stack_file=output_stack_csv_filename "
            "[--verbose]\n");

    printf ("\nwhere the following parameters are required:\n");
    printf ("    -list_file: name of the input text file containing the list "
            "of XML files to be processed, one file per line\n");
    printf ("    -stack_file: name of the output CSV file containing the "
            "list of files and associated metadata\n");
    printf ("\nwhere the following parameters are optional:\n");
    printf ("    -verbose: should intermediate messages be printed? (default "
            "is false)\n");
    printf ("\ngenerate_stack --help will print the usage statement\n");
    printf ("\nExample: generate_stack "
            "--list_file=input_stack.txt --stack_file=input_stack.csv "
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
----------    ---------------  -------------------------------------
03/12/2014    Gail Schmidt     Original Development

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
    char **stack_outfile, /* O: address of output stack filename */
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
        {"list_file", required_argument, 0, 'l'},
        {"stack_file", required_argument, 0, 's'},
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

            case 'l':  /* list infile */
                *list_infile = strdup (optarg);
                break;
     
            case 's':  /* stack outfile */
                *stack_outfile = strdup (optarg);
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

    if (*stack_outfile == NULL)
    {
        sprintf (errmsg, "Stack CSV output file is a required argument");
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
