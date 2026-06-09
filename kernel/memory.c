#include "memory.h"

/*   Configuração do heap   */

#define HEAP_START 0x80400000UL
#define HEAP_SIZE  (8 * 1024 * 1024)   // 8 MB

typedef struct block{ //cabeçalho guarda o tamanho do bloco, se ela ta ocupado ou não e o ponteiro pro proximo bloco da lista encadeada -- saudades carrard
    uint64_t size;
    int free;
    struct block *next;
} block_t;

static uint8_t *heap_base = (uint8_t*)HEAP_START;
static block_t *free_list;

uint64_t memory_total(void) {
    return HEAP_SIZE;
}

static uint64_t align8(uint64_t size) {
    return (size + 7) & ~7ULL; // alinha as alocações em multiplo de 8 btes, evita desalinhamento e segue o riscv 64 bits
}

/*   Inicialização   */

void memory_init(void)
{
    free_list = (block_t*)heap_base; // primeiro bloco começa no começo (redundante) no heap
    free_list->size = HEAP_SIZE - sizeof(block_t); // tratar o heap todo como um bloco livre
    free_list->free = 1;
    free_list->next = 0;
}

/*   Alocador bump   */ //AGORA VAI SER FIRST FIT - aloca a memoria de forma dinamica

void *kmalloc(uint64_t size)
{
    if (size == 0) {
        return 0;
    }

    size = align8(size);
    block_t *current = free_list;

    while (current)
    {
        if (current->free && current->size >= size){ // ENCONTRA O BLOCO VALIDO
            
            if (current->size >= size + sizeof(block_t)+8){ //DIVIDIR O BLOCO SE SOBRAR ESPAÇO
                block_t *new_block = (block_t*)((uint8_t*)current + sizeof(block_t)+size); // O PONTEIRO QUE RETORNA APONTA PRA AREA, PRA VOLTAR PRO CABEÇALHO É SÓ SUBTRAIR O TAMANHO DO HEADER.
                
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

        if (current->free && current->next->free && current_end == (uint8_t*)current->next){ // VE SE O BLOCO TA LIVRE, SE O PROX TA LIVRE E ELES SÃO ADJACENTES
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
    uint64_t free_mem = 0;
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