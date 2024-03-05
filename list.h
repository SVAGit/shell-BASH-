#include <stdlib.h>
#include <stdio.h>

typedef struct node{
	char** data;
	int flag;
	struct node* next;
}node;

typedef struct list{
	node* data;
	int flag;
	struct list* next;
}list;

list* create_list(node* list, int flag){
	struct list* ptr = (struct list*)calloc(1, sizeof(struct list));
	ptr->data = list;
	ptr->flag = flag;
	ptr->next = NULL;
	return ptr;
}

void add_list(list** ptr, node* data, int flag){
	if(*ptr == NULL){
		*ptr = create_list(data, flag);
		return ;
	}
	add_list(&((*ptr)->next), data, flag);
}

node* create_node(char** data, int flag){
	node* ptr = (node*)calloc(1,sizeof(node));
	ptr->data = data;
	ptr->flag = flag;
	ptr->next = NULL;
	return ptr;
}

void add_node(node** head, char** data, int flag){
	if(*head == NULL){
		*head = create_node(data, flag);
		return ;
	}
	add_node(&((*head)->next), data, flag);
}


void readNode(node* list){
	node* ptr = list;
	int count = 1;
	while(ptr != NULL){
		printf("%d command with flag: %d is \n",count , ptr->flag);
		for(int i = 0; ptr->data[i] != NULL; i++){
			printf("%s ", ptr->data[i]);
		}
		printf("\n");
		ptr = ptr->next;
		count++;
	}
}

void readList(list* head){
	list* ptr = head;
	int count = 1;
	while(ptr != NULL){
		printf("%d pipeline with flag: %d is \n",count , ptr->flag);
		readNode(ptr->data);
		printf("\n");
		ptr = ptr->next;
		count++;
	}
}