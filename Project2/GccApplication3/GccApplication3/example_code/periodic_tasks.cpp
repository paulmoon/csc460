
#include "kernel.h"
#include "periodic_tasks.h"

static periodic_block_t periodic_blocks[MAXPROCESS];
static queue_t active_periodic;
static queue_t dead_pool_periodic;

void periodic_init(){    
    int i = 0;

    // link together all the periodic_blocks
    for( i =0;i < MAXPROCESS -1 ; ++i)
    {
        periodic_blocks[i].next = &periodic_blocks[i + 1];
    }
    periodic_blocks[MAXPROCESS - 1].next = NULL;

    // active_periodic queue should begin empty.
    active_periodic.periodic_head = NULL;
    active_periodic.periodic_tail = NULL;
    active_periodic.size = 0;

    // setup the dead-pool for the periodic_blocks
    dead_pool_periodic.periodic_head = &periodic_blocks[0];
    dead_pool_periodic.periodic_tail = &periodic_blocks[MAXPROCESS - 1];
    dead_pool_periodic.size = MAXPROCESS;
}

void remove_active_task(task_descriptor_t* cur_task)
{
    periodic_block_t* prev = NULL;
    periodic_block_t* current = active_periodic.periodic_head;
    while( current != NULL)
    {
        if( current->task == cur_task)
        {            
            if(active_periodic.periodic_head == current)
            {
                active_periodic.periodic_head = current->next;
            }
            if( active_periodic.periodic_tail == current)
            {
                active_periodic.periodic_tail = prev;                
            }

            if( current->next != NULL)
            {
                current->next->time_remaining += current->next->time_remaining;
            }

            current->task = NULL;
            current->time_remaining = 0;
            current->next = NULL;
            periodic_push(&dead_pool_periodic,current);

            active_periodic.size -= 1;
            return; 
        }

        prev = current;
        current = current->next;
    }    
}

void periodic_update_ticker(task_descriptor_t* cur_task){
    // update wcet time_remaining
    if( cur_task != NULL &&
        cur_task->level == PERIODIC && 
        cur_task->state == RUNNING)
    {
        cur_task->wcet_counter -= 1;
        if (cur_task->wcet_counter <= 0)
        {
            //error_msg = ERR_RUN_7_PERIODIC_TOOK_TOO_LONG;
            //OS_Abort();
        }
    }
    
    // update the active periodic tasks
    if( active_periodic.size <= 0){
        return;
    }
    

    periodic_block_t* b;
    active_periodic.periodic_head->time_remaining -= 1;    
    while( active_periodic.periodic_head->time_remaining <= 0)
    {
        b = periodic_pop(&active_periodic); 

        // enqueue the task onto the periodic_queue
        b->task->state = READY;        
        //enqueue(&periodic_queue,b->task);

        // re-enqueue the periodic block
        b->time_remaining = b->task->period;
        b->task->wcet_counter = b->task->wcet_counter;
        periodic_push(&active_periodic,b);
    }

    // check for errors
    /*
	if( periodic_queue.size >= 2)
    {
        //OS_Abort();
    }

    if( cur_task != NULL &&
        cur_task->level == PERIODIC &&
        periodic_queue.size > 0)
    {
        //OS_Abort();
    }
	*/
}

void periodic_push(queue_t* q,periodic_block_t* b)
{
    if( q->periodic_head == NULL)
    {
        q->periodic_head = b;
        q->periodic_tail = b;
        q->size = 1;
    }
    else
    {        
        b->next = NULL;

        // user must set the time_remaining before they pass into the push function
        //b->time_remaining = b->task->period;

        // find the previous periodic_block whose time_remaining value
        // is greater than our current time left.
        periodic_block_t* prev = NULL;
        periodic_block_t* current = NULL;
        while( b->time_remaining >= current->time_remaining)
        {
            b->time_remaining -= current->time_remaining;

            prev = current;
            current  = current->next;
            if( current == NULL){
                break;
            }
        }

        if( prev == NULL)
        {
            // insert in the front of the queue
            b->next = q->periodic_head;
            q->periodic_head = b;

        }
        else if( prev == q->periodic_tail)
        {
            // inserting at the end of the queue
            q->periodic_tail->next = b;
            b->next = NULL;
        }
        else
        {
            // insert in the middle
            b->next = prev->next;
            prev->next = b;
        }        


        // decrement the time_remaining on the next immediate task
        if( b->next != NULL)
        {
            b->next->time_remaining -= b->time_remaining;
            if( b->next->time_remaining < 0)
            {
                //OS_Abort();
            }
        }
        
        q->size += 1;        
    }
}

periodic_block_t* periodic_pop(queue_t* q)
{
    periodic_block_t* rs = q->periodic_head;
    if( q->periodic_head != NULL)
    {           
        q->periodic_head = rs->next;
        rs->next = NULL;
        q->size -= 1;

        if( q->periodic_head == NULL){
            q->periodic_tail = NULL;
        }
    }
    return rs;
}