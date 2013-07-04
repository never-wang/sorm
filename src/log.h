/****************************************************************************
 *       Filename:  log.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/04/2013 02:18:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  
 ***************************************************************************/
#ifndef LOG_H
#define LOG_H


#include "zlog.h"
#include "config.h"

#ifdef ENABLE_ZLOG

extern zlog_category_t *zlog_category;

static inline void check_zlog_init()
{
    int ret;

    if(zlog_category == NULL)
    {
        ret = zlog_init(ZLOG_CONF);
        if(ret != 0)
        {
            printf("zlog init fail.\n");
            return;
        }
        zlog_category = zlog_get_category("SORM");
        if(zlog_category == NULL)
        {
            zlog_fini();
            printf("zlog_get_category fail.\n");
            return;
        }
    }
}

static inline void log_final()
{
    if(zlog_category != NULL)
    {
        zlog_fini();
        zlog_category = NULL;
    }
    printf("******************fuck\n");
}

#define log_log(f...) \
    do { \
        check_zlog_init(); \
        zlog_fetal(zlog_category, ##f); \
    } while(0)
#define log_debug(f...) \
    do { \
        check_zlog_init(); \
        zlog_debug(zlog_category, ##f); \
    } while(0)
#define log_error(f...) \
    do { \
        check_zlog_init(); \
        zlog_error(zlog_category, ##f); \
    } while(0)
#define log_info(f...) \
    do { \
        check_zlog_init(); \
        zlog_info(zlog_category, ##f); \
    } while(0)

#else

#define log_log(f...)   NULL
#define log_debug(f...) NULL
#define log_error(f...) NULL
#define log_info(f...)  NULL

#endif

#endif
