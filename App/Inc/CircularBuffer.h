#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <stdint.h>

#define BUFFER_SIZE 10

typedef struct 
{
    char buffer[BUFFER_SIZE][24];
    uint8_t head;
    uint8_t tail;
} Circular_Buffer_char24_t;

typedef struct
{
    char id[24];
    int64_t parsedAt;
} IdFrame_t;

typedef struct
{
    IdFrame_t buffer[BUFFER_SIZE];
    uint8_t head;
    uint8_t tail; 
} Circular_Buffer_idFrame_t;


void buffer_put(Circular_Buffer_char24_t* buf, char* data);
void buffer_get(Circular_Buffer_char24_t* buf, char* data);
void buffer_put_idframe(Circular_Buffer_idFrame_t* buf, IdFrame_t* data);
void buffer_get_idframe(Circular_Buffer_idFrame_t* buf, IdFrame_t* data);
#endif