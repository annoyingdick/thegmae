#ifndef HeapQueue_h_
#define HeapQueue_h_

//min binary heap priority queue implementation

#define DECLARE_HEAPQUEUE_TYPEDEF(type) typedef struct { \
    float priority; type value; \
} type##HeapQueueNode; \
DECLARE_ARRAY_TYPEDEF(type##HeapQueueNode) typedef type##HeapQueueNodeArray type##HeapQueue;

#define HEAPQUEUE_PARENT(i) ((i - 1) / 2)
#define	SWAP(type, x, y) do { const type temp = x; x = y; y = temp; } while (0)
#define	HEAPQUEUE_IS_LOWER(x, y) \
heapq->elements[x].priority < heapq->elements[y].priority

#define DECLARE_HEAPQUEUE_IMPL(type) \
DECLARE_ARRAY_IMPL(type##HeapQueueNode) \
void type##HeapQueue_Init(type##HeapQueue* const heapq) { \
    type##HeapQueueNodeArray_Init(heapq); \
} \
bool type##HeapQueue_IsEmpty(const type##HeapQueue* heapq) { return !heapq->numElements; } \
void type##HeapQueue_InsertElement( \
    type##HeapQueue* const heapq, const float priority, const type* const value \
) { \
    ArrayIndex i; \
    i = type##HeapQueueNodeArray_AppendElement(heapq, &(type##HeapQueueNode){ \
	.priority = priority, .value = *value \
    }); \
    while (i && heapq->elements[HEAPQUEUE_PARENT(i)].priority > heapq->elements[i].priority) { \
	SWAP(type##HeapQueueNode, heapq->elements[HEAPQUEUE_PARENT(i)], heapq->elements[i]); \
	i = HEAPQUEUE_PARENT(i); \
    } \
} \
void type##HeapQueue_Heapify(type##HeapQueue* const heapq, const ArrayIndex i) { \
    const ArrayIndex l = (2 * i) + 1, r = (2 * i) + 2; \
    ArrayIndex smallest; \
    smallest = i; \
    if (l < heapq->numElements && HEAPQUEUE_IS_LOWER(l, smallest)) smallest = l; \
    if (r < heapq->numElements && HEAPQUEUE_IS_LOWER(r, smallest)) smallest = r; \
    if (smallest != i) { \
	SWAP(type##HeapQueueNode, heapq->elements[smallest], heapq->elements[i]); \
	type##HeapQueue_Heapify(heapq, smallest); \
    } \
} \
void type##HeapQueue_PopElement(type##HeapQueue* const heapq, type* const dest) { \
    *dest = heapq->elements[0].value; \
    heapq->elements[0] = *type##HeapQueueNodeArray_GetLastElement(heapq); \
    heapq->numElements--; \
    type##HeapQueue_Heapify(heapq, 0); \
} \
void type##HeapQueue_Destroy(const type##HeapQueue* const heapq) { \
    type##HeapQueueNodeArray_Destroy(heapq); \
}

#define DECLARE_HEAPQUEUE(type) \
DECLARE_ARRAY(type##HeapQueueNode) \
void type##HeapQueue_Init(type##HeapQueue* heapq); \
bool type##HeapQueue_IsEmpty(const type##HeapQueue* heapq); \
void type##HeapQueue_InsertElement(type##HeapQueue* heapq, float priority, const type* value); \
void type##HeapQueue_Heapify(type##HeapQueue* heapq, ArrayIndex i); \
void type##HeapQueue_PopElement(type##HeapQueue* heapq, type* dest); \
void type##HeapQueue_Destroy(const type##HeapQueue* heapq);

#endif
