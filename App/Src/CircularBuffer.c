#include "CircularBuffer.h"
#include <string.h>
#include "esp_log.h"


void buffer_put(Circular_Buffer_char24_t* buf, char* data)
{
    if (buf->head + 1 == buf->tail || (buf->tail == 0 && (buf->head == BUFFER_SIZE - 1)))
    {
        // buffer full
    }
    else
    {
        memset(&(buf->buffer[buf->head]), 0, 24);
        memcpy(&(buf->buffer[buf->head]), data, 24);
        if (buf->head == BUFFER_SIZE - 1)
        {
            buf->head = 0;
        }
        else
        {
            buf->head++;
        }
    }
}

void buffer_get(Circular_Buffer_char24_t* buf, char* data)
{
    if(buf->tail == buf->head)
    {
        // buffer empty
    }
    else
    {
        memcpy(data, &(buf->buffer[buf->tail]), 24);
        if (buf->tail == BUFFER_SIZE - 1)
        {
            buf->tail = 0;
        }
        else
        {
            buf->tail++;
        }
    }
}

void buffer_put_idframe(Circular_Buffer_idFrame_t* buf, IdFrame_t* data)
{
    if (buf->head + 1 == buf->tail || (buf->tail == 0 && (buf->head == BUFFER_SIZE - 1)))
    {
        // buffer full
    }
    else
    {
        memset(&(buf->buffer[buf->head]), 0, sizeof(IdFrame_t));
        memcpy(&(buf->buffer[buf->head]), data, sizeof(IdFrame_t));
        if (buf->head == BUFFER_SIZE - 1)
        {
            buf->head = 0;
        }
        else
        {
            buf->head++;
        }
    }
}

void buffer_get_idframe(Circular_Buffer_idFrame_t* buf, IdFrame_t* data)
{
    if(buf->tail == buf->head)
    {
        // buffer empty
    }
    else
    {
        memcpy(data, &(buf->buffer[buf->tail]), sizeof(IdFrame_t));
        if (buf->tail == BUFFER_SIZE - 1)
        {
            buf->tail = 0;
        }
        else
        {
            buf->tail++;
        }
    }
}