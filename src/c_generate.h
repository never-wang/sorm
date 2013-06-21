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
 *	    Email:  never.wencan@gmail.com
 ***************************************************************************/
#ifndef C_GENERATE_H
#define C_GENERATE_H

#include "sorm.h"

#define INDENT "    "
#define INDENT_TWICE INDENT INDENT
#define INDENT_TRIPLE INDENT_TWICE INDENT
#define HEADER_NAME_MAX_LEN 127

void sorm_header_generate(
    const sorm_table_descriptor_t *table_desc);

#endif
