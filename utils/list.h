/****************************************************************************
 *       Filename:  list.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/25/13 14:38:58
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wang Wencan 
 *	    Email:  never.wencan@gmail.com
 ***************************************************************************/
#ifndef LIST_H
#define LIST_H

typedef struct
{
    list_head_t *next, *prev;
    void *data;

} list_head_t;

#define INIT_LIST_HEAD(head) do { \
    (head)->next = (head); (head)->prev = (head); \
} while (0)

#define list_for_each(pos, head) \
    for(pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_safe(pos, scratch, head) \
    for(pos = (head)->next, scratch = pos->next; pos != (head); \
	    pos = scratch, scratch = pos->next)

static inline void _qlist_add(
	list_head_t *new, list_head_t *prev, list_head_t *next)
{
    new->next = prev->next;
    prev->next = new;
    new->prev = next->prev;
    next->prev = new;
}

static inline void qlist_add_tail(list_head_t *new, list_head_t *head)
{
    _qlist_add(new, head->prev, head);
}

#endif
