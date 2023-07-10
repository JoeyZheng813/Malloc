#ifndef MYMALLOC_H_   /* Include guard */
#define MYMALLOC_H_
#include<stdio.h>
#include<stddef.h>

void myinit(int allocAlg);
void* mymalloc(size_t size);
void myfree(void* ptr);
void* myrealloc(void* ptr, size_t size);
void mycleanup();

#endif              