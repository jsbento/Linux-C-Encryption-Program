/*
Implements the prototypes defined in circ-buffer.h.
Buffers are created with a maximum size, checked
when nodes want to be added.
*/

#include "circ-buffer.h"
#include <stdlib.h>
#include <stdio.h>

// Returns a pointer to the buffer or NULL.
// Returns NULL if either the buffer or
// the head and tail nodes cannot be created
// otherwise returns buffer pointer
struct buffer_t* initBuffer(int max) {
    struct buffer_t *buffer;
    if((buffer = malloc(sizeof(struct buffer_t))) == 0) return NULL;

    struct node_t *head;
    struct node_t *tail;
    if((head = malloc(sizeof(struct node_t))) == 0 || (tail = malloc(sizeof(struct node_t))) == 0) {
        free(buffer);
        if(head) free(head);
        return NULL;
    }

    buffer->head = head;
    buffer->tail = tail;
    buffer->head->data = -1;
    buffer->tail->data = -1;
    buffer->head->counted = -1;
    buffer->head->counted = -1;
    buffer->head->next = tail;
    buffer->head->prev = tail;
    buffer->tail->next = head;
    buffer->tail->prev = head;
    buffer->maxSize = max;
    buffer->size = 0;

    return buffer;
}

// Allocates a new node for the buffer.
// Returns NULL if node cannot be created,
// otherwise a pointer to the node
struct node_t* allocNode(char c) {
    struct node_t *node;
    if((node = malloc(sizeof(struct node_t))) == 0) return NULL;

    node->data = c;
    node->counted = 0;

    return node;
}

// Adds a new node to the buffer.
// Returns 0 if buffer is full or node cannot be created
// Returns 1 if node added successfully
int addNode(struct buffer_t *b, char c) {
    if(b->size == b->maxSize) return 0;

    struct node_t *toAdd;
    if((toAdd = allocNode(c)) == NULL) return 0;

    link(b, toAdd);
    b->size = b->size + 1;

    return 1;
}

// Removes the first node in the buffer and
// returns its data
char remNode(struct buffer_t *b) {
    if(b->size == 0) return 0;

    struct node_t *toRet = unlink(b);
    char data = toRet->data;
    free(toRet);
    b->size = b->size - 1;
    return data;
}

// Links a node in the buffer
void link(struct buffer_t *b, struct node_t *n) {
    n->next = b->tail;
    n->prev = b->tail->prev;
    b->tail->prev->next = n;
    b->tail->prev = n;
}

// Unlinks a node from the buffer and
// returns the node
struct node_t* unlink(struct buffer_t *b) {
    struct node_t *toRet = b->head->next;
    b->head->next = toRet->next;
    b->head->next->prev = b->head;
    return toRet;
}

// Prints the contents of the buffer
void printBuffer(struct buffer_t *b) {
    if(b->size == 0) {
        printf("[]\n");
        return;
    }

    struct node_t *curr = b->head->next;

    printf("[");
    while(curr != b->tail) {
        if(curr->next == b->tail)
            printf("%c", curr->data);
        else
            printf("%c, ", curr->data);
        curr = curr->next;
    }
    printf("]\n");
}

// Checks if buffer is empty
int isEmpty(struct buffer_t *b) {
    if(b->size == 0) return 1;
    return 0;
}

// Checks if buffer is full
int isFull(struct buffer_t *b) {
    if(b->size == b->maxSize) return 1;
    return 0;
}

// Frees a buffer by first freeing all nodes,
// then the buffer itself
void freeBuffer(struct buffer_t *b) {
    struct node_t *curr = b->head->next;
    while(curr != b->head) {
        struct node_t *next = curr->next;
        free(curr);
        curr = next;
    }
    free(curr);
    free(b);
}