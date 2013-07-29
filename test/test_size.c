/****************************************************************************
 *       Filename:  test_size.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/22/2013 08:44:58 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 *        Company:  HPC Tsinghua
 ***************************************************************************/
#include <assert.h>

#include "sorm.h"

#include "a_sorm.h"
#include "b_sorm.h"

int main(int argc, char **argv)
{
    int ret;

    sorm_connection_t *conn_a, *conn_b;
    ret = sorm_open("A.db", SORM_DB_SQLITE, &conn_a);
    assert(ret == SORM_OK);
    ret = sorm_open("B.db", SORM_DB_SQLITE, &conn_b);
    assert(ret == SORM_OK);

    ret = a_create_table(conn_a);
    assert(ret == SORM_OK);

    ret = b_create_table(conn_b);
    assert(ret == SORM_OK);

    char name[128];
    char desc[128];
    a_t *a;
    b_t *b;
    int i;

    sorm_begin_transaction(conn_a);
    //for(i = 0; i < 10; i ++)
    //{
    //    a = a_new();
    //    sprintf(name, "name_%d", i);
    //    sprintf(desc, "desclonglonglonglonglonglong_%d", i);
    //    a_set_name(a, name);
    //    a_set_desc(a, desc);
    //    a_save(conn_a, a);
    //    a_free(a);
    //}

    for(i = 0; i < 100000; i ++)
    {
	a = a_new();
	sprintf(name, "name_%d", i);
	a_set_name(a, name);
	a_save(conn_a, a);
	a_free(a);
    }
    sorm_commit_transaction(conn_a);
    
    sorm_begin_transaction(conn_b);
    for(i = 0; i < 100000; i ++)
    {
	b = b_new();
	sprintf(name, "name_%d", i);
	b_set_name(b, name);
	b_save(conn_b, b);
	b_free(b);
    }
    sorm_commit_transaction(conn_b);

    a_free(a);
    b_free(b);

    sorm_close(conn_a);
    sorm_close(conn_b);

    return;
}
