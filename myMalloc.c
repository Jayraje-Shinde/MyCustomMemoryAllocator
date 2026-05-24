/*
 *  Copyright 2026 Jayraje Shinde
 */

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Block {
  size_t size;
  bool isFree;
  struct Block *next;
  struct Block *prev;
} Block;

typedef struct GloabalHeader {
  void *heapStart;
  void *heapEnd;
  struct Block *startBlock;
  struct Block *tailBlock;
  int pages;
} GloabalHeader;

const int PAGE_SIZE = 4096;
static GloabalHeader *globHead = NULL;
const int blockSplitLimit = 256;

bool isSplitable(Block *bestBlock, size_t size) {
  size_t neededSpace = sizeof(Block) + size;
  size_t remainingSpace = bestBlock->size - neededSpace;

  return remainingSpace > blockSplitLimit;
}

void *SplitBlock(Block *blockToSplit, size_t size) {
  blockToSplit->isFree = false;
  int spaceRemainigAfterSplit = blockToSplit->size - size - sizeof(Block);
  blockToSplit->size = size;

  Block *newBlock = (void *)((char *)blockToSplit + sizeof(Block) + size);

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

  return (void *)((char *)blockToSplit + sizeof(Block));
}

void *myMalloc(size_t size) {
  int noOfPagesToGrow = ((size + sizeof(Block)) % PAGE_SIZE) == 0
                            ? ((size + sizeof(Block)) / PAGE_SIZE)
                            : 1 + ((size + sizeof(Block)) / PAGE_SIZE);
  if (globHead == NULL) {
    void *heapStart = sbrk(noOfPagesToGrow * PAGE_SIZE);
    globHead = (GloabalHeader *)heapStart;
    globHead->pages = noOfPagesToGrow;
    globHead->heapStart = heapStart;
    globHead->heapEnd =
        (void *)((char *)heapStart + (noOfPagesToGrow * PAGE_SIZE));
    Block *StartingBlock =
        (void *)((char *)globHead->heapStart + sizeof(GloabalHeader));
    assert(StartingBlock != NULL);
    globHead->startBlock = StartingBlock;
    StartingBlock->prev = NULL;
    StartingBlock->next = NULL;
    StartingBlock->isFree = false;
    StartingBlock->size = size;
    globHead->tailBlock = StartingBlock;
    return (void *)((char *)StartingBlock + sizeof(Block));
  }

  Block *tempblock =
      (Block *)((char *)globHead->heapStart + sizeof(GloabalHeader));

  Block *bestBlock = NULL;
  while (tempblock != NULL) {
    if ((tempblock->isFree == true) && (tempblock->size >= size) &&
        (bestBlock != NULL ? tempblock->size < bestBlock->size : true)) {
      bestBlock = tempblock;
    }
    tempblock = tempblock->next;
  }

  if (bestBlock != NULL && bestBlock->isFree != false) {
    bool canBlockSplit = isSplitable(bestBlock, size);
    if (canBlockSplit) {
      return SplitBlock(bestBlock, size);
    } else {
      bestBlock->isFree = false;
      return (void *)((char *)bestBlock + sizeof(Block));
    }
  }
  tempblock = globHead->tailBlock;
  void *currentBlockEnd =
      (void *)((char *)tempblock + tempblock->size + sizeof(Block));
  void *newBlockEnd = (void *)((char *)currentBlockEnd + sizeof(Block) + size);

  if ((char *)globHead->heapEnd < (char *)newBlockEnd) {
    sbrk(noOfPagesToGrow * PAGE_SIZE);
    globHead->heapEnd = sbrk(0);
    globHead->pages += noOfPagesToGrow;
  }

  Block *newBlock =
      (Block *)((char *)tempblock + tempblock->size + sizeof(Block));
  assert(newBlock != NULL);
  newBlock->prev = tempblock;
  newBlock->next = NULL;
  newBlock->isFree = false;
  newBlock->size = size;
  tempblock->next = newBlock;
  globHead->tailBlock = newBlock;
  return (void *)((char *)newBlock + sizeof(Block));
}

void myFree(void *toFree) {
  if (toFree == NULL || toFree < globHead->heapStart ||
      toFree > globHead->heapEnd)
    return;

  Block *toFreeBlock = (void *)((char *)toFree - sizeof(Block));

  if (toFreeBlock->isFree == true)
    return;

  if (toFreeBlock == NULL)
    return;

  toFreeBlock->isFree = true;
}

void printHeap() {
  printf("GloabalHeader [No of Pages : %d]\n", globHead->pages);

  Block *tempblock = globHead->startBlock;
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
  int *a = myMalloc(100);
  int *b = myMalloc(50);
  char *h = myMalloc(100000);
  myFree(b);
  printHeap();

  printf("\nCustom Malloc By Jayraje Shinde");
  return 0;
}

// Create block mering function and done
