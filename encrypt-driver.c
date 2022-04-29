#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "encrypt-module.h"
#include "circ-buffer.h"

#define NUM_THREADS 5
#define TRUE 1
#define FALSE 0

// Input and output buffer semaphores
sem_t in_lock, out_lock;
sem_t in_full, out_full;
sem_t in_empty, out_empty;
sem_t in_count, out_count;
sem_t in_ctd, out_ctd;

// Buffers
struct buffer_t *in_buff;
struct buffer_t *out_buff;

// Keep track of whether or not a reset has been requested
int reset_req = 0;

// Reset requested condition and mutex
pthread_cond_t reset_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t reset_mutex = PTHREAD_MUTEX_INITIALIZER;

// Reads one character at a time and places them in the input buffer
// Must call provided read_input()
// May need to block until other threads have consumed data from the input buffer
// Continues until EOF is reached
void *reader_th(void *param) {
    char c;
    while(TRUE) {
        if(reset_req)
            pthread_cond_wait(&reset_cond, &reset_mutex);
        else {
            sem_wait(&in_empty);
            sem_wait(&in_lock);
            c = read_input();
            addNode(in_buff, c);
            sem_post(&in_lock);
            sem_post(&in_full);
            sem_post(&in_count);
        }

        if(c == EOF)
            pthread_exit(NULL);
    }
}

// Counts occurences of each letter in the input file by looking at each character in the input buffer (READ ONLY)
// Must call provided count_input(int c)
// Will need to block if no characters in the input buffer
void *inCount_th(void *param) {
    struct node_t *curr;
    int counted;
    while(TRUE) {
        counted = 0;
        sem_wait(&in_count);
        sem_wait(&in_lock);
        curr = in_buff->head->next;
        while(curr != in_buff->tail) {
            if(curr->counted == 0) {
                if(curr->data != EOF)
                    count_input(curr->data);
                curr->counted = 1;
                counted++;
                if(curr->data == EOF) {
                    sem_post(&in_lock);
                    for(int i = 0; i < counted; i++)
                        sem_post(&in_ctd);
                    pthread_exit(NULL);
                }
            }
            curr = curr->next;
        }
        sem_post(&in_lock);
        for(int i = 0; i < counted; i++)
            sem_post(&in_ctd);
    }
    pthread_exit(NULL);
}

// Consumes one character at a time from input buffer, encrypts it, and places it in the output buffer
// Must call provided encrypt(int c)
// May need to wait for an item in the input buffer and for a slot to be available in the output buffer
// Slots in output buffer cannot be overwritten until writer thread and output counter have processed the char
// Continues until all characters of the input file have been encrypted
void *encrypt_th(void *param) {
    char c;
    while(TRUE) {
        sem_wait(&in_ctd);
        sem_wait(&in_full);
        sem_wait(&in_lock);
        c = remNode(in_buff);
        sem_post(&in_lock);
        sem_post(&in_empty);
        
        sem_wait(&out_empty);
        sem_wait(&out_lock);
        if(c == EOF) {
            addNode(out_buff, EOF);
            sem_post(&out_lock);
            sem_post(&out_full);
            sem_post(&out_count);
            pthread_exit(NULL);
        }
        else {
            char enc = encrypt(c);
            addNode(out_buff, enc);
        }
        sem_post(&out_lock);
        sem_post(&out_full);
        sem_post(&out_count);
    }
    pthread_exit(NULL);
}

// Counts occurences of each letter in the output file by looking at each character in output buffer
// Must call provided count_output(int c)
// Block if no characters in the output buffer
void *outCount_th(void *param) {
    struct node_t *curr;
    int counted;
    while(TRUE) {
        counted = 0;
        sem_wait(&out_count);
        sem_wait(&out_lock);
        curr = out_buff->head->next;
        while(curr != out_buff->tail) {
            if(curr->counted == 0){
                if(curr->data != EOF)
                    count_output(curr->data);
                curr->counted = 1;
                counted++;
                if(curr->data == EOF) {
                    sem_post(&out_lock);
                    for(int i = 0; i < counted; i++)
                        sem_post(&out_ctd);
                    pthread_exit(NULL);
                }
            }
            curr = curr->next;
        }
        sem_post(&out_lock);
        for(int i = 0; i < counted; i++)
            sem_post(&out_ctd);
    }
    pthread_exit(NULL);
}

// Writes encrypted characters in the output buffer to the output file
// Must call provided write_output(int c)
// May need to block until an encrypted character is available in the buffer
// Continues until it has written the last encrypted character
void *writer_th(void *param) {
    char c;
    while(TRUE) {
        sem_wait(&out_ctd);
        sem_wait(&out_full);
        sem_wait(&out_lock);
        c = remNode(out_buff);
        if(c == EOF) {
            sem_post(&out_lock);
            sem_post(&out_empty);
            pthread_exit(NULL);
        }
        else
            write_output(c);
        sem_post(&out_lock);
        sem_post(&out_empty);
    }
    pthread_exit(NULL);
}

// Consistency check
void reset_requested() {
    // Stop reader thread
    // Allow output thread to "catch-up" if needed
    reset_req = 1;
    int empty;
    while(1) {
        sem_wait(&in_lock);
        empty = isEmpty(in_buff);
        sem_post(&in_lock);
        if(empty)
            break;
    }
	log_counts();
    reset_finished();
}

// Called after consistency check when module requests a reset
void reset_finished() {
    reset_req = 0;
    pthread_cond_signal(&reset_cond);
}

int main(int argc, char **argv) {
    if(argc != 4) {
        printf("Usage: ./encrypt <input filename> <output filename> <log filename>\n");
        return 0;
    }

    //Required input, output, and log filenames from the user
    char *input = argv[1];
    char *output = argv[2];
    char *log = argv[3];

    init(input, output, log);

    //Prompt for input buffer size
    int n, m, valid = 0;

    while(!valid) {
        printf("Enter input buffer size: ");
        scanf("%d", &n);
        if(n <= 1)
            printf("Size must be > 1.\n");
        else
            valid = 1;
    }
    valid = 0;
    while(!valid) {
        printf("Enter output buffer size: ");
        scanf("%d", &m);
        if(m <= 1)
            printf("Size must be > 1.\n");
        else
            valid = 1;
    }

    in_buff = initBuffer(n);
    out_buff = initBuffer(m);

    // Initialize semaphores
    sem_init(&in_lock, 0, 1); // Input buffer lock
    sem_init(&out_lock, 0, 1); // Output buffer lock
    sem_init(&in_empty, 0, n); // Input buffer slots available
    sem_init(&out_empty, 0, m); // Output buffer slots avaiable
    sem_init(&in_full, 0, 0); // Input buffer slots taken
    sem_init(&out_full, 0, 0); // Output buffer slots taken
    sem_init(&in_count, 0, 0); // Input characters available to count
    sem_init(&out_count, 0, 0); // Output characters available to count
    sem_init(&in_ctd, 0, 0); // Input characters counted
    sem_init(&out_ctd, 0, 0); // Output characters counted
    
    // Create the 5 threads
    pthread_t threads[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++) {
        int status;
        switch(i) {
            case 0:
                status = pthread_create(&threads[i], NULL, reader_th, NULL);
                break;
            case 1:
                status = pthread_create(&threads[i], NULL, inCount_th, NULL);
                break;
            case 2:
                status = pthread_create(&threads[i], NULL, encrypt_th, NULL);
                break;
            case 3:
                status = pthread_create(&threads[i], NULL, outCount_th, NULL);
                break;
            case 4:
                status = pthread_create(&threads[i], NULL, writer_th, NULL);
                break;
            default:
                break;
        }
        if(status) {
            printf("Error creating thread: %d\n", status);
            freeBuffer(in_buff);
            freeBuffer(out_buff);
            exit(-1);
        }
    }

    // Wait for threads to finish
    for(int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    // Cleanup resources
    freeBuffer(in_buff);
    freeBuffer(out_buff);
    sem_destroy(&in_lock);
    sem_destroy(&out_lock);
    sem_destroy(&in_empty);
    sem_destroy(&out_empty);
    sem_destroy(&in_full);
    sem_destroy(&out_full);
    sem_destroy(&in_count);
    sem_destroy(&out_count);
    sem_destroy(&in_ctd);
    sem_destroy(&out_ctd);
    log_counts();
    printf("Encryption finished.\n");
    return 0;
}