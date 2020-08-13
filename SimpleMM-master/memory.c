#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include "memory.h"

#define PAGE_SIZE 4096
#define PAGE_METADATA_SIZE 16

struct page_metadata{
	int bucket_size;
	int number_of_bytes_available;
};

struct node{
	struct node* next;
	void* data_ptr;
};

void *pages[100000];
int pageCount = 0;	// Number of pages used
struct node* bucket_list[9] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};	// Bucket 0: 16 bytes, bucket 8: Higher than 2048 bytes

static void *alloc_from_ram(size_t size)
{
	assert((size % PAGE_SIZE) == 0 && "size must be multiples of 4096");
	void* base = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
	if (base == MAP_FAILED)
	{
		printf("Unable to allocate RAM space\n");
		exit(0);
	}
	return base;
}

static void free_ram(void *addr, size_t size)
{
	munmap(addr, size);
}

int get_bucket_index(size_t size){
	if(size <= 16){
		return 0;
	}
	if(size == 32){
		return 1;
	}
	if(size == 64){
		return 2;
	}
	if(size == 128){
		return 3;
	}
	if(size == 256){
		return 4;
	}
	if(size == 512){
		return 5;
	}
	if(size == 1024){
		return 6;
	}
	if(size == 2048){
		return 7;
	}
	if(size > 2048){   // Bucket 8 contains all cases needin a full page or more
		return 8;
	}
}

int bucket_id_to_size(int bucket_id)
{
	int size = 16;
	while (bucket_id>0)
	{
		size = size << 1;
		bucket_id--;
	}
	return size;
}

// Add a new node at the head of the linked list
void add_to_linked_list(int bucket_id, struct node* new_node)
{
	// bucket_list[bucket_id] points to the head of the linke dlist
	if (bucket_list[bucket_id]==NULL)
	{
		bucket_list[bucket_id] = new_node;
	}
	else
	{
		new_node->next = bucket_list[bucket_id];
		bucket_list[bucket_id] = new_node;
	}
}

// Delete and return a node from the head of the linked list
struct node* delete_from_head_of_linked_list(int bucket_id)
{
	struct node* ret_node = bucket_list[bucket_id];
	bucket_list[bucket_id] = bucket_list[bucket_id]->next;
	return ret_node;
}

// Given the value of the data_ptr of a node, delete from anywhere in the linked list
// Returns 0 is successful, 1 if not
int delete_specific_node_from_linked_list(int bucket_id, void *data_ptr)
{
	struct node* ptr = bucket_list[bucket_id];
	struct node* prev = NULL;
	while (ptr != NULL && ptr->data_ptr != data_ptr)
	{
		prev = ptr;
		ptr = ptr->next;
	}
	if (ptr==NULL)
	{
		printf("Node with data ptr = %p not found in bucket_id %d\n",data_ptr,bucket_id);
		return 1;
	}
	// This implies ptr->data_ptr = data_ptr
	if (prev==NULL)
	{
		// ptr was the only node in the bucket_list
		bucket_list[bucket_id] = NULL;
	}
	else
	{
		prev->next = ptr->next;
	}
	return 0;
}

void print_linked_list(int bucket_id)
{
	// printf("Printing linked list for bucket %d\n", bucket_id);
	struct node* ptr = bucket_list[bucket_id];
	int count = 0;
	while (ptr != NULL)
	{
		count ++;
		// printf("Node ptr = %p, data_ptr = %p\n",ptr, ptr->data_ptr);
		ptr = ptr->next;
	}
	printf("Bucket list for bucket_id %d contains %d nodes\n", bucket_id, count);
}

size_t next_higher_power_of_2(size_t size)
{
	int shifts = 0; 
	size_t ret_val = 1;
	size--;  // Decrement by 1 so that an exact power of 2 returns itself
	// Count the number of right shifts required to reduce the number to 0
	while (size > 0)
	{
		shifts++;
		size = size >> 1;
	}
	// Left shift 1 the require dnumber of times
	while (shifts > 0){
		ret_val = ret_val << 1;
		shifts--;
	}
	return ret_val;
}

void *get_page_ptr_for_allocated_mem(void * allocated_mem_ptr)
{
	long long int num_ptr = (long long int)allocated_mem_ptr;
	void *page_corresponding_to_ptr = (void *) ((num_ptr/4096) * 4096);
	return page_corresponding_to_ptr;
}


void allocate_blocks_to_bucket(int bucket_id)
{
	printf("Allocating blocks to bucket list %d\n", bucket_id);
	void * page_ptr = alloc_from_ram(PAGE_SIZE);
	pages[pageCount] = page_ptr;
	pageCount++;

	struct page_metadata *page_metadata_ptr = (struct page_metadata *)page_ptr;
	int bucket_size = bucket_id_to_size(bucket_id);
	page_metadata_ptr->bucket_size = bucket_size;
	page_metadata_ptr->number_of_bytes_available = PAGE_SIZE - PAGE_METADATA_SIZE;
	int num_blocks = (page_metadata_ptr->number_of_bytes_available)/bucket_size;

	struct node* new_node = page_ptr + PAGE_METADATA_SIZE;
	for (int block_num = 0; block_num<num_blocks; block_num++)
	{
		new_node->next = NULL;
		new_node->data_ptr = (void *) new_node;
		add_to_linked_list(bucket_id, new_node);
		new_node = (void *)(new_node) + bucket_size;
	}
	print_linked_list(bucket_id);
}

int delete_page_from_pagelist(void *page_ptr)
{
	int pageFound = 0;
	for (int pageNum =0; pageNum < pageCount; pageNum++)
	{
		struct page_metadata *page_metadata_ptr = (struct page_metadata *)pages[pageNum];
		if (pageFound==1)
		{
			pages[pageNum-1] = pages[pageNum];
		}
		if (page_metadata_ptr == (struct page_metadata *)page_ptr)
		{
			free_ram(page_ptr, page_metadata_ptr->number_of_bytes_available + PAGE_METADATA_SIZE);
			pageFound = 1;
		}
	}
	if (pageFound==1)
	{
		pageCount--;
		return 0;
	}
	else
	{
		return 1;  // error. Page not found
	}
}

int delete_all_blocks_of_page(void *page_ptr)
{
	struct page_metadata *page_metadata_ptr = (struct page_metadata *)page_ptr;
	int bucket_size = page_metadata_ptr->bucket_size;
	int num_blocks = (page_metadata_ptr->number_of_bytes_available)/bucket_size;
	int bucket_id = get_bucket_index(bucket_size);
	printf("Deleting %d blocks for bucket_id %d for page_ptr %p\n",num_blocks,bucket_id,page_ptr);

	void *node_ptr = page_ptr + PAGE_METADATA_SIZE;
	for (int block_num = 0; block_num < num_blocks; block_num++)
	{
		printf("Trying to delete block %p\n", node_ptr);
		if (delete_specific_node_from_linked_list(bucket_id, node_ptr)==1)
		{
				return 1;
		}
		node_ptr += bucket_size;
	}
	delete_page_from_pagelist(page_ptr);
	return 0;
}


void myfree(void *ptr)
{
	// printf("myfree is not implemented\n");
	// abort();
	printf("Calling myfree for ptr %p\n", ptr);
	void *page_ptr = get_page_ptr_for_allocated_mem(ptr);
	struct page_metadata *page_metadata_ptr = (struct page_metadata *) (page_ptr);
	int bucket_size = page_metadata_ptr->bucket_size;
	int bucket_id = get_bucket_index(bucket_size);

	if (bucket_size > PAGE_SIZE/2)
	{
		// Full page needs to be deleted
		if (delete_page_from_pagelist(page_ptr)!=0)
		{
			printf("Error deleting full page with ptr %p\n",ptr);
		}
	}
	else
	{
		struct node* new_node = (struct node*)ptr;
		new_node->next = NULL;
		new_node->data_ptr = (void *) new_node;
		add_to_linked_list(bucket_id, new_node);

		page_metadata_ptr->number_of_bytes_available += bucket_size;
		if (page_metadata_ptr->number_of_bytes_available == PAGE_SIZE - PAGE_METADATA_SIZE)
		{
			// printf("Need to delete page\n");
			if (delete_all_blocks_of_page(page_ptr)!=0)
			{
				printf("Error deleting all blocks on page with ptr %p\n",ptr);
			}

		}
	}
}

void *mymalloc(size_t size)
{
	void *allocated_page_ptr, *allocated_mem_ptr;
	printf("Calling mymalloc size %zu\n", size);
	if (size*2.0/PAGE_SIZE > 1.0)
	{
		// Current requirement is more than half a page size. So at least one full page is needed
		size_t mem_size = next_higher_power_of_2(size + PAGE_METADATA_SIZE);
		printf("Allocating size %zu\n",mem_size);
		allocated_page_ptr = alloc_from_ram(mem_size);
		allocated_mem_ptr = allocated_page_ptr + PAGE_METADATA_SIZE;
		struct page_metadata* page_metadata_ptr = (struct page_metadata*) allocated_page_ptr;

		page_metadata_ptr->bucket_size = mem_size;
		page_metadata_ptr->number_of_bytes_available = mem_size-PAGE_METADATA_SIZE;

		pages[pageCount] = allocated_page_ptr;
		pageCount++;

		return allocated_mem_ptr;
	}
	else
	{
		size_t bucket_size = next_higher_power_of_2(size);
		int bucket_id = get_bucket_index(bucket_size);
		if (bucket_list[bucket_id] == NULL)
		{
			allocate_blocks_to_bucket(bucket_id);
		}
		allocated_mem_ptr = delete_from_head_of_linked_list(bucket_id);
		struct page_metadata *page_metadata_ptr = (struct page_metadata *) (get_page_ptr_for_allocated_mem(allocated_mem_ptr));
		page_metadata_ptr->number_of_bytes_available -= bucket_size;
		return allocated_mem_ptr;
	}
	return NULL;
}
