#pragma once
#include <stdint.h>

#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 8

typedef struct
{
    uint64_t regs[31];   // x1–x31 (x0 é zero)
    uint64_t sepc;

    void (*entry)(void);

    int priority;
    int state;

    uint8_t *stack;
    uint64_t stack_size;

} TCB;

extern TCB tasks[MAX_TASKS];
extern int task_count;

void xTaskCreate(void (*task)(void),
                 uint32_t stack_size,
                 int priority);

#endif