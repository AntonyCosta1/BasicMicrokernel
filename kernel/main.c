#include "task.h"
#include "scheduler.h"
#include "memory.h"

extern void uart_print(const char*);

/*   Tasks   */

void task1()
{
    while (1)
    {
        uart_print("Task 1 running\n");

        uart_print("Memory used: ");
        uart_print_uint(memory_used());
        uart_print(" bytes\n");

        uart_print("Memory free: ");
        uart_print_uint(memory_free());
        uart_print(" bytes\n");

        uart_print("Memory Total: ");
        uart_print_uint(memory_total());
        uart_print( "bytes\n\n");

        yield();
    }
}

void task2()
{
    while (1)
    {
        uart_print("Task 2 running\n");

        uart_print("Memory used: ");
        uart_print_uint(memory_used());
        uart_print(" bytes\n");

        uart_print("Memory free: ");
        uart_print_uint(memory_free());
        uart_print(" bytes\n");

        uart_print("Memory Total: ");
        uart_print_uint(memory_total());
        uart_print( "bytes\n\n");

        yield();
    }
}

/*   Kernel   */

void kernel_main()
{
    memory_init();   // OBRIGATÓRIO (INICIALIZAR O HEAP)

    uart_print("\n Teste do heap no SO \n");

    // Estatísticas iniciais

    uart_print("Heap total: ");
    uart_print_uint(memory_total());
    uart_print(" bytes\n");

    uart_print("Heap usado: ");
    uart_print_uint(memory_used());
    uart_print(" bytes\n");

    uart_print("Heap livre: ");
    uart_print_uint(memory_free());
    uart_print(" bytes\n\n");

    // TESTE 1 - Múltiplas alocações

    uart_print("TESTE 1 - Multiplas alocacoes\n");

    void *a1 = kmalloc(1024);
    void *b1 = kmalloc(2048);
    void *c1 = kmalloc(512);

    uart_print("Heap usado: ");
    uart_print_uint(memory_used());
    uart_print(" bytes\n");

    uart_print("Heap livre: ");
    uart_print_uint(memory_free());
    uart_print(" bytes\n\n");

    // TESTE 2 - Liberação

    uart_print("TESTE 2 - Liberacao\n");

    kfree(a1);
    kfree(b1);
    kfree(c1);

    uart_print("Heap usado: ");
    uart_print_uint(memory_used());
    uart_print(" bytes\n");

    uart_print("Heap livre: ");
    uart_print_uint(memory_free());
    uart_print(" bytes\n\n");

    //TESTE 3 - Reutilização (First Fit)

    uart_print("TESTE 3 - Reutilizacao\n");

    void *a2 = kmalloc(1024);

    uart_print("Endereco A2: ");
    uart_print_uint((uint64_t)a2);
    uart_print("\n");

    kfree(a2);

    void *b2 = kmalloc(512);

    uart_print("Endereco B2: ");
    uart_print_uint((uint64_t)b2);
    uart_print("\n");

    uart_print("Se os enderecos forem iguais, houve reutilizacao.\n\n");

    // TESTE 4 - Coalescência

    uart_print("TESTE 4 - Coalescencia\n");

    void *a3 = kmalloc(1024);
    void *b3 = kmalloc(1024);

    kfree(a3);
    kfree(b3);

    void *c3 = kmalloc(1800);

    if (c3) {
        uart_print("Coalescencia OK\n");
        uart_print("Endereco: ");
        uart_print_uint((uint64_t)c3);
        uart_print("\n");

         uart_print("Heap usado: ");
        uart_print_uint(memory_used());
        uart_print(" bytes\n");

        uart_print("Heap livre: ");
        uart_print_uint(memory_free());
        uart_print(" bytes\n");
    }else{
        uart_print("Coalescencia FALHOU\n");
    }

    uart_print("\n");

    uart_print("TESTE 5 - Criacao de Tasks\n");

    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);

    uart_print("Heap usado apos criar tasks: ");
    uart_print_uint(memory_used());
    uart_print(" bytes\n");

    uart_print("Heap livre apos criar tasks: ");
    uart_print_uint(memory_free());
    uart_print(" bytes\n\n");


    /*uart_print("\n");

    
    uart_print("\n=== Kernel ===\n");
    
    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);
    
    */

   scheduler_start();

    while (1);
}