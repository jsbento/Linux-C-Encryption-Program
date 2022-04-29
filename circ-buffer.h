/*
Defines the necessary components for a circular buffer
with two dummy nodes, head and tail. Contains prototypes
for creating the buffer, creating nodes, adding/removing
nodes, linking/unlinking nodes, printing a buffer,
full/empty check, and freeing a buffer. The backing structure
is a doubly linked list.
*/

#ifndef CIRC_BUFFER_H
#define CIRC_BUFFER_H

// Stores the head and tail pointers,
// size, and max size for a buffer
struct buffer_t {
    struct node_t *head;
    struct node_t *tail;
    int size;
    int maxSize;
};


// Stores the char data, next, and previous
// pointers to nodes in the buffer
struct node_t {
    char data;
    int counted;
    struct node_t *next;
    struct node_t *prev;
};

struct buffer_t * initBuffer(int);
struct node_t * allocNode(char);
int addNode(struct buffer_t *, char);
char remNode(struct buffer_t *);
void link(struct buffer_t *, struct node_t *);
struct node_t * unlink(struct buffer_t *);
void printBuffer(struct buffer_t *);
int isEmpty(struct buffer_t *);
int isFull(struct buffer_t *);
void freeBuffer(struct buffer_t *);

#endif