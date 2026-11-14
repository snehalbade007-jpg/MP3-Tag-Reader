
#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Endian conversion */
unsigned int BtoL_endian(unsigned char *size_of_tag);
void LtoB_endian(unsigned int size, unsigned char *data);

/* Validate MP3 file */
int validate_view(char *file);
int validate_edit(char *file);

/* View MP3 metadata */
int view_data(char *file);

/* Edit MP3 metadata */
int edit_data(char *option, char *new_data, char *file);

#endif

