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

static int find_dir_entry(inode_t *dir, const char *name)
{
    if (dir == 0){
        return -1; // SE O INODE DO DIRETÓRIO FOR NULO, RETORNA -1
    }

    if (dir->type != FS_DIR){
        return -1; // SE O INODE NÃO FOR UM DIRETÓRIO, RETORNA -1
    }

    uint8_t buffer[BLOCK_SIZE];
    for(int b = 0; b < 8; b++){
        if (dir->blocks[b] == 0){
            continue; // SE O BLOCO DO DIRETÓRIO FOR NULO, CONTINUA PARA O PRÓXIMO BLOCO
        }

        block_read(dir->blocks[b], buffer); // LÊ O BLOCO DO DIRETÓRIO PARA O BUFFER

        dir_entry_t *entryes = (dir_entry_t *)buffer; // CONVERTE O BUFFER PARA UM PONTEIRO PARA ENTRADAS DE DIRETÓRIO
        int total = BLOCK_SIZE / sizeof(dir_entry_t); // CALCULA O NÚMERO TOTAL DE ENTRADAS DE DIRETÓRIO NO BLOCO

        for (int i = 0; i < total; i++){
            if (entryes[i].inode != 0){
                if (strcmp_local(entryes[i].name, name) == 0){
                    return entryes[i].inode; // SE O NOME DA ENTRADA DE DIRETÓRIO FOR IGUAL AO NOME PROCURADO, RETORNA O NÚMERO DO INODE
                }
            }
        }
    }
    return -1; // SE NÃO ENCONTRAR A ENTRADA DE DIRETÓRIO, RETORNA -1
}

static int add_dir_entry(inode_t *dir, const char *name, uint32_t inode_id){
    if (dir == 0){
        return -1; // SE O INODE DO DIRETÓRIO FOR NULO, RETORNA -1
    }

    if (dir->type != FS_DIR){
        return -1; // SE O INODE NÃO FOR UM DIRETÓRIO, RETORNA -1
    }

    uint8_t buffer[BLOCK_SIZE];

    for (int b = 0; b < 8; b++){
        if (dir->blocks[b] == 0){
            // ALOCAR UM NOVO BLOCO PARA O DIRETÓRIO
            int new_block = block_alloc();
            if (new_block < 0){
                return -1; // SE NÃO CONSEGUIR ALOCAR UM NOVO BLOCO, RETORNA -1
            }

            dir->blocks[b] = new_block; // ATRIBUI O NOVO BLOCO AO DIRETÓRIO
            clear_buffer(buffer, BLOCK_SIZE); // LIMPA O BUFFER

        } else {
            block_read(dir->blocks[b], buffer); // LÊ O BLOCO DO DIRETÓRIO PARA O BUFFER
        }

        dir_entry_t *entryes = (dir_entry_t *)buffer; // CONVERTE O BUFFER PARA UM PONTEIRO PARA ENTRADAS DE DIRETÓRIO
        int total = BLOCK_SIZE / sizeof(dir_entry_t); // CALCULA O NÚMERO TOTAL DE ENTRADAS DE DIRETÓRIO NO BLOCO

        for (int i = 0; i < total; i++){
            if (entryes[i].inode == 0){
                entryes[i].inode = inode_id; // ATRIBUI O NÚMERO DO INODE À ENTRADA DE DIRETÓRIO
                strcpy_local(entryes[i].name, name); // COPIA O NOME PARA A ENTRADA DE DIRETÓRIO
                block_write(dir->blocks[b], buffer); // ESCREVE O BUFFER DE VOLTA PARA O BLOCO DO DIRETÓRIO
                dir->size += sizeof(dir_entry_t); // ATUALIZA O TAMANHO DO DIRETÓRIO
                
                return 0; // RETORNA 0 PARA INDICAR SUCESSO
            }
        }
    }
    return -1; // SE NÃO CONSEGUIR ADICIONAR A ENTRADA DE DIRETÓRIO, RETORNA -1 
}

static int split_path(const char *path, char *parent, char *name){
    uint32_t len = strlen_local(path);

    if (len == 0){
        return -1; // SE O CAMINHO FOR VAZIO, RETORNA -1
    }

    if (path[0] != '/'){
        return -1; // SE O CAMINHO NÃO COMEÇAR COM '/', RETORNA -1
    }

    uint32_t last_slash = 0;

    for (uint32_t i = 0; i < len; i++){
        if (path[i] == '/'){
            last_slash = i; // ARMAZENA A POSIÇÃO DO ÚLTIMO '/' ENCONTRADO
        }
    }

    if (last_slash === 0){
        parent[0] = '/'; // SE O ÚLTIMO '/' FOR O PRIMEIRO CARACTERE, O PAI É A RAIZ
        parent[1] = '\0'; // TERMINA A STRING

        uint32_t j = 0;

        for (uint32_t i = 1; i < len; i++){
            name[j] = path[i]; // COPIA O NOME DO ARQUIVO PARA A STRING 'name'
            j++;
        }
        name[j] = '\0'; // TERMINA A STRING
        return 0; // RETORNA 0 PARA INDICAR SUCESSO
    }

    for (uint32_t i = 0; i < last_slash; i++){
        parent[i] = path[i]; // COPIA O CAMINHO DO PAI PARA A STRING 'parent'
    }

    parent[last_slash] = '\0'; // TERMINA A STRING

    uint32_t j = 0;
    for (uint32_t i = last_slash + 1; i < len; i++){
        name[j] = path[i]; // COPIA O NOME DO ARQUIVO PARA A STRING 'name'
        j++;
    }

    name[j] = '\0'; // TERMINA A STRING
    return 0; // RETORNA 0 PARA INDICAR SUCESSO
}

int fs_init(void){
    superblock.magic = TREEFS_MAGIC; // ATRIBUI O VALOR MÁGICO AO SUPERBLOCO
    superblock.total_blocks = MAX_BLOCKS; // ATRIBUI O NÚMERO TOTAL DE BLOCOS AO SUPERBLOCO
    superblock.total_inodes = MAX_INODES; // ATRIBUI O NÚMERO TOTAL DE INODES AO SUPERBLOCO
    superblock.block_size = BLOCK_SIZE; // ATRIBUI O TAMANHO DO BLO

    block_init();
    inode_init();

    inode_t *root = inode_alloc();

    if (root == 0){
        return -1; // SE NÃO CONSEGUIR ALOCAR O INODE RAIZ, RETORNA -1
    }

    root_inode_id = inode_number(root); // ATRIBUI O NÚMERO DO INODE RAIZ À VARIÁVEL 'root_inode_id'
    root->type = FS_DIR;
    root->size = 0;
    root->links = 1;

    int root_block = block_alloc();
    if (root_block < 0){
        return -1; // SE NÃO CONSEGUIR ALOCAR UM BLOCO PARA O INODE RAIZ, RETORNA -1
    }

    root->blocks[0] = root_block;

    mkdir("/home");
    mkdir("/tmp");
    mkdir("/bin");

    uart_print("\nSistema de arquivos inicializando..\n");
    uart_print("Diretorios inicias criados: /home, /tmp, /bin\n");

    return 0; // RETORNA 0 PARA INDICAR SUCESSO
}