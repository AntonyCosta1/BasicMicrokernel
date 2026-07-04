#include <stdint.h>
#include "fs.h"
#include "block.h"

static uint8_t virtual_disk[MAX_BLOCKS][BLOCK_SIZE];  // SIMULA UM ESPAÇO DE DISCO (VIRTUAL)
static uint8_t block_bitmap[MAX_BLOCKS]; // BITMAP PARA GERENCIAR BLOCO DE DADOS

void block_init(void)
{
    for (uint32_t i = 0; i < MAX_BLOCKS; i++)
    {
        block_bitmap[i] = 0; // BLOCO LIVRE

        for (uint32_t j = 0; j < BLOCK_SIZE; j++)
        {
            virtual_disk[i][j] = 0; // ZERA O BLOCO
        }
    }
}

int block_alloc(void)
{
    for (uint32_t i = 0; i < MAX_BLOCKS; i++){
        if (block_bitmap[i] == 0) // BLOCO LIVRE
        {
            block_bitmap[i] = 1; // MARCA COMO OCUPADO
            for (uint32_t j = 0; j < BLOCK_SIZE; j++)
            {
                virtual_disk[i][j] = 0; // ZERA O BLOCO
            }
            return i; // RETORNA O INDICE DO BLOCO ALOCADO
        }
    }
    return -1; // NENHUM BLOCO DISPONÍVEL
}

void block_free(uint32_t block)
{
    if (block >= MAX_BLOCKS){
        return; // BLOCO INVÁLIDO
    }

    block_bitmap[block] = 0; // MARCA COMO LIVRE
    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        virtual_disk[block][i] = 0; // ZERA O BLOCO
    }
}

int block_read(uint32_t block, void *buf)
{
    if (block >= MAX_BLOCKS){
        return -1; // BLOCO INVÁLIDO
    }
    uint8_t *dst = (uint8_t *)buf; // CONVERTE O PONTEIRO PARA BYTE

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        dst[i] = virtual_disk[block][i]; // COPIA O BLOCO PARA O BUFFER
    }
    return 0; // SUCESSO
}

int block_write(uint32_t block, const void *buf)
{
    if (block >= MAX_BLOCKS){
        return -1; // BLOCO INVÁLIDO
    }

    const uint8_t *src = (const uint8_t *)buf; // CONVERTE O PONTEIRO PARA BYTE

    for (uint32_t i = 0; i < BLOCK_SIZE; i++)
    {
        virtual_disk[block][i] = src[i]; // COPIA O BUFFER PARA O BLOCO
    }
    return 0; // SUCESSO
}