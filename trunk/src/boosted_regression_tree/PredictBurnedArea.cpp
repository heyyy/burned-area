/*
 * Predict.cpp
 *
 *  Created on: Nov 15, 2012
 *      Author: jlriegle
 */

#include <stdlib.h>
#include <stdio.h>

#include "PredictBurnedArea.h"
#include "output.h"
#include "input.h"



PredictBurnedArea::PredictBurnedArea() {


        trueCnt = 0;

        /* Copy the SDS names and QA SDS names from the input structure for the
           output structure, since we are simply duplicating the input */
    //    for (ib = 0; ib < input->nband; ib++)
    //        strcpy (&sds_names[ib][0], input->sds[ib].name);
    //    for (ib = 0; ib < input->nqa_band; ib++)
    //        strcpy (&qa_sds_names[ib][0], input->qa_sds[ib].name);

        /* Create and open output file */
    //    if (!CreateOutput(output_file_name))
    //    {
     //       sprintf (errstr, "creating output file - %s", output_file_name);
    //        ERROR (errstr, "main");
    //    }
    //    output = OpenOutput (output_file_name, input->nband, input->nqa_band,
    //        sds_names, qa_sds_names, &input->size);
//        if (output == NULL)
//        {
    //        sprintf (errstr, "opening output file - %s", output_file_name);
    //        ERROR(errstr, "main");
    //    }

        /* JLR - 11/8/12 - Read data into array */
      //  if (!ReadIntoArray(input)) {
      //      sprintf (errstr, "Reading input image data ");
      //                      ERROR (errstr, "main");
      //  }

        /* Loop through each line in the image - reading each line and then
           writing the line back out */
    //    for (il = 0; il < input->size.l; il++)
      //  {
            /* Print status on every 100 lines */
        //    if (!(il%100))
        //    {
        //       printf ("Processing line %d\r",il);
         //      fflush (stdout);
        //    }

            /* For each of the image bands */
        //    for (ib = 0; ib < input->nband; ib++)
          //  {
                /* Read each input reflective band -- data is read into
                   input->buf[ib] */
          //      if (!GetInputLine(input, ib, il))
         //       {
            //        sprintf (errstr, "Reading input image data for line %d, "
              //          "band %d", il, ib);
           //         ERROR (errstr, "main");
            //    }

                /* Copy the input band to the output band -- just because that's
                   what we are going to write back out */

              //  memcpy (&output->buf[ib][0], &input->buf[ib][0],
              //      input->size.s * sizeof (int16));
//
                /* Write each image band */
            //    if (!PutOutputLine(output, ib, il))
            //    {
            //        sprintf (errstr, "Writing output image data for line %d, "
             //           "band %d", il, ib);
             //       ERROR (errstr, "main");
             //   }

              /*  if(!ReadIntoArray(output, ib, il, predictArray))
                {
                    sprintf (errstr, "Writing array data for line %d, "
                                        "band %d", il, ib);
                                    ERROR (errstr, "main");
                }*/

       //     }

            /* For each of the QA bands */
        //    for (ib = 0; ib < input->nqa_band; ib++)
       //    {
                /* Read each input QA band -- data is read into
                   input->qa_buf[ib] */
        //        if (!GetInputQALine(input, ib, il))
        //        {
         //           sprintf (errstr, "Reading input QA data for line %d, band %d",
         //               il, ib);
         //           ERROR (errstr, "main");
          //      }

                /* Copy the input band to the output band -- just because that's
                   what we are going to write back out */
          //      memcpy (&output->qa_buf[ib][0], &input->qa_buf[ib][0],
          //          input->size.s * sizeof (uint8));

                /* Write each QA band */
          //      if (!PutOutputQALine(output, ib, il))
           //     {
          //         sprintf (errstr, "Writing output QA data for line %d, "
            //            "band %d", il, ib);
                 //   ERROR (errstr, "main");
           //     }
         //   }

            /* For the thermal band */
            /* Read the input thermal band -- data is read into input->therm_buf */
       //     if (!GetInputThermLine(input, il))
       //     {
       //         sprintf (errstr, "Reading input thermal data for line %d", il);
        //        ERROR (errstr, "main");
        //    }

            /* We won't write the thermal band to output since it doesn't "fit" */
       // }  /* end for line */
     //   printf ("\n");
//


        /* Close the input file and free the structure */
      //  CloseInput (input);
       // FreeInput (input);

        /* Close the output file and free the structure */
     //   CloseOutput (output);
     //   FreeOutput (output);
      //  free(predictArray);
      //  printf ("Processing complete.\n");
        //return (SUCCESS);
}

PredictBurnedArea::~PredictBurnedArea() {
    // TODO Auto-generated destructor stub
}

