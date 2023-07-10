#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mymalloc.h"

//Struct linked list of explict 
typedef struct freeNode{
    struct freeNode *nextNode;
    struct freeNode *prevNode;
}freeNode;

typedef struct blockSize{
    size_t size;
}blockSize;

//Global Variables 
char* mymalloc_arr;
int fit; 
freeNode *root;
freeNode *end; 
freeNode *next; 

size_t getSizeofBlock(void* ptr){
    blockSize* size = (void*)ptr - sizeof(struct blockSize);
    return size->size;
}

void printFreelist(){
    freeNode *current_node = root->nextNode;
   	while ( current_node != NULL) {
        printf("%ld\n", getSizeofBlock(current_node));
        printf("Current Node: %p\n", current_node);
        printf("Next Node: %p\n", current_node->nextNode);
        printf("Prev Node: %p\n", current_node->prevNode);
        current_node = current_node->nextNode;
    }
}

void setSize(freeNode *curr, size_t size, int ifmalloc){
    blockSize* mallocBlockHead = (void*)curr - sizeof(struct blockSize);
    mallocBlockHead->size = size+ifmalloc; 

    blockSize* mallocBlockTail = (void*)mallocBlockHead - sizeof(struct blockSize) + size;
    mallocBlockTail->size = size+ifmalloc; 

}

void movePointer(freeNode* removed, freeNode* replaced){

    replaced->nextNode = removed->nextNode;
    replaced->prevNode = removed->prevNode;
    removed->prevNode->nextNode = replaced;

    if(replaced->nextNode != NULL) removed->nextNode->prevNode = replaced;
    if (removed == end) end = replaced;

    removed->nextNode = NULL;
    removed->prevNode = NULL;
}

void split(freeNode *fit, size_t size){ //Takes a fit block and split to 2 Nodes 1 malloced of size 1 free leftover
    size_t totalsize = (8-(size%8==0 ? 8 : size%8)+size) + 2*sizeof(struct blockSize);
    if(totalsize < 32) totalsize = 32;
    size_t sizeFree = ((blockSize*)((void*)fit - sizeof(struct blockSize)))->size - totalsize;

    setSize(fit,totalsize,1);

    freeNode *newBlock = (void*)((void*)fit + totalsize);

    setSize(newBlock,sizeFree,0);
    
    movePointer(fit, newBlock);
    next = newBlock; 

}

void* allocateMem(size_t currSize, size_t totalsize, size_t payloadSize, freeNode* curr){
    if(currSize == (totalsize)){
        next = curr->nextNode;
        setSize(curr,currSize,1);
        curr->prevNode->nextNode = curr->nextNode;
        if(curr->nextNode) curr->nextNode->prevNode = curr->prevNode;
        if(root->nextNode == NULL) end = NULL; 
        return (void*)(curr); 
    }
    else if( currSize > totalsize ){
        if(currSize-totalsize < 32) {
            next = curr->nextNode;
            setSize(curr,currSize,1);
            curr->prevNode->nextNode = curr->nextNode;
            if(curr->nextNode) curr->nextNode->prevNode = curr->prevNode;
            if(root->nextNode == NULL) end = NULL; 
        }
        else split(curr, payloadSize);
        return (void*)(curr);
    }
    return NULL; //When no freed space for malloc 
}

void myinit(int allocAlg){   //0: first fit, 1: next fit, 2: best fit
    fit = allocAlg;
    mymalloc_arr = malloc(1024*1024); 

    root = malloc(sizeof(struct freeNode));
    blockSize *header = (void*)mymalloc_arr + 8;
    header->size = (1024*1024 - 8);
    blockSize *footer = (void*)(header) + header->size - sizeof(struct blockSize);
    footer->size = header->size; 

    //Pointers
    freeNode *rootNext = (void*)header + sizeof(struct blockSize);
    root->nextNode = rootNext;
    root->prevNode = NULL;
    rootNext->nextNode = NULL;
    rootNext->prevNode = root;
    end = rootNext;
    next = root->nextNode;
}

void* findFirstFree(size_t payloadSize){
    freeNode *curr = root->nextNode; 

    size_t currSize = ((blockSize*)((void*)curr - sizeof(struct blockSize)))->size;
    size_t totalsize = (8-(payloadSize%8==0 ? 8 : payloadSize%8)+payloadSize) + 2*sizeof(struct blockSize);
    if(totalsize < 32) totalsize = 32;
    while( (currSize < totalsize )  && curr->nextNode != NULL){
        curr = curr->nextNode;
        currSize = getSizeofBlock(curr);
    }
    return allocateMem(currSize, totalsize, payloadSize, curr);
}

void* findNextFree(size_t payloadSize){
    if(next == NULL) next = root->nextNode;
    freeNode* curr = next;
    freeNode* secPtr = curr;
    size_t currSize = getSizeofBlock(curr);
    size_t totalsize = (8-(payloadSize%8==0 ? 8 : payloadSize%8)+payloadSize) + 2*sizeof(struct blockSize);
    if(totalsize < 32) totalsize = 32;

    if(currSize >= totalsize) return allocateMem(currSize, totalsize, payloadSize, curr);
    if(curr->nextNode == NULL) curr = root;

    //end of the list
    while(curr->nextNode && currSize < totalsize && curr->nextNode != secPtr){

        if(curr->nextNode == NULL){
            curr = root;
        }
        curr = curr->nextNode;
        currSize = getSizeofBlock(curr);
    }
    return allocateMem(currSize, totalsize, payloadSize, curr);
}

void* findBestFree(size_t payloadSize){
    freeNode *curr = root->nextNode; 
    freeNode *bestFit = curr; 
    size_t currSize = getSizeofBlock(curr);
    size_t totalsize = (8-(payloadSize%8==0 ? 8 : payloadSize%8)+payloadSize) + 2*sizeof(struct blockSize);
    if(totalsize < 32) totalsize = 32;
    size_t minDiff = 1024*1024 - 8;
    size_t bestFitSize = 0; 
    if(currSize - totalsize < minDiff && currSize - totalsize >= 0){
        minDiff = currSize - totalsize;
        bestFitSize = currSize;
    }
    
    while(curr->nextNode != NULL){
        curr = curr->nextNode;
        currSize = getSizeofBlock(curr);
        if(currSize - totalsize < minDiff && currSize - totalsize >= 0){
            bestFit = curr;
            minDiff = currSize - totalsize;
            bestFitSize = currSize;
        }
    }
    
    return allocateMem(bestFitSize, totalsize, payloadSize, bestFit);
}

void* mymalloc(size_t size){
    if ((int)size < 1 || !root->nextNode) return NULL; 
    if(fit == 0){
        return findFirstFree(size);
    }
    else if(fit == 1){
        return findNextFree(size);
    }
    else if(fit == 2){
        return findBestFree(size);
    }
    else return NULL;

}

void insertAfter(freeNode* prev_node, freeNode* new_node){
    /*1. check if the given prev_node is NULL */
    if (prev_node == NULL) {
        prev_node = root;
    }
    /* 4. Make next of new node as next of prev_node */
    new_node->nextNode = prev_node->nextNode;
 
    /* 5. Make the next of prev_node as new_node */
    prev_node->nextNode = new_node;
 
    /* 6. Make prev_node as previous of new_node */
    new_node->prevNode = prev_node;
 
    /* 7. Change previous of new_node's next node */
    if (new_node->nextNode != NULL)
        new_node->nextNode->prevNode = new_node;
    else end = new_node;
}

void insertFreelistInList(void* ptr){
    if(!ptr) return;
    freeNode* currptr = (void*)ptr;
    freeNode *current_node = root->nextNode;
    if(current_node == NULL){
        insertAfter(root,currptr);
        return;       
    }
   	while ( current_node != NULL && current_node < currptr) {
        if(current_node->nextNode == NULL) {
            insertAfter(current_node,currptr);
            return;
        }
        current_node = current_node->nextNode;        
    }
    insertAfter(current_node->prevNode,currptr);
}

void* coalesceForward(void* ptr){
    if(!end || ((void*)ptr > (void*)end)) return ptr; 
    size_t size = getSizeofBlock(ptr);
    blockSize *newBlock = (void*)((void*)ptr + size - sizeof(struct blockSize));
    if(newBlock->size % 2 == 1) return (void*)ptr; //Next block is malloced 
    size_t totalSize = size; 
    totalSize += newBlock->size; 
    
    freeNode *newFree = (void*)ptr; 
    setSize(newFree,totalSize,0);

    //Set next and prev  
    freeNode *pointers = (void*)((void*)ptr + size);
    movePointer(pointers, newFree); 
    if(newFree->nextNode == NULL) end = newFree;
    return (void*)ptr;
    
}

void deleteCurr(freeNode* prev, freeNode* removedCurr){

    prev->nextNode = removedCurr->nextNode;    
    if(prev->nextNode != NULL) removedCurr->nextNode->prevNode = prev;

    removedCurr->nextNode = NULL;
    removedCurr->prevNode = NULL;
}

void* coalesceBackward(void* ptr, size_t initialSize){
    //size of prev block
    if(!root->nextNode || root->nextNode > (freeNode*)ptr) return ptr;
    size_t size = getSizeofBlock(ptr);
    size_t prevBlockSize = getSizeofBlock((void*)ptr - sizeof(struct blockSize));
    if(prevBlockSize % 2 == 1) return (void*)ptr;  //Prev block is malloced 
    size_t totalSize = size; 
    totalSize += prevBlockSize; 

    freeNode *newFree = (freeNode*)((void*)ptr - prevBlockSize);

    setSize(newFree,totalSize,0);
    if(size != initialSize) deleteCurr(newFree,(freeNode*)ptr);
    if(newFree->nextNode == NULL) end = newFree;
    return (void*)newFree; 
}

//freeing the allocated memory
void myfree(void* ptr){
    if(ptr == NULL) return;
    blockSize* headerSize = (void*)ptr - sizeof(struct blockSize);
    size_t initialSize = getSizeofBlock(ptr) - 1;
    headerSize->size = initialSize;
    blockSize* footerSize = ((void*)ptr + initialSize - 2*sizeof(struct blockSize));
    footerSize->size = initialSize;

    ptr = coalesceForward(ptr);
    ptr = coalesceBackward(ptr,initialSize);
    
    size_t currentSize = getSizeofBlock(ptr);
    if(currentSize == initialSize) insertFreelistInList(ptr);
}

void* myrealloc(void* ptr, size_t size){
    if(!ptr) return mymalloc(size);
    if(size == 0){
        myfree(ptr);
        return NULL;
    }

    int msize = getSizeofBlock(ptr);
    if (size <= msize) return ptr;
    void* newptr = mymalloc(size);

    memmove(newptr, ptr, msize);
    myfree(ptr);

    return newptr;
}

void mycleanup(){
    free(mymalloc_arr);
    free(root);
}

