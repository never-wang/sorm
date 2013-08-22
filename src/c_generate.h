/****************************************************************************
 *       Filename:  c_generate.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/13/2013 12:06:48 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *          Email:  never.wencan@gmail.com
 ***************************************************************************/
#ifndef C_GENERATE_H
#define C_GENERATE_H

#include "sorm.h"

void sorm_header_generate(
    const sorm_table_descriptor_t *table_desc, char *in_file_name);

#endif
