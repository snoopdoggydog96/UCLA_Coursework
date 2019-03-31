#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "SortedList.h"
#include <signal.h>
void
malloc_error(void)
{
  fprintf(stderr, "Error with malloc() sys call. Exiting Immediately.\n");
  exit(1); 
}
void
SortedList_insert(SortedList_t *list, SortedListElement_t *element) 
{
  if (opt_yield & INSERT_YIELD)
    {
      sched_yield();
    }
  SortedListElement_t *insertAfter = (SortedListElement_t *) malloc(sizeof(SortedListElement_t)); 
  if (insertAfter == NULL) malloc_error();
  insertAfter->key = element->key;
  SortedListElement_t *iter = list;
  if((iter->next)->key==NULL)
    {
      /*first element to be inserted into list */
      list->next = insertAfter;
      list->prev = insertAfter;
      insertAfter->next = list;
      insertAfter->prev = list;
      return; 
    }
  const char *key1  = (iter->next)->key;
  const char *key2 = insertAfter->key;
  while(((iter->next)->key != NULL) && (strcmp(key1, key2) <=0))
    {
      iter = iter->next; /*traverse list until position of insert */
    }
  SortedListElement_t *new = iter->next;
  iter->next = insertAfter;
  new->prev = insertAfter;
  insertAfter->prev = iter;
  insertAfter->next = new; 
}
int 
SortedList_delete(SortedListElement_t *element)
{
  if (opt_yield & DELETE_YIELD)
    {
      sched_yield();
    }
 
  if(element->key == NULL)
    {
      fprintf(stderr, "Attempting to delete head of list. Exiting Immediately.\n");
      exit(2);
    }
  if ((element->next)->prev != element  || (element->prev)->next != element)
    {
      return 1;
    }
  (element->next)->prev = element->prev;
  (element->prev)->next = element->next;
  free(element);
  return 0;
}

SortedListElement_t *
SortedList_lookup(SortedList_t *list, const char *key)
{
  if (opt_yield & LOOKUP_YIELD) sched_yield();

  SortedList_t *iter = list->next;
  while (iter->key != NULL)
    {
    if (strcmp(iter->key, key) == 0) return iter;
    iter = iter->next; 
    }
  return NULL; 
}

int
SortedList_length(SortedList_t *list)
{
  if (opt_yield & LOOKUP_YIELD) sched_yield();
  int len = 0;
  SortedList_t *iter = list->next;
  while (iter->key != NULL)
    {
      len++;
      iter = iter->next;
    }
  return len; 
}
  

