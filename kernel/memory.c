#include "memory.h"

/*   Configuração do heap   */

#define HEAP_START 0x80400000UL
#define HEAP_SIZE  (8 * 1024 * 1024)   // 8 MB

typedef struct {
    uint64_t size;
    int free;
    struct block *next;
} block_t;

static uint8_t *heap_base = (uint8_t*)HEAP_START;
static block_t *free_list;

static uint64_t align8(uint64_t size) {
    return (size + 7) & ~7ULL;
}

/*   Inicialização   */

void memory_init(void)
{
    free_list = (block_t*)heap_base;
    free_list->size = HEAP_SIZE - sizeof(block_t);
    free_list->free = 1;
    free_list->next = 0;
}

/*   Alocador bump   */ //AGORA VAI SER FIRST FIT

void *kmalloc(uint64_t size)
{
    if (size == 0) {
        return 0;
    }

    size = align8(size);
    block_t *current = free_list;

    while (current)
    {
        if (current->free && current->size >= size){
            //DIVIDIR O BLOCO SE SOBRAR ESPAÇO
            if (current->size >= size + sizeof(block_t)+8){
                block_t &new_block = (block_t*)((uint8_t*)current + sizeof(block_t)+size);
                
                new_block->size = current->size - size - sizeof(block_t);
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }
            current->free = 0;
            return(uint8_t*)current + sizeof(block_t);
        }
        current = current->next;
    }
    return 0; //SEM MEMO SUFICIENTE
}

/*   Free mínimo   */ //AGORA VAI LIBERAR COM COALESCÊNCIA SIMPLES

void kfree(void *ptr)
{
    if (ptr == 0){
        return;
    }
    block_t *block = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    block->free = 1;

    //COALESCÊNCIA SIMPLES
    block_t *current = free_list;

    while (current && current->next){
        uint8_t *current_end = (uint8_t*)current + sizeof(block_t) + current->size;

        if (current->free && current->next->free && current_end == (uint8_t*)current->next){
            current->size += sizeof(block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next; 
        }
    }
}

/*   Estatísticas   */

uint64_t memory_used(void)
{
    uint64_t used = 0;
    block_t *current = free_list;
    
    while (current)
    {
        if (!current->free) {
            used += current->size;
        }
        current = current->next;
    }
    return used;
}

uint64_t memory_free(void)
{
    uint64_t free = 0;
    block_t *current = free_list;
    
    while (current)
    {
        if (current->free){
            free_mem += current->size;
        }
        current = current->next;
    }
    return free_mem;
}