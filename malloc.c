#include<stdio.h>
#include <stddef.h>
#include <stdlib.h>

#define SIZE 25000
#define SMALL_BLOCK_SIZE 762
#define LARGE_BLOCK_SIZE 3830
#define SMALL_BLOCK_COUNT 22
#define LARGE_BLOCK_COUNT 2
#define FREE_LIST(i) (Block *)(Heap + sizeof(Block)*i)
#define ALLOCATED_LIST(i) (Allocated *)(Heap + sizeof(Block)*(SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT) + sizeof(Block *)*i)
#define ASSIGN_BLOCK(BLOCK_COUNT)   \
                    {   \
                        CHECK_MEM_SIZE\
                        \
                        Block *adr = NULL;\
                        Allocated *all = NULL;\
                        for(int i = 0; i < BLOCK_COUNT; i++) { \
                            if(i >= SMALL_BLOCK_COUNT){\
                                adr = (Block *)FREE_LIST(i);\
                                Block *head = (Block *)(void *)(Heap_pool + (i)*SMALL_BLOCK_SIZE + (i-SMALL_BLOCK_COUNT)*LARGE_BLOCK_SIZE);\
                                head->size = LARGE_BLOCK_SIZE-sizeof(Block);\
                                head->next = NULL;\
                                adr->next = head;\
                                adr->size = head->size;\
                            }else{\
                                adr = FREE_LIST(i);\
                                Block *head = (Block *)(void *)(Heap_pool + (i)*SMALL_BLOCK_SIZE);\
                                head->size = SMALL_BLOCK_SIZE-sizeof(Block);\
                                head->next = NULL;\
                                adr->next = head;\
                                adr->size = head->size;\
                            }\
                            all = ALLOCATED_LIST(i);\
                            all->next = NULL;\
                        }\
                    }

#define CHECK_MEM_SIZE \
                    {\
                        if((SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT)*(sizeof(Block) + sizeof(Allocated *)) + SMALL_BLOCK_COUNT*SMALL_BLOCK_SIZE + LARGE_BLOCK_SIZE*LARGE_BLOCK_COUNT > SIZE){\
                            printf("\nTotal size of small block and large block is exceed the allocated array size!!!\n");\
                            exit(0);\
                        }\
                    }


struct Block{
    size_t size;
    struct Block *next;
};

struct Allocated{
    struct Block *next;
};

typedef struct Block Block;
typedef struct Allocated Allocated;

char Heap[SIZE];
char *Heap_pool = Heap + sizeof(Block)*(SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT) + sizeof(Block *)*(SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT);
int assign_block_check = 1;

void *MyMalloc(size_t size){

    int i = 0;
    Block *temp = NULL;
    Block *pretmp = NULL;
    Block *start = NULL;
    void *location = NULL;

    if(assign_block_check){
        ASSIGN_BLOCK(SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT);
        assign_block_check = 0;
    }

    if(size > SMALL_BLOCK_SIZE-sizeof(Block)) i = SMALL_BLOCK_COUNT;

    do{
        if(temp != NULL && temp->size >= size){
            start = temp;
            break;
        }else if(i == SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT) temp = NULL;
        else temp = FREE_LIST(i++);
    }while(temp != NULL);

    if(temp != NULL){

        int temp_size = temp->size;
        Block *temp_block = temp;
        do{
            if(temp->next != NULL){
                if(temp->next->size >= size && temp->next->size <= temp_size){
                    temp_size = temp->next->size;
                    temp_block = temp->next;
                    pretmp = temp;
                }
            }
            temp = temp->next;
        }while(temp != NULL);
        
        pretmp->next = temp_block->next;
        temp_block->next = NULL;
        temp = temp_block;
        if(size == temp->size) location = (void *)((char *)temp + sizeof(Block));
        else{
            if((temp->size - size) > sizeof(Block)){
                Block *c = (Block *)(void *)((char *)temp + sizeof(Block) + size);
                c->size = temp->size - size - sizeof(Block);
                c->next = pretmp->next;
                pretmp->next = c;
                location = (void *)((char *)temp + sizeof(Block));
            }else{
                size = temp->size;
                location = (void *)((char *)temp + sizeof(Block));
            }
        }

        start->size = 0;
        temp = start->next;
        while(temp != NULL){
            if(start->size < temp->size) start->size = temp->size;
            temp = temp->next;
        }

        Block *f = (Block *)(location - sizeof(Block));

        Allocated *all = ALLOCATED_LIST(--i);
        if(all->next == NULL) all->next = f;
        else{
            temp = all->next;
            while(temp != NULL){
                pretmp = temp;
                temp = temp->next;
            }
            pretmp->next = f;
        }

        f->size = size;
        f->next = NULL;
        
        printf("\nAllocated\n");
        return location;
    }
    printf("\nNot enough memory\n");
    return NULL;
}

void MyFree(void *addr){

    Block *temp = NULL;
    Block *start = NULL;
    Block *pretmp = NULL;
    Block *pre_all = NULL;
    Allocated *all = NULL;
    Block *b_addr = (Block *)((char *)addr - sizeof(Block));

    for(int i=0;i<(SMALL_BLOCK_COUNT + LARGE_BLOCK_COUNT);i++){
        if(i < SMALL_BLOCK_COUNT && (char*)addr >= Heap_pool + (SMALL_BLOCK_SIZE*(i)) && (char*)addr < Heap_pool + (SMALL_BLOCK_SIZE*(i+1))){
            temp = FREE_LIST(i);
            all = ALLOCATED_LIST(i);
            break;
        }else if((char*)addr >= Heap_pool + (SMALL_BLOCK_SIZE*SMALL_BLOCK_COUNT + LARGE_BLOCK_SIZE*(i - SMALL_BLOCK_COUNT)) && (char*)addr < Heap_pool + SMALL_BLOCK_SIZE*SMALL_BLOCK_COUNT + (LARGE_BLOCK_SIZE*(i + 1 - SMALL_BLOCK_COUNT))){
            temp = FREE_LIST(i);
            all = ALLOCATED_LIST(i);
            break;
        }
    }

    start = temp;
    if(temp != NULL){
        pretmp = (Block *)all->next;
        temp = NULL;
        while(pretmp != NULL){
            if((void *)b_addr == (void *)pretmp){
                temp = start;
                break;
            }
            pre_all = pretmp;
            pretmp = pretmp->next;
        }

        if(temp != NULL){
            if(pre_all == NULL) all->next = NULL;
            else pre_all->next = b_addr->next;
        }
        
        while(temp != NULL){
            if((char *)temp + sizeof(Block) + temp->size < (char *)b_addr){
                if((char *)temp->next > (char *)b_addr + sizeof(Block) + b_addr->size || temp->next == NULL){
                    b_addr->next = temp->next;
                    temp->next = b_addr;
                }else if((char *)temp->next == (char *)b_addr + sizeof(Block) + b_addr->size){
                    b_addr->size = b_addr->size + sizeof(Block) + temp->next->size;
                    b_addr->next = temp->next->next;
                    temp->next = b_addr;
                }
            }else if((char *)b_addr + sizeof(Block) + b_addr->size == (char *)temp){
                b_addr->next = temp->next;
                pretmp->next = b_addr;
                b_addr->size = b_addr->size + sizeof(Block) + temp->size;
            }
            pretmp = temp;
            temp = temp->next;
        }

        start->size = 0;
        temp = start->next;

        while(temp != NULL){
            if(start->size < temp->size) start->size = temp->size;
            temp = temp->next;
        }
        printf("\nMemory deallocated succesfully!!!\n");
        return;
    }
    printf("\nWrong address!!! Cannot deallocate memory!!!\n");
}