#ifndef TCLH_LIFO_H
#define TCLH_LIFO_H

/*
 * Copyright (c) 2010-2023, Ashok P. Nadkarni
 * All rights reserved.
 *
 * See the file LICENSE for license
 */

#include <stdint.h>
#include "tclhBase.h"

#define TCLH_LIFO_EXTERN

typedef size_t Tclh_LifoUSizeT;
#define TCLH_LIFO_SIZE_MODIFIER "z"

typedef struct Tclh_Lifo Tclh_Lifo;
typedef struct Tclh_LifoMarkInfo Tclh_LifoMarkInfo;
typedef Tclh_LifoMarkInfo *Tclh_LifoMark;

typedef void *Tclh_LifoAllocFn(size_t sz);
typedef void Tclh_LifoFreeFn(void *p);

struct Tclh_Lifo {
    Tclh_LifoMark lifo_top_mark; /* Topmost mark */
    Tclh_LifoMark lifo_bot_mark; /* Bottommost mark */
    Tclh_LifoUSizeT lifo_chunk_size;   /* Size of each chunk to allocate.
                                  Note this size might not be a multiple
                                  of the alignment size */
    Tclh_LifoAllocFn *lifo_allocFn;
    Tclh_LifoFreeFn *lifo_freeFn;
#define TCLH_LIFO_PANIC_ON_FAIL 0x1
    int32_t lifo_magic; /* Only used in debug mode */
#define TCLH_LIFO_MAGIC 0xb92c610a
    int lifo_flags;
};

/* Error codes */
#define TCLH_LIFO_E_SUCCESS  0
#define TCLH_LIFO_E_NOMEMORY 1
#define TCLH_LIFO_E_INVALID_PARAM 2

/* Function: Tclh_LifoInit
 * Create a Last-In-First-Out memory pool
 *
 * Parameters: 
 * lifoP - Memory pool to initialize
 * allocFunc - function to us to allocate memory. Must be callable at any
 *             time, including at initialization time and must return
 *             memory aligned appropriately for the platform. A default
 *             allocator is used if NULL.
 * freeFunc - function to free memory allocated by allocFunc. Can be NULL
 *            iff allocFunc is NULL.
 * chunkSz - Default size of usable space when allocating a chunk (does
 *           not include the chunk header). This is only a hint and may be
 *           adjusted to a minimum or maximum size.
 * flags - See TCLH_LIFO_F_* definitions
 * 
 * Initializes a memory pool from which memory can be allocated in
 * Last-In-First-Out fashion.
 *
 * Returns:
 * Returns TCLH_LIFO_E_SUCCESS or a TCLH_LIFO_E_* error code.
 */
TCLH_LIFO_EXTERN int Tclh_LifoInit(Tclh_Lifo *lifoP,
                               Tclh_LifoAllocFn *allocFunc,
                               Tclh_LifoFreeFn *freeFunc,
                               Tclh_LifoUSizeT chunkSz,
                               int flags);

/* Function: Tclh_LifoClose
 * Frees up resources associated with a LIFO memory pool.
 *
 * Parameters:
 * lifoP - Memory pool to free up
 * 
 * Frees up various resources allocated for a LIFO memory pool. The pool
 * must not be used after the function is called. Note the function frees
 * the resources in use by the memory pool, not lifoP itself which is
 * owned by the caller.
*/
TCLH_LIFO_EXTERN void Tclh_LifoClose(Tclh_Lifo  *lifoP);

/* Function: Tclh_LifoAllocMin
 * Allocate a minimum amount of memory from LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 * minNumBytes - minimum number of bytes to allocate
 * allocatedP - location to store the actual number allocated. On
 *     success, this will be at least numBytes.
 *
 * Allocates memory from a LIFO memory pool and returns a pointer to it. If
 * allocatedP is not NULL and the current chunk has more than minNumBytes free,
 * all of it will be allocated.
 *
 * Returns:
 * Returns pointer to allocated memory on success. On failure returns 
 * a NULL pointer unless TCLH_LIFO_PANIC_ON_FAIL is set for the pool,
 * in which case it panics.
 */
TCLH_LIFO_EXTERN void *Tclh_LifoAllocMin(
    Tclh_Lifo *lifoP,
    Tclh_LifoUSizeT minNumBytes,
    Tclh_LifoUSizeT *allocatedP
);

/* Function: Tclh_LifoAlloc
 * Allocate memory from LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 * numBytes - number of bytes to allocate
 *
 * Allocates memory from a LIFO memory pool and returns a pointer to it.
 *
 * Returns:
 * Returns pointer to allocated memory on success. On failure returns 
 * a NULL pointer unless TCLH_LIFO_PANIC_ON_FAIL is set for the pool,
 * in which case it panics.
 */
TCLH_INLINE void* Tclh_LifoAlloc(Tclh_Lifo *l, Tclh_LifoUSizeT numBytes) {
    return Tclh_LifoAllocMin(l, numBytes, NULL);
}

/* Function: Tclh_LifoPushFrameMin
 * Allocate a software stack frame in a LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 * minNumBytes - minimum number of bytes to allocate in the frame
 * allocatedP - location to store the actual number allocated. On
 *     success, this will be at least numBytes.
 * 
 * Allocates space in a LIFO memory pool being used as a software stack.
 * This provides a means of maintaining a software stack for temporary structures
 * that are too large to be allocated on the hardware stack.
 *
 * Both Tclh_LifoMark and Tclh_LifoPushFrame may be used on the same memory pool. The
 * latter function in effect creates a anonymous mark that is maintained
 * internally and which may be released (along with the associated user memory)
 * through the function Tclh_LifoPopFrame which releases the last allocated mark,
 * irrespective of whether it was allocated through Tclh_LifoMark or
 * Tclh_LifoPushFrame. Alternatively, the mark and associated memory are also
 * freed when a previosly allocated mark is released.
 *
 * If allocatedP is not NULL, as much memory (at least minNumBytes) will be allocated
 * as possible from the current chunk.
 *
 * Returns:
 * Returns pointer to allocated memory on success.
 * If memory cannot be allocated, returns NULL unless TCLH_LIFO_PANIC_ON_FAIL
 * is set for the pool, in which case it panics.
*/
TCLH_LIFO_EXTERN void *Tclh_LifoPushFrameMin(Tclh_Lifo *lifoP,
                                         Tclh_LifoUSizeT minNumBytes,
                                         Tclh_LifoUSizeT *allocatedP);

/* Function: Tclh_LifoPushFrame
 * Allocate a software stack frame in a LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 * numBytes - number of bytes to allocate in the frame
 * 
 * Allocates space in a LIFO memory pool being used as a software stack.
 * This provides a means of maintaining a software stack for temporary structures
 * that are too large to be allocated on the hardware stack.
 *
 * Both Tclh_LifoMark and Tclh_LifoPushFrame may be used on the same memory pool. The
 * latter function in effect creates a anonymous mark that is maintained
 * internally and which may be released (along with the associated user memory)
 * through the function Tclh_LifoPopFrame which releases the last allocated mark,
 * irrespective of whether it was allocated through Tclh_LifoMark or
 * Tclh_LifoPushFrame. Alternatively, the mark and associated memory are also
 * freed when a previosly allocated mark is released.
 *
 * Returns:
 * Returns pointer to allocated memory on success.
 * If memory cannot be allocated, returns NULL unless TCLH_LIFO_PANIC_ON_FAIL
 * is set for the pool, in which case it panics.
 */
TCLH_INLINE void *
Tclh_LifoPushFrame(Tclh_Lifo *lifoP, Tclh_LifoUSizeT numBytes)
{
    return Tclh_LifoPushFrameMin(lifoP, numBytes, NULL);
}

/* Function: Tclh_LifoPushMark
 * Mark current state of a LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 *
 * Stores the current state of a LIFO memory pool on a stack. The state can be
 * restored later by calling Tclh_LifoPopMark. Any number of marks may be
 * created but they must be popped in reverse order. However, not all marks
 * need be popped since popping a mark automatically pops all marks created
 * after it.
 *
 * Returns:
 * On success, returns a handle for the mark.
 * On failure, returns NULL unless the TCLH_LIFO_PANIC_ON_FAIL flag
 * is set for the pool, in which case the function panics.
 */
TCLH_LIFO_EXTERN Tclh_LifoMark Tclh_LifoPushMark(Tclh_Lifo *lifoP);

/* Function: Tclh_LifoPopMark
 * Restore state of a LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 * 
 * Restores the state of a LIFO memory pool that was previously saved
 * using Tclh_LifoPushMark or Tclh_LifoPushFrame.  Memory allocated
 * from the pool between the push and this pop is freed up. Caller must
 * not subsequently call this routine with marks created between the
 * Tclh_LifoMark and this Tclh_LifoPopMark as they will have been
 * freed as well. The mark being passed to this routine is also freed
 * and hence must not be referenced afterward.
*/
TCLH_LIFO_EXTERN void Tclh_LifoPopMark(Tclh_LifoMark mark);

/* Function: Tclh_LifoPopFrame
 * Release the topmost mark from the memory pool.
 *
 * Parameters:
 * lifoP - memory pool
 *
 * Releases the topmost (last allocated) mark from a Tclh_Lifo_t. The
 * mark may have been allocated using either Tclh_LifoMark or
 * Tclh_LifoPushFrame.
 */
TCLH_INLINE void Tclh_LifoPopFrame(Tclh_Lifo *lifoP) {
    Tclh_LifoPopMark(lifoP->lifo_top_mark);
}

/* Function: Tclh_LifoExpandLast
 * Expand the last memory block allocated from a LIFO memory pool
 *
 * Parameters:
 * lifoP - memory pool
 * incr - amount by which to expand the last allocated block
 * dontMove - if non-0, the expansion must happen in place, else the block
 *    may be moved in memory.
 * 
 * Expands the last memory block allocated from a LIFO memory pool. If no
 * memory was allocated in the pool since the last mark, just allocates
 * a new block of size incr.
 *
 * The function may move the block if necessary unless the dontMove parameter
 * is non-0 in which case the function will only attempt to expand the block in
 * place. When dontMove is 0, caller must be careful to update pointers that
 * point into the original block since it may have been moved on return.
 *
 * On success, the size of the block is guaranteed to be increased by at
 * least the requested increment. The function may fail if the last
 * allocated block cannot be expanded without moving and dontMove is set,
 * if it is not the last allocation from the LIFO memory pool, or if a mark
 * has been allocated after this block, or if there is insufficient memory.
 *
 * Returns:
 * On success, returns a pointer to the block position.
 * On failure, the function return a NULL pointer.
 * Note the function does not panic even if TCLH_LIFO_PANIC_ON_FAIL
 * is set for the pool.
 */
TCLH_LIFO_EXTERN void *
Tclh_LifoExpandLast(Tclh_Lifo *lifoP, Tclh_LifoUSizeT incr, int dontMove);

/* Function: Tclh_LifoShrinkLast
 * Shrink the last memory block allocated from a LIFO memory pool
 * 
 * Parameters:
 * lifoP - memory pool
 * decr - amount by which to shrink the last allocated block
 * dontMove - if non-0, the shrinkage must happen in place, else the block
 *    may be moved in memory.
 * Shrinks the last memory block allocated from a LIFO memory pool. No marks
 * must have been allocated in the pool since the last memory allocation else
 * the function will fail.
 *
 * The function may move the block to compact memory unless the dontMove
 * parameter is non-0. When dontMove is 0, caller must be careful to update
 * pointers that point into the original block since it may have been moved
 * on return.
 *
 * On success, the size of the block is not guaranteed to have been decreased.
 *
 * Returns:
 * On success, returns a pointer to the block position.
 * On failure, the function return a NULL pointer. The original allocation
 * is still valid in this case.
 * Note the function does not panic even if TCLH_LIFO_PANIC_ON_FAIL
 * is set for the pool.
*/
TCLH_LIFO_EXTERN void *
Tclh_LifoShrinkLast(Tclh_Lifo *lifoP, Tclh_LifoUSizeT decr, int dontMove);

/* Function:
 * Resize the last memory block allocated from a LIFO memory pool
 * 
 * Parameters:
 * lifoP - memory pool
 * newSz - new size of the block
 * dontMove - if non-0, the resize must happen in place, else the block
 *    may be moved in memory.
 *
 * Resizes the last memory block allocated from a LIFO memory pool.  No marks
 * must have been allocated in the pool since the last memory allocation else
 * the function will fail.  The block may be moved if necessary unless the
 * dontMove parameter is non-0 in which the function will only attempt to resiz
 * e the block in place. When dontMove is 0, caller must be careful to update
 * pointers that point into the original block since it may have been moved on
 * return.

 * On success, the block is guaranteed to be at least as large as the requested
 * size. The function may fail if the block cannot be resized without moving
 * and dontMove is set, or if there is insufficient memory.
 *
 * Returns:
 * Returns pointer to new block position on success, else a NULL pointer.
 * Note the function does not panic even if TCLH_LIFO_PANIC_ON_FAIL
 * is set for the pool.
*/
TCLH_LIFO_EXTERN void *
Tclh_LifoResizeLast(Tclh_Lifo *lifoP, Tclh_LifoUSizeT newSz, int dontMove);

TCLH_LIFO_EXTERN int Tclh_LifoValidate(Tclh_Lifo *l);

#ifdef TCLH_IMPL
#include "tclhLifoImpl.c"
#endif

#endif
