#include "task.h"
#include "scheduler.h"
#include "memory.h"
#include "fs.h"
#include "inode.h"
#include <stdint.h>

extern void uart_print(const char*);
extern void uart_print_uint(uint64_t);

/*   Tasks   */

void task1()
{
    while (1)
    {
        uart_print("Task 1 running\n");
        yield();
    }
}

void task2()
{
    while (1)
    {
        uart_print("Task 2 running\n");
        yield();
    }
}

/*   Kernel   */

void kernel_main()
{
    memory_init();

    uart_print("\n=== TESTE DO HEAP ===\n");

    uart_print("Heap total: ");
    uart_print_uint(memory_total());
    uart_print(" bytes\n");

    uart_print("Heap usado: ");
    uart_print_uint(memory_used());
    uart_print(" bytes\n");

    uart_print("Heap livre: ");
    uart_print_uint(memory_free());
    uart_print(" bytes\n\n");

    uart_print("=== TESTE TREEFS ===\n");

    fs_init();

    uart_print("\nListagem da raiz:\n");
    ls("/");

    uart_print("\nCriando /home/aluno\n");
    mkdir("/home/aluno");

    uart_print("\nListagem de /home:\n");
    ls("/home");

    uart_print("\nCriando /home/aluno/notas.txt\n");
    int fd = create("/home/aluno/notas.txt");

    if (fd >= 0)
    {
        uart_print("Arquivo criado com fd/inode: ");
        uart_print_uint(fd);
        uart_print("\n");
    }
    else
    {
        uart_print("Erro ao criar arquivo\n");
    }

    uart_print("\nListagem de /home/aluno:\n");
    ls("/home/aluno");

    uart_print("\nEscrevendo no arquivo...\n");
    write(fd, "Sistemas Operacionais", 22);

    char buffer[64];

    for (int i = 0; i < 64; i++)
    {
        buffer[i] = 0;
    }

    uart_print("Lendo arquivo:\n");
    read(fd, buffer, 22);

    buffer[22] = '\0';

    uart_print(buffer);
    uart_print("\n");

    uart_print("\nRemovendo /home/aluno/notas.txt\n");
    unlink("/home/aluno/notas.txt");

    uart_print("\nListagem de /home/aluno apos remover:\n");
    ls("/home/aluno");

    uart_print("\n=== TESTE DE TASKS ===\n");

    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);

    scheduler_start();

    while (1);
}