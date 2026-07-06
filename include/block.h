#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

void block_init(void);

int block_read(uint32_t block, void *buf);
int block_write(uint32_t block, const void *buf);

int block_alloc(void);
void block_free(uint32_t block);

#endif