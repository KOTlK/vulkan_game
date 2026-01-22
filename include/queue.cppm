module;

#include "basic.h"
#include "allocator.h"
#include "assert.h"

export module queue;

#define QUEUE_INITIAL_LENGTH 256
#define QUEUE_REALLOC_STEP   128

#define QUEUE_TEMPLATE export template <typename T>

QUEUE_TEMPLATE
struct QueueIterator {
    T* data;
    u32 current;
    u32 length;

    QueueIterator(T* data, u32 current, u32 length) :
        data(data), current(current % length), length(length) {
    }
    
    QueueIterator& operator++() {
        current = (current + 1) % length;
        return *this;
    }

    bool operator!=(const QueueIterator& other) const {
        return current != other.current;
    }

    T operator*() const {
        return data[current];
    }
};

QUEUE_TEMPLATE
struct Queue {
    T*         data;
    Allocator* allocator;
    u32        count;
    u32        length;
    u32        head;
    u32        tail;

    Queue() : data(NULL), allocator(NULL), count(0), length(0), head(0), tail(0){};
    ~Queue() = default;

    QueueIterator<T> begin() {
        return QueueIterator(data, head, length);
    }

    QueueIterator<T> end() {
        return QueueIterator(data, tail, length);
    }
};


QUEUE_TEMPLATE
inline
void
queue_make(Queue<T>* queue, u32 length = QUEUE_INITIAL_LENGTH, Allocator* allocator = Allocator_Persistent) {
    auto data = AllocatorAlloc(T, allocator, sizeof(T) * length);
    Assert(data, "Cannot allocate memory for queue data");

    queue->data      = data;
    queue->count     = 0;
    queue->length    = length;
    queue->head      = 0;
    queue->tail      = 0;
    queue->allocator = allocator;
}

QUEUE_TEMPLATE
inline
Queue<T>
queue_make(u32 length = QUEUE_INITIAL_LENGTH, Allocator* allocator = Allocator_Persistent) {
    Queue<T> queue{};
    
    auto data = AllocatorAlloc(T, allocator, sizeof(T) * length);
    Assert(data, "Cannot allocate memory for queue data");

    queue.data      = data;
    queue.count     = 0;
    queue.length    = length;
    queue.head      = 0;
    queue.tail      = 0;
    queue.allocator = allocator;

    return queue;
}

QUEUE_TEMPLATE
inline
void
queue_realloc(Queue<T>* queue, u32 length) {
    Assert(queue->data, "Cannot resize uninitialized queue, initialize it with queue_make.");
    Assert(length > queue->length, "Cannot resize queue with less size.");
    if (queue->allocator == Allocator_Temp) {
        T* new_data = AllocatorAlloc(T, queue->allocator, sizeof(T) * length);
        Assert(new_data, "Cannot allocate enough memory for new queue");

        u32 head = queue->head;
        u32 tail = queue->tail;

        queue->head = 0;
        queue->tail = 0;

        while (head != tail) {
            new_data[queue->tail++] = queue->data[head];
            head = (head + 1) % queue->length;
        }

        queue->data = new_data;
    } else {
        T* origin = queue->data;
        queue->data = (T*)queue->allocator->realloc(queue->data, sizeof(T) * length);
        Assert(queue->data, "Cannot allocate enough memory for new queue");

        if (queue->head > queue->tail || queue->head == queue->tail) {
            u32 start = queue->length;
            u32 end   = queue->tail;

            for (u32 i = 0; i < end; i++) {
                u32 index = (start + i) % length;
                queue->data[index] = origin[i];
            }

            queue->tail = (start + end) % length;
        }
    }

    queue->length = length;
}

QUEUE_TEMPLATE
inline
void
queue_free(Queue<T>* queue) {
    Assert(queue->data, "Cannot free uninitialized queue, initialize it with queue_make.");
    if (queue->allocator == Allocator_Temp) return;

    AllocatorFree(queue->allocator, queue->data);
}

QUEUE_TEMPLATE
inline
void
queue_enqueue(Queue<T>* queue, T elem) {
    Assert(queue->data, "Cannot enqueue to uninitialized queue, initialize it with queue_make.");
    if (queue->count >= queue->length)
        queue_realloc(queue, queue->count + 1 + QUEUE_REALLOC_STEP);

    u32 index   = queue->tail;
    queue->tail = (queue->tail + 1) % queue->length;
    queue->count++;
    queue->data[index] = elem;
}

QUEUE_TEMPLATE
inline
T
queue_dequeue(Queue<T>* queue) {
    Assert(queue->data, "Cannot dequeue from uninitialized queue, initialize it with queue_make.");
    Assert(queue->count > 0, "Cannot dequeu if queue is empty.");
    T elem = queue->data[queue->head];
    queue->count--;
    queue->head = (queue->head + 1) % queue->length;

    if (queue->count == 0) {
        queue->head = 0;
        queue->tail = 0;
    }

    return elem;
}

QUEUE_TEMPLATE
inline
void
queue_clear(Queue<T>* queue) {
    Assert(queue->data, "Cannot clear uninitialized queue, initialize it with queue_make.");
    queue->head  = 0;
    queue->tail  = 0;
    queue->count = 0;
}

QUEUE_TEMPLATE
inline
bool
queue_contains(Queue<T>* queue, T elem) {
    Assert(queue->data, "Cannot search in uninitialized queue, initialize it with queue_make.");
    u32 counter = queue->head;

    while(counter != queue->tail) {
        if (queue->data[counter] == elem) return true;

        counter = (counter + 1) % queue->length;
    }

    return false;
}