#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "buddy.h"

#define BUDDY_BLOCK_ORDER 6 /**< @brief Minimum block order */

buddy_list_bucket_t *buddy_master = 0;

/**
 */
void buddy_free(void *ptr)
{
    buddy_list_t *blt = ptr;
    blt--;

    // Now blt is is pointing to the correct address
    unsigned int size = blt->size + sizeof(buddy_list_t);
}

/**
 * This function assumes that a block of the specified order exists.
 */
void buddy_split(buddy_list_bucket_t *bucket)
{
    assert(bucket->count && "Bucket contains no entries!");

    // Remove an entry from this bucket and adjust the count
    buddy_list_t *blt = LIST_FIRST(&bucket->ptr);
    bucket->count--;
    LIST_REMOVE(blt, next_freelist);

    // Add two to the lower order bucket
    bucket--;
    bucket->count += 2;

    // Update the BLTs
    blt->use = FREE;
    blt->size = (1 << bucket->order) - sizeof(buddy_list_t);

    void *address = ((char *)blt) + (1 << bucket->order);
    buddy_list_t *new_blt = address;
    printf("address of new_blt is %p\n", new_blt);

    // new_blt->next_freelist = NULL;
    new_blt->use = FREE;
    new_blt->size = (1 << bucket->order) - sizeof(buddy_list_t);

    LIST_INSERT_HEAD(&bucket->ptr, new_blt, next_freelist);
    LIST_INSERT_HEAD(&bucket->ptr, blt, next_freelist);
}

/**
 * Finds the next power of 2 or, if v is a power of 2, return that.
 * From http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
 */
unsigned int
next_power2(unsigned int v)
{
    // We're not allocating chunks smaller than 2^BUDDY_BLOCK_ORDER bytes
    if (v < (1 << BUDDY_BLOCK_ORDER)) {
        return (1 << BUDDY_BLOCK_ORDER);
    }

    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

/**
 * Find the smallest block that will contain size and return it.
 * Note this returns the memory allocated and usable, not the entire buffer.
 * This may involve breaking up larger blocks.
 */
void *buddy_alloc(unsigned size)
{
    char *ret = 0; // Return value

    // We'll prepend a BLT before each allocation so add that now
    size += sizeof(buddy_list_t);
    size = next_power2(size);

    // Find the bucket we need
    buddy_list_bucket_t *blbt = buddy_master;
    while (size > (1 << blbt->order)) {
        printf("%d > %d\n", size, 1 << blbt->order);
        blbt++;
    }

    printf("target: %d-sized block\n", 1 << blbt->order);

    if (blbt->count == 0) {
        unsigned split_count = 0;

        // If there are none, keep moving up to larger sizes
        while (!blbt->count) {
            blbt++;
            split_count++;
        }

        while (split_count--) {
            buddy_split(blbt--);
        }
    }

    buddy_list_t *blt = LIST_FIRST(&blbt->ptr);
    LIST_REMOVE(blt, next_freelist);
    blbt->count--;
    ret = (char *)blt;
    ret += sizeof(buddy_list_t);
    return ret;
}

/**
 * Pass in the power of two e.g., passing 5 will yield 2^5 = 32.
 * The smallest order we'll create will be 32 so this would yield one list.
 */
buddy_list_bucket_t * create_buddy_table(unsigned int power_of_two)
{
    int i;
    int size;
    int list_count;
    void *memory;
    buddy_list_bucket_t *bsystem;

    if (power_of_two < BUDDY_BLOCK_ORDER) {
        power_of_two = BUDDY_BLOCK_ORDER;
    }

    list_count = power_of_two - BUDDY_BLOCK_ORDER + 1;

    bsystem = calloc(list_count, sizeof(buddy_list_bucket_t));

    for (i = 0; i < list_count; i++) {
        bsystem[i].count = 0;
        bsystem[i].order = i + BUDDY_BLOCK_ORDER;
        LIST_INIT(&(bsystem[i].ptr));
    }

    // Allocate the memory
    size = 1 << power_of_two;
    printf("Allocating %d bytes\n", size);
    memory = calloc(1, size);
    printf("memory is %p\n", memory);

    // Set up the primordial buddy block (2^power_of_two)
    buddy_list_t *primordial = memory;
    primordial->use       = FREE;
    primordial->size      = (1 << power_of_two) - sizeof(buddy_list_t);

    bsystem[list_count - 1].count = 1;
    LIST_INSERT_HEAD(&bsystem[list_count - 1].ptr, primordial, next_freelist);

    return bsystem;
}
