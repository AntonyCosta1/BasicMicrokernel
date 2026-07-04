#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

int block_alloc(void);
void block_free(uint32_t block);

#endif // BLOCK_H