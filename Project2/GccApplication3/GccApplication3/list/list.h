#ifndef __LIST_H__
#define __LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NODE_T node_t
typedef struct n_t node_t;
struct n_t
{
    int value;
    node_t* next;
};

typedef struct
{
    NODE_T* head;
    NODE_T* tail;
    int size;
}
list_t;

void list_push(list_t* q, int index, NODE_T* n);
NODE_T* list_pop(list_t* q, int index);

/*
Desired Output:
[0]
1 [1]
1 2 [2]
1 [1]
1 2 3 [3]
1 3 2 [3]
3 1 2 [3]
[0]
1 [1]
2 [1]
1 [1]
1 2 [2]
[0]
1 [1]
2 3 [2]

void print_list(list_t* q)
{
    NODE_T* current = q->head;
    while(current !=  NULL)
    {
        printf("%d ",current->value);
        current = current->next;
    }
    printf("[%d]\n",q->size);    
}
int test(){    
    list_t q;
    q.size = 0;
    q.head = NULL;
    q.tail = NULL;

    NODE_T a;
    a.value = 1;
    NODE_T b;
    b.value = 2;
    NODE_T c;
    c.value = 3;


    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,1,&a);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,2,&b);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_push(&q,2,&c);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_push(&q,1,&c);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_push(&q,0,&c);
    print_list(&q);


    // test removes
    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_pop(&q,0);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_pop(&q,1);
    print_list(&q);


    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_pop(&q,0);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_pop(&q,1);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_pop(&q,2);
    print_list(&q);


    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_pop(&q,0);
    list_pop(&q,0);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_pop(&q,1);
    list_pop(&q,1);
    print_list(&q);

    q.head = q.tail = NULL;
    q.size = 0;
    list_push(&q,0,&a);
    list_push(&q,1,&b);
    list_push(&q,2,&c);
    list_pop(&q,2);
    list_push(&q,1,&c);
    list_pop(&q,1);
    list_push(&q,2,&c);
    list_pop(&q,0);
    print_list(&q);


    return 0;
}*/


#ifdef __cplusplus
}
#endif
#endif