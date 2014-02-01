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
 *          Email:  never.wencan@gmail.com
 ***************************************************************************/
#ifndef LOG_H
#define LOG_H

#undef ENABLE_ZLOG

#include "config.h"

typedef enum
{
    LOG_OK          =   0,
    LOG_INIT_FAIL   =   1,
}log_error_t;

#ifdef ENABLE_ZLOG

#include "zlog.h"

extern zlog_category_t *zlog_category;

static inline int log_init()
{
    int ret;

    if(zlog_category == NULL)
    {
        ret = zlog_init(ZLOG_CONF);
        if(ret != 0)
        {
            printf("zlog init fail.\n");
            return LOG_INIT_FAIL;
        }
        zlog_category = zlog_get_category("SORM");
        if(zlog_category == NULL)
        {
            zlog_fini();
            printf("zlog_get_category fail.\n");
            return LOG_INIT_FAIL;
        }
    }

    return LOG_OK;
}

static inline void log_final()
{
    if(zlog_category != NULL)
    {
        zlog_fini();
        zlog_category = NULL;
    }
}

#define log_log(f...) \
    do { \
        zlog_fetal(zlog_category, ##f); \
    } while(0)
#define log_debug(f...) \
    do { \
        zlog_debug(zlog_category, ##f); \
    } while(0)
#define log_error(f...) \
    do { \
        zlog_error(zlog_category, ##f); \
    } while(0)
#define log_info(f...) \
    do { \
        zlog_info(zlog_category, ##f); \
    } while(0)

#else

#define log_log(f...)   do {} while(0)
#define log_debug(f...) do {} while(0)
#define log_error(f...) do {} while(0)
#define log_info(f...)  do {} while(0)

#define log_init()      LOG_OK
#define log_final()     do {} while(0)

#endif

#endif
