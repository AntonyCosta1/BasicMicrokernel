#include <stdint.h>
#include "fs.h"
#include "inode.h"

static inode_t inode_table[MAX_INODES]; // TABELA DE INODES
static uint8_t inode_bitmap[MAX_INODES]; // BITMAP PARA GERENCIAR INODES

void inode_init(void)
{
    for (uint32_t i = 0; i < MAX_INODES; i++)
    {
        inode_bitmap[i] = 0; // INODE LIVRE
        inode_table[i].size = 0;
        inode_table[i].type = 0;
        inode_table[i].links = 0;

        for (uint32_t j = 0; j < 8; j++)
        {
            inode_table[i].blocks[j] = 0; // ZERA OS BLOCO DE DADOS
        }
    }
}

inode _t *inode_alloc(void)
{
    for (uint32_t i = 0; i < MAX_INODES; i++){
        if (inode_bitmap[i] == 0) //INODE LIVRE
        {
            inode bitmap[i] = 1; // MARCA COMO OCUPADO
            inode_table[i].size = 0;
            inode_table[i].links = 1; // INICIALIZA O LINK COUNT

            for (int j = 0; j < 8; j++)
            {
                inode_table[i].blocks[j] = 0; // ZERA OS BLOCO DE DADOS
            }
            return &inode_table[i];
        }
    }
    return 0; // NÃO FOI POSSÍVEL ALOCAR UM INODE
}

void inode_free(uint32_t inode)
{
    if (inode >= MAX_INODES){
        return; // INODE INVÁLIDO
    }

    inode_bitmap[inode] = 0; // MARCA COMO LIVRE
    inode_table[inode].size = 0;
    inode_table[inode].links = 0;
    inode_table[inode].type = 0;

    for (int i = 0; i < 8; i++)
    {
        inode_table[inode].blocks[i] = 0; // ZERA OS BLOCO DE DADOS
    }
}

inode_t *inode_get(uint32_t inode)
{
    if (inode >= MAX_INODES){
        return 0; // INODE INVÁLIDO
    }
    
    return &inode_table[inode];
}