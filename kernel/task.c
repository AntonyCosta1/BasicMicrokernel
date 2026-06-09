#include "task.h"
#include "memory.h"
#include <stdint.h>

TCB tasks[MAX_TASKS];
int task_count = 0;

void xTaskCreate(void (*task)(void),
                 uint32_t stack_size,
                 int priority)
{
    if (task_count >= MAX_TASKS)
        return;

    if (stack_size == 0)
    stack_size = 2048;

    TCB *t = &tasks[task_count];

    t->entry = task;
    t->priority = priority;
    t->state = 0;
    t->stack_size = stack_size;

    t->stack = (uint8_t*)kmalloc(stack_size);
    if (!t->stack) return;

    uint64_t *sp = (uint64_t*)(t->stack + stack_size);

    /* Configurar contexto inicial */

    t->regs[0] = (uint64_t)task;   // ra
    t->regs[1] = (uint64_t)sp;     // sp
    t->sepc = (uint64_t)task;

    task_count++;
}