#ifndef MAD_DECODER_H
#define MAD_DECODER_H

#include <stdio.h>
#include "error_codes.h"

/******************************************************************************
 * Decode mp3 file using the mad library
 *****************************************************************************/

/* Decode an mp3 file and write the decoded sample data to *output.
 * *output will be newly allocated ane must be freed.
 * The number of samples will be put in *n_samples */
extern error_code mad_decode(FILE *file, signed short **output, size_t *n_samples);

#endif
