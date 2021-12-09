#ifndef _BLUSHER_COLLECTIONS_H
#define _BLUSHER_COLLECTIONS_H

#include <stdint.h>

typedef struct bl_ptr_btree {
    void *btree;
} bl_ptr_btree;

bl_ptr_btree* bl_ptr_btree_new();

bool bl_ptr_btree_insert(bl_ptr_btree *btree, uint32_t val);

bool bl_ptr_btree_remove(bl_ptr_btree *btree, uint32_t val);

void bl_ptr_bree_free(bl_ptr_btree *btree);

#endif /* _BLUSHER_COLLECTIONS_H */
