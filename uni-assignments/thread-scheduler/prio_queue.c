#include <stdio.h>
#include <stdlib.h>
#include "prio_queue.h"
#include "utils.h"

/* Initializes a node with given data */
static pqNode * newNode(unsigned int tIndex, uint8_t prio)
{
	pqNode *node;

	node = malloc(sizeof(pqNode));
	DIE(node == NULL, "malloc fail");
	node->next = NULL;
	node->tIndex = tIndex;
	node->prio = prio;
	return node;
}

/* Inserts a new node into the queue */
void enqueue(pqNode **head, unsigned int tIndex, uint8_t prio)
{
	pqNode *new_node;
	pqNode *iter = NULL;
	pqNode *prev = NULL;

	new_node = newNode(tIndex, prio);
	if (*head == NULL) {
		*head = new_node;
	} else {
		iter = *head;
		prev = iter;
		while (iter != NULL) {
			if (iter->prio < new_node->prio)
				break;
			prev = iter;
			iter = iter->next;
		}
		if (prev == iter) {
			new_node->next = iter;
			*head = new_node;
		} else {
			prev->next = new_node;
			new_node->next = iter;
		}
	}
}

/* Returns top node priority */
unsigned int topPrio(pqNode *head)
{
	if (head == NULL)
		return -1;
	else
		return head->prio;
}

/* Removes top node from queue and returns its index */
unsigned int dequeue(pqNode **head)
{
	pqNode *ret_node = *head;
	unsigned int ret_val;

	if (*head == NULL)
		return -1;
	*head = (*head)->next;
	ret_val = ret_node->tIndex;
	free(ret_node);
	return ret_val;
}

/* Deletes all nodes from the queue */
void deleteAllNodes(pqNode **head)
{
	pqNode *iter = *head;
	pqNode *next = iter;

	while (iter != NULL) {
		next = iter->next;
		free(iter);
		iter = next;
	}
	*head = NULL;
}
