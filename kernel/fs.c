#include <stdint.h>
#include "fs.h"
#include "block.h"
#include "inode.h"
#include "uart.h"

static superblock_t superblock;
static uint32_t root_inode_id = 0; // ID DO INODE RAIZ

static int strcmp_local(const char *a, const char *b) // FUNÇÃO PARA COMPARAR DUAS STRINGS
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b; // RETORNA A DIFERENÇA ENTRE OS CARACTERES, SE FOR 0, AS STRINGS SÃO IGUAIS E SE FOR DIFERENTE, RETORNA A DIFERENÇA ENTRE OS CARACTERES
}

static uint32_t strlen_local(const char *s) // FUNÇÃO PARA CALCULAR O TAMANHO DE UMA STRING
{
    uint32_t len = 0;
    while (s[len] != '\0')
    { // ENQUANTO NÃO CHEGAR NO FINAL DA STRING, INCREMENTA O TAMANHO
        len++;
    }
    return len;
}

static void strcpy_local(char *dst, const char *src) // FUNÇÃO PARA COPIAR UMA STRING PARA OUTRA
{
    while (*src)
    { // ENQUANTO NÃO CHEGAR NO FINAL DA STRING, COPIA O CARACTERE PARA A STRING DE DESTINO
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0'; // ADICIONA O CARACTERE NULO NO FINAL DA STRING
}

static void clear_buffer(uint8_t *buf, uint32_t size) // FUNÇÃO PARA LIMPAR UM BUFFER
{
    for (uint32_t i = 0; i < size; i++)
    {
        buf[i] = 0; // ZERA O BUFFER
    }
}

static uint32_t inode_number(inode_t *node) // FUNÇÃO PARA OBTER O NÚMERO DO INODE A PARTIR DO PONTEIRO PARA O INODE
{
    inode_t *base = inode_get(0);   // OBTÉM O PONTEIRO PARA O PRIMEIRO INODE DA TABELA DE INODES
    return (uint32_t)(node - base); // RETORNA O NÚMERO DO INODE, SUBTRAINDO O PONTEIRO PARA O INODE DA TABELA DE INODES
}

static int find_dir_entry(inode_t *dir, const char *name)
{
    if (dir == 0)
    {
        return -1; // SE O INODE DO DIRETÓRIO FOR NULO, RETORNA -1
    }

    if (dir->type != FS_DIR)
    {
        return -1; // SE O INODE NÃO FOR UM DIRETÓRIO, RETORNA -1
    }

    uint8_t buffer[BLOCK_SIZE];
    for (int b = 0; b < 8; b++)
    {
        if (dir->blocks[b] == 0)
        {
            continue; // SE O BLOCO DO DIRETÓRIO FOR NULO, CONTINUA PARA O PRÓXIMO BLOCO
        }

        block_read(dir->blocks[b], buffer); // LÊ O BLOCO DO DIRETÓRIO PARA O BUFFER

        dir_entry_t *entryes = (dir_entry_t *)buffer; // CONVERTE O BUFFER PARA UM PONTEIRO PARA ENTRADAS DE DIRETÓRIO
        int total = BLOCK_SIZE / sizeof(dir_entry_t); // CALCULA O NÚMERO TOTAL DE ENTRADAS DE DIRETÓRIO NO BLOCO

        for (int i = 0; i < total; i++)
        {
            if (entryes[i].inode != 0)
            {
                if (strcmp_local(entryes[i].name, name) == 0)
                {
                    return entryes[i].inode; // SE O NOME DA ENTRADA DE DIRETÓRIO FOR IGUAL AO NOME PROCURADO, RETORNA O NÚMERO DO INODE
                }
            }
        }
    }
    return -1; // SE NÃO ENCONTRAR A ENTRADA DE DIRETÓRIO, RETORNA -1
}

static int add_dir_entry(inode_t *dir, const char *name, uint32_t inode_id)
{
    if (dir == 0)
    {
        return -1; // SE O INODE DO DIRETÓRIO FOR NULO, RETORNA -1
    }

    if (dir->type != FS_DIR)
    {
        return -1; // SE O INODE NÃO FOR UM DIRETÓRIO, RETORNA -1
    }

    uint8_t buffer[BLOCK_SIZE];

    for (int b = 0; b < 8; b++)
    {
        if (dir->blocks[b] == 0)
        {
            // ALOCAR UM NOVO BLOCO PARA O DIRETÓRIO
            int new_block = block_alloc();
            if (new_block < 0)
            {
                return -1; // SE NÃO CONSEGUIR ALOCAR UM NOVO BLOCO, RETORNA -1
            }

            dir->blocks[b] = new_block;       // ATRIBUI O NOVO BLOCO AO DIRETÓRIO
            clear_buffer(buffer, BLOCK_SIZE); // LIMPA O BUFFER
        }
        else
        {
            block_read(dir->blocks[b], buffer); // LÊ O BLOCO DO DIRETÓRIO PARA O BUFFER
        }

        dir_entry_t *entryes = (dir_entry_t *)buffer; // CONVERTE O BUFFER PARA UM PONTEIRO PARA ENTRADAS DE DIRETÓRIO
        int total = BLOCK_SIZE / sizeof(dir_entry_t); // CALCULA O NÚMERO TOTAL DE ENTRADAS DE DIRETÓRIO NO BLOCO

        for (int i = 0; i < total; i++)
        {
            if (entryes[i].inode == 0)
            {
                entryes[i].inode = inode_id;         // ATRIBUI O NÚMERO DO INODE À ENTRADA DE DIRETÓRIO
                strcpy_local(entryes[i].name, name); // COPIA O NOME PARA A ENTRADA DE DIRETÓRIO
                block_write(dir->blocks[b], buffer); // ESCREVE O BUFFER DE VOLTA PARA O BLOCO DO DIRETÓRIO
                dir->size += sizeof(dir_entry_t);    // ATUALIZA O TAMANHO DO DIRETÓRIO

                return 0; // RETORNA 0 PARA INDICAR SUCESSO
            }
        }
    }
    return -1; // SE NÃO CONSEGUIR ADICIONAR A ENTRADA DE DIRETÓRIO, RETORNA -1
}

static int split_path(const char *path, char *parent, char *name)
{
    uint32_t len = strlen_local(path);

    if (len == 0)
    {
        return -1; // SE O CAMINHO FOR VAZIO, RETORNA -1
    }

    if (path[0] != '/')
    {
        return -1; // SE O CAMINHO NÃO COMEÇAR COM '/', RETORNA -1
    }

    uint32_t last_slash = 0;

    for (uint32_t i = 0; i < len; i++)
    {
        if (path[i] == '/')
        {
            last_slash = i; // ARMAZENA A POSIÇÃO DO ÚLTIMO '/' ENCONTRADO
        }
    }

    if (last_slash == 0)
    {
        parent[0] = '/';  // SE O ÚLTIMO '/' FOR O PRIMEIRO CARACTERE, O PAI É A RAIZ
        parent[1] = '\0'; // TERMINA A STRING

        uint32_t j = 0;

        for (uint32_t i = 1; i < len; i++)
        {
            name[j] = path[i]; // COPIA O NOME DO ARQUIVO PARA A STRING 'name'
            j++;
        }
        name[j] = '\0'; // TERMINA A STRING
        return 0;       // RETORNA 0 PARA INDICAR SUCESSO
    }

    for (uint32_t i = 0; i < last_slash; i++)
    {
        parent[i] = path[i]; // COPIA O CAMINHO DO PAI PARA A STRING 'parent'
    }

    parent[last_slash] = '\0'; // TERMINA A STRING

    uint32_t j = 0;
    for (uint32_t i = last_slash + 1; i < len; i++)
    {
        name[j] = path[i]; // COPIA O NOME DO ARQUIVO PARA A STRING 'name'
        j++;
    }

    name[j] = '\0'; // TERMINA A STRING
    return 0;       // RETORNA 0 PARA INDICAR SUCESSO
}

int fs_init(void)
{
    superblock.magic = TREEFS_MAGIC;      // ATRIBUI O VALOR MÁGICO AO SUPERBLOCO
    superblock.total_blocks = MAX_BLOCKS; // ATRIBUI O NÚMERO TOTAL DE BLOCOS AO SUPERBLOCO
    superblock.total_inodes = MAX_INODES; // ATRIBUI O NÚMERO TOTAL DE INODES AO SUPERBLOCO
    superblock.block_size = BLOCK_SIZE;   // ATRIBUI O TAMANHO DO BLO

    block_init();
    inode_init();

    inode_t *root = inode_alloc();

    if (root == 0)
    {
        return -1; // SE NÃO CONSEGUIR ALOCAR O INODE RAIZ, RETORNA -1
    }

    root_inode_id = inode_number(root); // ATRIBUI O NÚMERO DO INODE RAIZ À VARIÁVEL 'root_inode_id'
    root->type = FS_DIR;
    root->size = 0;
    root->links = 1;

    int root_block = block_alloc();
    if (root_block < 0)
    {
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

inode_t *path_lookup(const char *path)
{
    if (path == 0)
        return 0;

    if (path[0] != '/')
        return 0;

    if (path[1] == '\0')
        return inode_get(root_inode_id);

    inode_t *current = inode_get(root_inode_id);

    char component[32];
    uint32_t i = 1;

    while (1)
    {
        uint32_t j = 0;

        // Copia o próximo componente
        while (path[i] != '/' && path[i] != '\0')
        {
            component[j++] = path[i++];
        }

        component[j] = '\0';

        // Procura esse componente no diretório atual
        int inode_id = find_dir_entry(current, component);

        if (inode_id < 0)
        {
            return 0;
        }

        current = inode_get(inode_id);

        if (path[i] == '\0')
        {
            return current;
        }

        // Pula a '/'
        i++;
    }
}

int mkdir(const char *path)
{
    char parent[128];
    char name[32];

    // Divide o caminho em pai e nome
    if (split_path(path, parent, name) != 0)
        return -1;

    // Localiza o diretório pai
    inode_t *parent_inode = path_lookup(parent);

    if (parent_inode == 0)
        return -1;

    // Verifica se já existe um arquivo/diretório com esse nome
    if (find_dir_entry(parent_inode, name) >= 0)
        return -1;

    // Aloca um novo inode
    inode_t *new_inode = inode_alloc();

    if (new_inode == 0)
        return -1;

    // Aloca um bloco para o novo diretório
    int block = block_alloc();

    if (block < 0)
    {
        inode_free(inode_number(new_inode)); // Libera o inode alocado
        return -1;
    }

    // Inicializa o inode
    new_inode->type = FS_DIR;
    new_inode->size = 0;
    new_inode->links = 1;

    for (int i = 0; i < 8; i++)
        new_inode->blocks[i] = 0;

    new_inode->blocks[0] = block;

    // Limpa o bloco do diretório
    uint8_t buffer[BLOCK_SIZE];
    clear_buffer(buffer, BLOCK_SIZE);
    block_write(block, buffer);

    // Adiciona o diretório ao pai
    if (add_dir_entry(parent_inode, name, inode_number(new_inode)) != 0)
    {
        block_free(block);                   // Libera o bloco alocado
        inode_free(inode_number(new_inode)); // Libera o inode alocado
        return -1;
    }
    return 0;
}

int create(const char *path)
{
    char parent[128];
    char name[32];

    if (split_path(path, parent, name) != 0)
        return -1;

    inode_t *parent_inode = path_lookup(parent);

    if (parent_inode == 0)
        return -1;
    if (parent_inode->type != FS_DIR)
        return -1;
    if (find_dir_entry(parent_inode, name) >= 0)
        return -1;

    inode_t *new_inode = inode_alloc();
    if (new_inode == 0)
        return -1;

    new_inode->type = FS_FILE;
    new_inode->size = 0;
    new_inode->links = 1;

    for (int i = 0; i < 8; i++)
    {
        new_inode->blocks[i] = 0;
    }

    uint32_t new_inode_id = inode_number(new_inode);
    if (add_dir_entry(parent_inode, name, new_inode_id) != 0)
    {
        inode_free(new_inode_id);
        return -1;
    }

    return new_inode_id;
}

int ls(const char *path)
{
    inode_t *dir = path_lookup(path);

    if (dir == 0)
        return -1;
    if (dir->type != FS_DIR)
        return -1;

    uint8_t buffer[BLOCK_SIZE];

    for (int b = 0; b < 8; b++){
        if (dir->blocks[b] == 0)
        {
            continue;
        }

        block_read(dir->blocks[b], buffer);

        dir_entry_t *entryes = (dir_entry_t *)buffer;
        int total = BLOCK_SIZE / sizeof(dir_entry_t);

        for (int i = 0; i < total; i++){
            if (entryes[i].inode != 0)
            {
                uart_print(entryes[i].name);
                uart_print("\n");
            }
        }
    }
    return 0;
}

int write(int fd, const void *buf, uint32_t size)
{
    if (fd < 0 || fd >= MAX_INODES)
    {
        return -1; // Verifica se o descritor de arquivo é válido
    }

    inode_t *node = inode_get(fd); //OBTEM O PONTEIRO PARA O INODE CORRESPONDENTE AO DESCRITOR DE ARQUIVO

    if (node == 0 || node->type != FS_FILE)
    {
        return -1;
    }

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t bytes_written = 0;
    uint32_t block_index = 0;

    while (bytes_written < size && block_index < 8){
        if (node->blocks[block_index] == 0){
            int new_block = block_alloc();
            if (new_block < 0)
            {
                break; // Se não conseguir alocar um novo bloco, sai do loop
            }
            node->blocks[block_index] = new_block; // Atribui o novo bloco ao inode
        }
        uint8_t block_buffer[BLOCK_SIZE];
        clear_buffer(block_buffer, BLOCK_SIZE);

        uint32_t remaining = size - bytes_written;
        uint32_t to_copy = remaining;

        if (to_copy > BLOCK_SIZE)
        {
            to_copy = BLOCK_SIZE; // Limita a quantidade de bytes a copiar ao tamanho do bloco
        }

        for (uint32_t i = 0; i < to_copy; i++){
            block_buffer[i] = src[bytes_written + i]; // Copia os dados do buffer de entrada para o buffer do bloco
        }
        block_write(node->blocks[block_index], block_buffer); // Escreve o buffer do bloco no disco
        bytes_written += to_copy;
        block_index++;
    }

    node->size = bytes_written; // Atualiza o tamanho do arquivo no inode
    return bytes_written;
}

int read(int fd, void *buf, uint32_t size){
    if (fd < 0 || fd >= MAX_INODES){
        return -1;
    }
    inode_t *node = inode_get(fd);

    if (node == 0 || node->type != FS_FILE){
        return -1;
    }

    uint8_t *dst = (uint8_t *)buf;
    uint32_t bytes_read = 0;
    uint32_t block_index = 0;
    uint32_t limit = size;

    if (limit > node->size)
    {
        limit = node->size; // Limita a quantidade de bytes a ler ao tamanho do arquivo
    }

    while (bytes_read < limit && block_index < 8){
        if (node->blocks[block_index] == 0){
            break;
        }

        uint8_t block_buffer[BLOCK_SIZE];
        block_read(node->blocks[block_index], block_buffer);

        uint32_t remaining = limit - bytes_read;
        uint32_t to_copy = remaining;

        if (to_copy > BLOCK_SIZE)
        {
            to_copy = BLOCK_SIZE; // Limita a quantidade de bytes a copiar ao tamanho do bloco
        }

        for (uint32_t i = 0; i < to_copy; i++){
            dst[bytes_read + i] = block_buffer[i]; // Copia os dados do buffer do bloco para o buffer de saída
        }

        bytes_read += to_copy;
        block_index++;
    }
    return bytes_read;
}

int unlink(const char *path)
{
    char parent[128];
    char name[32];

    if (split_path(path, parent, name) != 0)
        return -1;

    inode_t *parent_inode = path_lookup(parent);

    if (parent_inode == 0)
        return -1;

    int inode_id = find_dir_entry(parent_inode, name);
    if (inode_id < 0)
        return -1;

    inode_t *node = inode_get(inode_id);
    if (node == 0)
        return -1;

    // LIBERA OS BLOCOS

    for (int i = 0; i < 8; i++){
        if (node->blocks[i] != 0)
        {
            block_free(node->blocks[i]);
            node->blocks[i] = 0;
        }
    } 

    // REMOVE A ENTRADA DO DIRETÓRIO

    uint8_t buffer[BLOCK_SIZE];
    for (int b = 0; b < 8; b++){
        if (parent_inode->blocks[b] == 0)
        {
            continue;
        }

        block_read(parent_inode->blocks[b], buffer);
        dir_entry_t *entryes = (dir_entry_t *)buffer;
        int total = BLOCK_SIZE / sizeof(dir_entry_t);

        for (int i = 0; i < total; i++){
            if (entryes[i].inode == inode_id) // SE A ENTRADA DE DIRETÓRIO CORRESPONDER AO INODE A SER REMOVIDO
            {
                entryes[i].inode = 0;
                entryes[i].name[0] = '\0';
                block_write(parent_inode->blocks[b], buffer); // ESCREVE O BUFFER DE VOLTA PARA O BLOCO DO DIRETÓRIO
                inode_free(inode_id);
                return 0;
            }
        }
    }
    return -1;
}