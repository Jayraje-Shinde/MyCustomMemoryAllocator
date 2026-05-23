/*
 *  Copyright 2026 Jayraje Shinde
 */

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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

  while (tempblock != NULL) {
    if (tempblock->isFree == true && tempblock->size >= size) {
      tempblock->isFree = false;
      return (void *)((char *)tempblock + sizeof(Block));
    }

    if (tempblock->next == NULL)
      break;
    tempblock = tempblock->next;
  }

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

int main() {
  int *b = myMalloc(10000);
  int *a = myMalloc(100);
  printf("%p", a);
  printf("\nCustom Malloc By Jayraje Shinde");
  return 0;
}

// Todo update the first fit to best fit and after that also add block spliting
