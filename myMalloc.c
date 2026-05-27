/*
 *  Copyright 2026 Jayraje Shinde
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//	Below is the Data structure for the memory block and the heap manager.
// The heap manager will keep track of the start and end of the heap, 
//	the number of pages allocated, and the linked list of memory blocks. 
//	Each memory block will have its size, a flag to indicate if it's free or not, 
//	and pointers to the next and previous blocks in the list. 

typedef struct MemoryBlock {
  size_t size;
  bool isFree;
  struct MemoryBlock *next;
  struct MemoryBlock *prev;
} MemoryBlock;

typedef struct HeapManager {
  void *heapStart;
  void *heapEnd;
  struct MemoryBlock *startBlock;
  struct MemoryBlock *tailBlock;
  int pages;
} HeapManager;


const int PAGE_SIZE = 4096; //This constant defines the size of memory page which is 4096 byte (4KB).
//Now generally there are rarly diffrent page sizes for example we can see 16KB page size in IOS.


// Static keyword below helps us to maintain the state of our global header metadata variable to live
// until the program ends and also keeps it private that is it can only be accessed within this file.
static HeapManager *globHead = NULL; 


const int blockSplitLimit = 256; // This constant defines the minimum size available for a block that can be split.


//Below is the function that checks if a given memory block is able to split by check the size of the block
//which is given input to the function with the size we want to allocate and the size of the metadata for 
//the new block that will be created after split if the remaining space is greater than the blockSplitLimit 
//then we can split the block and use the remaining space for future allocations.
bool isSplitable(MemoryBlock *bestFitBlock, size_t size) {
  size_t neededSpace = sizeof(MemoryBlock) + size;
  size_t remainingSpace = bestFitBlock->size - neededSpace;

  return remainingSpace > blockSplitLimit;
}


// Now once we have for the condition of spliting the block , below is the function that will perform the actual splitting
// of the block. It will mark the original block as used becuase we will be giving it to the user after succesfully 
//updating the size meta data variable of the block the remaining space will be given to a new block that w ewill create 
//and hence we get a new block 
// example we need 100 bytes and the block we got is 500 bytes so this block is eligible for splitting
//so we will split this block into 100 bytes block and the remainig 400 will be used by the blockmetadata + actual free space.
void *SplitMemoryBlock(MemoryBlock *blockToSplit, size_t size) {
  blockToSplit->isFree = false;
  int spaceRemainigAfterSplit = blockToSplit->size - size - sizeof(MemoryBlock);
  blockToSplit->size = size;

  MemoryBlock *newBlock =
      (void *)((char *)blockToSplit + sizeof(MemoryBlock) + size);

  if (globHead->tailBlock == blockToSplit) {
    globHead->tailBlock = newBlock;
    newBlock->next = NULL;
  } else {
    newBlock->next = blockToSplit->next;
    blockToSplit->next->prev = newBlock;
  }
  newBlock->prev = blockToSplit;
  blockToSplit->next = newBlock;
  newBlock->size = spaceRemainigAfterSplit;
  newBlock->isFree = true;

  return (void *)((char *)blockToSplit + sizeof(MemoryBlock));
}


//Now finally one of the core function our memory allocator 
//This is not at all even close to the real malloc implementation of the C library
// but this function got everything that a core allocator should have 
//Now coming to the implementation of this function :
// It checks for pages needed that is how much size is needed to be allocated
// once we have that we check if out global header is initialised or not if not then we create our first block
// once out global header is set after that whenever we need to allocate memory we travese through our blocks
// starting from starting block and after that we check for the best fit block that is the block that is enough as well as 
//smallest among eligible blocks if we find such block we check for spliting if possible split and return the address 
//if we cant split we return that whole block to the user and mark it as used
// finally if our global header is initialised that is first block exists and we dont have any eligible block 
//then we create a new block at the end of the heap and return that to user and update our global header tail hence having the integrity of the list 
//Note : our MemoryAllocator return user the address just after the block metadata is over
void *AllocateMemory(size_t size) {
  int pagesNeeded = ((size + sizeof(MemoryBlock)) % PAGE_SIZE) == 0
                        ? ((size + sizeof(MemoryBlock)) / PAGE_SIZE)
                        : 1 + ((size + sizeof(MemoryBlock)) / PAGE_SIZE);
  if (globHead == NULL) {
    void *heapStart = sbrk(pagesNeeded * PAGE_SIZE);
    globHead = (HeapManager *)heapStart;
    globHead->pages = pagesNeeded;
    globHead->heapStart = heapStart;
    globHead->heapEnd = (void *)((char *)heapStart + (pagesNeeded * PAGE_SIZE));
    MemoryBlock *StartingBlock =
        (void *)((char *)globHead->heapStart + sizeof(HeapManager));
    assert(StartingBlock != NULL);
    globHead->startBlock = StartingBlock;
    StartingBlock->prev = NULL;
    StartingBlock->next = NULL;
    StartingBlock->isFree = false;
    StartingBlock->size = size;
    globHead->tailBlock = StartingBlock;
    return (void *)((char *)StartingBlock + sizeof(MemoryBlock));
  }

  MemoryBlock *tempblock =
      (MemoryBlock *)((char *)globHead->heapStart + sizeof(HeapManager));

  MemoryBlock *bestFitBlock = NULL;
  while (tempblock != NULL) {
    if ((tempblock->isFree == true) && (tempblock->size >= size) &&
        (bestFitBlock != NULL ? tempblock->size < bestFitBlock->size : true)) {
      bestFitBlock = tempblock;
    }
    tempblock = tempblock->next;
  }

  if (bestFitBlock != NULL && bestFitBlock->isFree != false) {
    bool canBlockSplit = isSplitable(bestFitBlock, size);
    if (canBlockSplit) {
      return SplitMemoryBlock(bestFitBlock, size);
    } else {
      bestFitBlock->isFree = false;
      return (void *)((char *)bestFitBlock + sizeof(MemoryBlock));
    }
  }
  tempblock = globHead->tailBlock;
  void *currentBlockEnd =
      (void *)((char *)tempblock + tempblock->size + sizeof(MemoryBlock));
  void *newBlockEnd =
      (void *)((char *)currentBlockEnd + sizeof(MemoryBlock) + size);

  if ((char *)globHead->heapEnd < (char *)newBlockEnd) {
    sbrk(pagesNeeded * PAGE_SIZE);
    globHead->heapEnd = sbrk(0);
    globHead->pages += pagesNeeded;
  }

  MemoryBlock *newBlock = (MemoryBlock *)((char *)tempblock + tempblock->size +
                                          sizeof(MemoryBlock));
  assert(newBlock != NULL);
  newBlock->prev = tempblock;
  newBlock->next = NULL;
  newBlock->isFree = false;
  newBlock->size = size;
  tempblock->next = newBlock;
  globHead->tailBlock = newBlock;
  return (void *)((char *)newBlock + sizeof(MemoryBlock));
}


//Below is a healper function that helps us identify if adjacent blocks to the block we want to free are free or not this is important because if they are free then we can merge them together and create a bigger block which will 
//help us to reduce fragmentation in our heap and also will help us to reuse the memory more efficiently.
bool CanCoalease(MemoryBlock *BlockToCheck) {
  if (BlockToCheck->next != NULL && BlockToCheck->next->isFree == true)
    return true;

  if (BlockToCheck->prev != NULL && BlockToCheck->prev->isFree == true)
    return true;

  return false;
}


//Once our CanCoalease function identifies that we can merge the adjacent blocks then this function will perform the actual merging of the blocks it will keep merging the adjacent free blocks until we hit a used block and
//hence we will get a bigger block with more free space which can be used for future allocations.
//How the coalease function works is simple we first marks our block free then we update the block to the left most free block that is the block that is previos and is free 
//once we get to the previous most free block we then loop forward and merge block removing the meta data and adding size + sizeof(block) to our block creating bigger block
//Edge cases in this are the head block and tail block handled carefully
void CoaleaseAdjacentBlocks(MemoryBlock *blockToMerge) {
  blockToMerge->isFree = true;
  while (blockToMerge != globHead->startBlock &&
         blockToMerge->prev->isFree == true) {
    // this loop will bring us to the prev block which is free until we hit a
    // block that is USED
    blockToMerge = blockToMerge->prev;
  }
  while (blockToMerge->next != NULL && blockToMerge->next->isFree == true) {
    MemoryBlock *toRemove = blockToMerge->next;

    blockToMerge->size += sizeof(MemoryBlock) + toRemove->size;

    if (toRemove == globHead->tailBlock) {
      globHead->tailBlock = blockToMerge;
      blockToMerge->next = NULL;
      break;
    }

    blockToMerge->next = toRemove->next;
    if (toRemove->next != NULL) {
      toRemove->next->prev = blockToMerge;
    }
  }
}


//Now the next main core function of memory allocator which is Freeing the memory using freememory In c library we know this by the name of free()
//Now how this works so firsly this checks if the address is valid in our heap boundary and reveser calculates the adress as our memory allocator doesnt return block start adress
//it returns the address just after the block metadata so we need to reverse calculate the block start address by subtracting the size of the 
//metadata from the given address once we have that we check if this block is already free
//or not if its already free then we just return as this is a double free case and we dont want 
//to mess with our heap in that case
//Now after all the edge cases and safety cases we simple check for coaleasing if possible as we saw in above
//function then we can easily proceed to coalease using the function or else if not possible we simply mark the block as free
void FreeMemory(void *toFree) {
  if (toFree == NULL || toFree < globHead->heapStart ||
      toFree > globHead->heapEnd)
    return;

  MemoryBlock *toFreeBlock = (void *)((char *)toFree - sizeof(MemoryBlock));

  if (toFreeBlock == NULL)
	 return;
	 
  if (toFreeBlock->isFree == true)
    return;


  if (CanCoalease(toFreeBlock)) {
    CoaleaseAdjacentBlocks(toFreeBlock);
  } else {
    toFreeBlock->isFree = true;
  }
}



void printHeap() { // as the name suggest this function prints the blocks and there metadata and addresses for debugging purpose
  printf("HeapManager [No of Pages : %d]\n", globHead->pages);

  MemoryBlock *tempblock = globHead->startBlock;
  int count = 1;
  while (tempblock != NULL) {
    if (tempblock->isFree == true) {
      printf("[Prev : %p] -> [Block %d : %p] -> [FREE] - [size : %zu] -> "
             "[Next : %p]\n",
             tempblock->prev, count++, tempblock, tempblock->size,
             tempblock->next);
    } else {
      printf("[Prev : %p] -> [Block %d : %p] -> [USED] - [size : %zu] -> "
             "[Next : %p]\n",
             tempblock->prev, count++, tempblock, tempblock->size,
             tempblock->next);
    }

    tempblock = tempblock->next;
  }
}


int main() {

  printf("=========== CUSTOM MALLOC TEST SUITE ===========\n\n");

  // -------------------------------------------------
  // TEST 1 : Basic Allocations
  // -------------------------------------------------
  printf("TEST 1 : Basic Allocations\n");

  int *a = AllocateMemory(100);
  int *b = AllocateMemory(50);
  int *c = AllocateMemory(80);

  printHeap();

  // Expected:
  // USED -> USED -> USED

  printf("\n===============================================\n\n");

  // -------------------------------------------------
  // TEST 2 : Free Middle Block
  // -------------------------------------------------
  printf("TEST 2 : Free Middle Block\n");

  FreeMemory(b);

  printHeap();

  // Expected:
  // USED -> FREE -> USED

  printf("\n===============================================\n\n");

  // -------------------------------------------------
  // TEST 3 : Adjacent Coalescing
  // -------------------------------------------------
  printf("TEST 3 : Adjacent Coalescing\n");

  FreeMemory(c);

  printHeap();

  // Expected:
  // USED -> FREE (merged b + c)

  printf("\n===============================================\n\n");

  // -------------------------------------------------
  // TEST 4 : Head Coalescing
  // -------------------------------------------------
  printf("TEST 4 : Head Coalescing\n");

  FreeMemory(a);

  printHeap();

  // Expected:
  // Single FREE block

  printf("\n===============================================\n\n");

  // -------------------------------------------------
  // TEST 5 : Split Reuse
  // -------------------------------------------------
  printf("TEST 5 : Split Reuse\n");

  int *d = AllocateMemory(120);

  printHeap();

  // Expected:
  // USED block of 120
  // Remaining FREE split block

  printf("\n===============================================\n\n");

  // -------------------------------------------------
  // TEST 6 : New Allocation
  // -------------------------------------------------
  printf("TEST 6 : Additional Allocations\n");

  int *e = AllocateMemory(60);
  int *f = AllocateMemory(40);

  printHeap();

  printf("\n===============================================\n\n");

  // -------------------------------------------------
  // TEST 7 : Full Heap Coalescing
  // -------------------------------------------------
  printf("TEST 7 : Full Heap Coalescing\n");

  FreeMemory(d);
  FreeMemory(e);
  FreeMemory(f);

  printHeap();

  // Expected:
  // Entire heap merged into one FREE block

  printf("\n===============================================\n\n");

  printf("Custom Malloc By Jayraje Shinde\n");

  return 0;
}
