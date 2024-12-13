#include "linked_list.h"
#include "memory_manager.h"



void list_init(Node **head, size_t size){
    // Set up memory pool 

    mem_init(size);
    // make sure the lsist empty 
    
    *head = NULL;
    printf("init done\n");
}

void list_insert(Node **head, uint16_t data){

    // allocate memory for the new node 
    Node *new_node = (Node *)mem_alloc(sizeof(Node));

    if(new_node == NULL)
    {
        printf("Error: Memory allocation failed");
        return;
    }

    pthread_mutex_init(&new_node->lock, NULL);
    new_node->data = data;
    new_node->next = NULL;

    if(*head == NULL)
    {
        pthread_mutex_lock(&new_node->lock);
        *head = new_node;
        pthread_mutex_unlock(&new_node->lock);
        return;
    }

    Node* current = *head;
    pthread_mutex_lock(&current->lock);
    while (current->next != NULL)
    {
        Node *next = current->next;
        pthread_mutex_lock(&next->lock);      // Lock the next node
        pthread_mutex_unlock(&current->lock); // Unlock the current node
        current = next;
    }

    
    current->next = new_node;
    pthread_mutex_unlock(&current->lock);
    //printf("insert done");
}

void list_insert_after(Node *prev_node, uint16_t data){
    
    if (prev_node == NULL)
    {
        printf("Error: Previous node cannot be NULL\n");
        return;
    }

    // allocate memory for the new node
    Node *new_node = (Node *)mem_alloc(sizeof(Node));

    if (new_node == NULL)
    {
        printf("Error: Memory allocation failed");
        return;
    }
    pthread_mutex_init(&new_node->lock, NULL);
    new_node->data = data;

    pthread_mutex_lock(&prev_node->lock);
    new_node->next = prev_node->next;

    // Update the next pointer of the previous node to point to the new node
    prev_node->next = new_node;
    pthread_mutex_unlock(&prev_node->lock);
}

void list_insert_before(Node **head, Node *next_node, uint16_t data)
{
    if (*head == NULL || next_node == NULL)
    {
        printf("Error: The list is empty or the next_node is NULL\n");
        return;
    }

    // allocate memory for the new node
    Node *new_node = (Node *)mem_alloc(sizeof(Node));

    if (new_node == NULL)
    {
        printf("Error: Memory allocation failed");
        return;
    }

    pthread_mutex_init(&new_node->lock, NULL);
    new_node->data = data;
    new_node->next = NULL;

    Node *current = *head;
    // Check if the next_node is the head of the list
    pthread_mutex_lock(&current->lock);
    if (*head == next_node)
    {
        new_node->next = *head;
        pthread_mutex_unlock(&current->lock);
        *head = new_node;
        
        return;
    }
    pthread_mutex_unlock(&current->lock);

    // Traverse the list to find the node before next_node
    while (current != NULL)
    {
        pthread_mutex_lock(&current->lock);

        if (current->next == next_node)
        {
            pthread_mutex_lock(&next_node->lock);

            // Insert the new node
            new_node->next = next_node;
            current->next = new_node;

            pthread_mutex_unlock(&next_node->lock);
            pthread_mutex_unlock(&current->lock);
            return;
        }

        Node *temp = current->next; // Move to the next node
        pthread_mutex_unlock(&current->lock);
        current = temp;
    }

    // If next_node is not found in the list
    printf("Error: next_node not found in the list\n");
    mem_free(new_node); // Free the allocated memory if insertion fails
    return;
}

void list_delete(Node **head, uint16_t data)
{
    // Check if the list is empty
    if (*head == NULL)
    {
        printf("Error: The list is empty\n");
        return;
    }

    // Set up for eazy deletion
    
    Node* current = *head;
    Node* prev = NULL;

    pthread_mutex_lock(&current->lock); // make sure current-> data is not modified

    // check if head holds the data to be deleted
    if (current->data == data)
    {
        if (*head != NULL)
        {
            // Lock the new head before unlocking the current head
            pthread_mutex_lock(&(*head)->lock);
        }

        pthread_mutex_unlock(&current->lock);
        pthread_mutex_destroy(&current->lock);
        
        mem_free(current);
        if (*head != NULL)
        {
            pthread_mutex_unlock(&(*head)->lock); // Unlock the new head
        }

        return;
    }

    // Traverse the list to find the node to delete
    while (current != NULL)
    {
        Node *next_node = current->next;

        if (next_node != NULL)
        {
            pthread_mutex_lock(&next_node->lock); // Lock the next node
        }

        if (current->data == data)
        {
            if (prev != NULL)
            {
                prev->next = next_node; // Remove the node from the list
                pthread_mutex_unlock(&prev->lock);
            }

            pthread_mutex_unlock(&current->lock); // Unlock the node to be deleted
            pthread_mutex_destroy(&current->lock);
            mem_free(current);                    // Free the memory of the deleted node

            if (next_node != NULL)
            {
                pthread_mutex_unlock(&next_node->lock); // Unlock the next node
            }

            return;
        }

        if (prev != NULL)
        {
            pthread_mutex_unlock(&prev->lock); // Unlock the previous node
        }

        prev = current;
        current = next_node;
    }

    if (prev != NULL)
    {
        pthread_mutex_unlock(&prev->lock); // Unlock the last valid node
    }

    printf("Error: Data not found in the list\n");
}

Node* list_search(Node **head, uint16_t data){
    // Check if the list is empty
    if (*head == NULL)
    {
        printf("Error: The list is empty\n");
        return NULL;
    }

    Node *current = *head;
    pthread_mutex_lock(&current->lock);

    // Traverse the list to find the node with the given data
    while (current != NULL)
    {
        if (current->data == data)
        {
            pthread_mutex_unlock(&current->lock);
            return current; 
        }

        Node *next = current->next;
        pthread_mutex_unlock(&current->lock);

        if (next) // make sure that the current->data do not get updated before checking it in the next iteration
        {
            pthread_mutex_lock(&next->lock); // Lock the next node
        }

        current = current->next;
        
    }

    printf("Error: Data not found in the list\n");
    return NULL;
}

void list_display(Node **head){
    // Check if the list is empty
    if (*head == NULL)
    {
        printf("Error: The list is empty\n");
        return;
    }
    Node *current = *head;
    pthread_mutex_lock(&current->lock);

    // Traverse the list and print
    printf("[");
    while (current != NULL)
    {
        printf("%u,", current->data);
        Node *next = current->next;
        pthread_mutex_unlock(&current->lock);

        if (next)
        {
            pthread_mutex_lock(&next->lock); // Lock the next node
        }

        current = current->next;
        if (current != NULL)
        {
            printf(", ");
        }
    }
    printf("]\n");
}

void list_display_range(Node **head, Node *start_node, Node *end_node)
{
    // Check if the list is empty
    if (*head == NULL)
    {
        printf("Error: The list is empty\n");
        return;
    }

    Node *current = *head;
    if (start_node == NULL)
    {
        start_node = *head;
    }

    // Traverse the list until start point reached  
    while (current != NULL && current != start_node)
    {
        Node *next = current->next;
        pthread_mutex_unlock(&current->lock);
        if (next)
        {
            pthread_mutex_lock(&next->lock); // Lock the next node
        }
        current = next;
    }

    if (current == NULL)
    {
        printf("Error: start_node not found in the list\n");
        return;
    }

    // travers the list and print start to end_node(end)
    printf("[");
    while (current != NULL)
    {
        printf("%u", current->data);

        // Stop if the current node is the end_node
        if (current == end_node)
        {
            pthread_mutex_unlock(&current->lock);
            break;
        }
        Node *next = current->next;
        pthread_mutex_unlock(&current->lock);

        if (next)
        {
            pthread_mutex_lock(&next->lock); // Lock the next node
        }

        current = current->next;
        if (current != NULL)
        {
            printf(", ");
        }
    }
    printf("]");
}

int list_count_nodes(Node **head)
{
    // Check if the list is empty
    if (*head == NULL)
    {
        printf("Error: The list is empty\n");
        return 0;
    }

    int count = 0;
    Node *current = *head;

    pthread_mutex_lock(&current->lock);

    // Travertse the list and increment the count
    while (current != NULL)
    {
        count++;
        Node *next = current->next;
        pthread_mutex_unlock(&current->lock);
        
        if (next)
        {
            pthread_mutex_lock(&next->lock); // Lock the next node
        }
        current = current->next;
    }

    return count;
}

void list_cleanup(Node **head){
    // Check if the list is empty
    if (*head == NULL)
    {
        printf("Error: The list is empty\n");
        return;
    }

    
    
    pthread_mutex_lock(&(*head)->lock);
    

    Node *current = *head;
    Node *next_node = NULL; // node_next exist to make it easy to free current node

    // Traverse the list and free each node
    while (current != NULL)
    {
        next_node = current->next;
        if (next_node != NULL)
        {
            pthread_mutex_lock(&next_node->lock);
        }
        mem_free(current);         
        current = next_node;       
    }

    *head = NULL;
}

// LD_LIBRARY_PATH=. ./test_memory_manager