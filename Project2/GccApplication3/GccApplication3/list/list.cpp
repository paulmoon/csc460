#include "list.h"

void list_push(list_t* q, int index,NODE_T* n)
{
    if( index < 0 || index >= (q->size +1 ))
    {
        return;
    }
    n->next = NULL;

    // empty queue,so just add to the queue.
    if( q->size == 0)
    {
        q->head = n;
        q->tail = n;
        q->size = 1;
        return;
    }

    // we only get here if our queue has more than one element.    
    NODE_T* prev = NULL;
    NODE_T* current = q->head;
    int i;
    for( i = 0 ;i < index; ++i)
    {
        if( current == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    if(prev != NULL)
    {
        if(prev->next == NULL)
        {
            // inserting at the end
            q->tail = n;
            prev->next = n;
        }
        else
        {
            // inserting in the middle            
            n->next = prev->next;
            prev->next = n;
        }
    }
    else
    {
        // inserting at the head
        n->next = q->head;
        q->head = n;
    }

    q->size += 1;
}

NODE_T* list_pop(list_t* q, int index)
{
    if( index < 0 || index >= q->size)
    {
        return NULL;
    }
    
    NODE_T* prev = NULL;
    NODE_T* current = q->head;
    int i;
    for( i = 0;i < index; ++i)
    {
        if(current == NULL)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    if( prev == NULL)
    {                
        // we are removing the first element.        
        if( q->head == q->tail )
        {
            // there is only one element in the list.
            q->tail = NULL;
            q->head = NULL;
        }
        else
        {
            // make head point to the next element
            q->head = q->head->next;
        }
    }
    else
    {
        // remove the desired element
        prev->next = current->next;

        if( current == q->tail)
        {   
            // reassign the tail
            q->tail = prev;
        }
    }
    q->size -= 1;
    return current;
}