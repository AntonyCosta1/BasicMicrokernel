#ifndef INODE_H
#define INODE_H

#include "fs.h"

void inode_init(void);
inode_t *inode_alloc(void);
void inode_free(uint32_t inode);
inode_t *inode_get(uint32_t inode);

#endif // INODE_H