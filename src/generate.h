/****************************************************************************
 *       Filename:  generate.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/13/2013 12:34:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *          Email:  never.wencan@gmail.com
 ***************************************************************************/
#ifndef GENERATE_H
#define GENERATE_H

#include "sorm.h"

extern sorm_table_descriptor_t *table_desc;
extern sorm_list_t *columns_list_head;
extern sorm_column_descriptor_t *column_desc;

#define INDENT "    "
#define INDENT_TWICE INDENT INDENT
#define INDENT_TRIPLE INDENT_TWICE INDENT
#define HEADER_NAME_MAX_LEN 127

static inline void case_lower2upper(char *lower_case, char *upper_case)
{
    int i = 0;

    while(lower_case[i] != '\0')
    {
        if(lower_case[i] <= 'z' && lower_case[i] >= 'a')
        {
            upper_case[i] = lower_case[i] - 32;
        }else{
            upper_case[i] = lower_case[i];
        }
        i++;
    }
    upper_case[i] = '\0';
}

#endif
