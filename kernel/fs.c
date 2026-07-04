#include <stdint.h>
#include "fs.h"
#include "block.h"
#include "inode.h"
#include "uart.h"

static superblock_t superblock;
static uint32_t root_inode_id = 0; // ID DO INODE RAIZ

static int strcmp_local (const char *a, const char *b) // FUNÇÃO PARA COMPARAR DUAS STRINGS
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b; // RETORNA A DIFERENÇA ENTRE OS CARACTERES, SE FOR 0, AS STRINGS SÃO IGUAIS E SE FOR DIFERENTE, RETORNA A DIFERENÇA ENTRE OS CARACTERES
}

static uint32_t strlen_local(const char *s) // FUNÇÃO PARA CALCULAR O TAMANHO DE UMA STRING
{
    uint32_t len = 0;
    while (s[len] != '\0') { // ENQUANTO NÃO CHEGAR NO FINAL DA STRING, INCREMENTA O TAMANHO
        len++;
    }
    return len;
}

static void strcpy_local(char *dst, const char *src) // FUNÇÃO PARA COPIAR UMA STRING PARA OUTRA
{
    while (*src) { // ENQUANTO NÃO CHEGAR NO FINAL DA STRING, COPIA O CARACTERE PARA A STRING DE DESTINO
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0'; // ADICIONA O CARACTERE NULO NO FINAL DA STRING
}

static void clear_buffer(uint8_t *buf, uint32_t size) // FUNÇÃO PARA LIMPAR UM BUFFER
{
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0; // ZERA O BUFFER
    }
}

static uint32_t inode_number(inode_t *node) // FUNÇÃO PARA OBTER O NÚMERO DO INODE A PARTIR DO PONTEIRO PARA O INODE
{
    inode_t *base = inode_get(0); // OBTÉM O PONTEIRO PARA O PRIMEIRO INODE DA TABELA DE INODES
    return (uint32_t)(node - base); // RETORNA O NÚMERO DO INODE, SUBTRAINDO O PONTEIRO PARA O INODE DA TABELA DE INODES
}