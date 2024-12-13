#include "memory_manager.h"
#define MemorySize 1024
#define MAX_METADATA_BLOCKS 1000

static void *memory_pool = NULL;           // Preallocated memory pool
static size_t pool_size = 0;               // Total size of the memory pool
static memory_block *metadata_head = NULL; // Head of the metadata linked list

void mem_init(size_t size)
{

    if (size == 0)
    {
        printf("Can't Initialization zero memory\n");
        return;
    }

    memory_pool = malloc(size);
    if (!memory_pool)
    {
        printf("Memory initialization failed!\n");
        return;
    }

    metadata_head = (memory_block *)malloc(sizeof(memory_block));
    if (!metadata_head)
    {
        printf("Failed to allocate metadata array!\n");
        free(memory_pool);
        memory_pool = NULL;
        return;
    }
    // Initialzie metadata

    metadata_head->size = size;
    metadata_head->free = 1; // 1 indicates the block is free
    metadata_head->offset = memory_pool;
    metadata_head->next = NULL;
    pthread_mutex_init(&metadata_head->lock, NULL);
};

// Initialize the memory with a single large block
void *mem_alloc(size_t size)
{
    memory_block *current = metadata_head;

    // Traverse the list to find the first free block that can be used
    while (current != NULL)
    {
        pthread_mutex_lock(&current->lock); // Lock the current block
        if (current->free && current->size >= size)
        {
            // Look if free block larger then needed, then split it
            if (current->size > size)
            {
                memory_block *new_block = (memory_block *)malloc(sizeof(memory_block));
                if (!new_block)
                {
                    pthread_mutex_unlock(&current->lock); // unlock for safety 
                    printf("Error: Metadata allocation failed.\n");
                    return NULL;
                }

                // Initialize the new block
                new_block->size = current->size - size;
                new_block->free = 1;
                new_block->next = current->next;
                new_block->offset = (char *)current->offset + size;
                pthread_mutex_init(&new_block->lock, NULL);

                // Update the current block
                current->size = size;
                current->free = 0;
                current->next = new_block;

                pthread_mutex_unlock(&current->lock); // unlock current block before leaving function

                return current->offset;
            }

            // Allocate the entire block
            current->free = 0;
            pthread_mutex_unlock(&current->lock);
            return current->offset;
        }
        pthread_mutex_unlock(&current->lock); // Unlock the current block before moving to the next
        current = current->next;
    }
    printf("Allocation failed: no sufficient memory left.\n");
    return NULL;
}

// help functions
memory_block *get_block_from_ptr(void *ptr)
{
    if (!ptr)
        return NULL;
    return (memory_block *)ptr - 1; // Go back to the block metadata
}

void mem_free(void *ptr)
{
    if (!ptr || !memory_pool)
    {
        printf("Invaild pointer or uninitialized memory pool");
        return;
    }

    memory_block *current = metadata_head;

    // Find the correct block to free
    while (current != NULL)
    {
        pthread_mutex_lock(&current->lock); // Look the current block
        if (current->offset == ptr)
        {
            current->free = 1; // Mark the block as free

            // Merge adjacent free blocks
            memory_block *next = current->next;
            if(next != NULL)
            {
                pthread_mutex_lock(&next->lock); // Lock the next block for merging
                if (next->free == 1)
                {
                    current->size += next->size;
                    current->next = next->next;

                    pthread_mutex_unlock(&next->lock);
                    pthread_mutex_destroy(&next->lock);
                    free(next); // Free the metadata for the merged block
                }
                else {
                    pthread_mutex_unlock(&next->lock);
                }
            }
            pthread_mutex_unlock(&current->lock);
            return;
        }
        pthread_mutex_unlock(&current->lock);
        current = current->next;
    }
}

void *mem_resize(void *block, size_t size)
{

    if (!block) // no block
    {
        // allocate a new block
        return mem_alloc(size);
    }

    memory_block *current_block = metadata_head;

    while (current_block != NULL)
    {
        pthread_mutex_lock(&current_block->lock); // Lock the current block

        // check address is the same and the size needs to uppgrade
        if(current_block->offset == block && current_block->size < size)
        {
            // check if next block is free and has enought memory
            memory_block *next = current_block->next;
            if(next != NULL)
            {
                pthread_mutex_lock(&next->lock); // Lock the next block
                if (next->free &&
                    (current_block->size + current_block->next->size >= size))
                {
                    // Merge and resize
                    current_block->size += next->size;
                    current_block->next = next->next;
                    pthread_mutex_unlock(&next->lock);
                    pthread_mutex_destroy(&next->lock);
                    free(next);
                    pthread_mutex_unlock(&current_block->lock);
                    return block;
                }
                else
                {
                    // Allocate a new block, copy data, and free the old block
                    void *new_ptr = mem_alloc(size);
                    pthread_mutex_unlock(&next->lock);
                    pthread_mutex_unlock(&current_block->lock); // needed for mem_free
                    if (new_ptr)
                    {
                        memcpy(new_ptr, block, current_block->size);
                        mem_free(block);
                    }
                    return new_ptr;
                }
            }
           
        }
        pthread_mutex_unlock(&current_block->lock);
        current_block = current_block->next;
    }
    return NULL; // Block not found
}

void mem_deinit()
{
    // Free all metadata
    memory_block *current = metadata_head;
    while (current)
    {
        pthread_mutex_lock(&current->lock);
        memory_block *next = current->next;
        pthread_mutex_unlock(&current->lock);
        free(current);
        current = next;
    }

    // Free the memory pool
    free(memory_pool);
    memory_pool = NULL;
    metadata_head = NULL;
    pool_size = 0;
    printf("Memory pool deinitialized.\n");
}

// Display the memory state (for debugging)
void display_memory()
{
    printf("Memory pool: %p, size: %zu\n", memory_pool, pool_size);
    memory_block *current = metadata_head;
    while (current)
    {
        printf("[Block at %p: size=%zu, free=%d]\n", current->offset, current->size, current->free);
        current = current->next;
    }
}
/*
#define my_assert(expr)                                                                         \
    do                                                                                          \
    {                                                                                           \
        if (!(expr))                                                                            \
        {                                                                                       \
            printf("Assertion failed: %s, file %s, line %d.\n", #expr, __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                                 \
        }                                                                                       \
    } while (0)

int main()
{
    printf("  Testing non-contiguous allocation failure ---> ");
    mem_init(800); // Initialize with 800 bytes of memory

    // Allocate several blocks to fragment the memory
    void *block1 = mem_alloc(250);
    void *block2 = mem_alloc(250);
    void *block3 = mem_alloc(250);
    mem_free(block1); // Free the first block
    mem_free(block3); // Free the third block, leaving non-contiguous free slots

    // Attempt to allocate a block larger than any single free block but smaller than the total free space
    void *block4 = mem_alloc(500);
    my_assert(block4 == NULL); // This allocation should fail due to lack of contiguous space

    mem_free(block2); // Cleanup
    mem_deinit();
    printf("[PASS].\n");
}*/