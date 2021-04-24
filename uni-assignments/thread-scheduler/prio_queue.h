#include <stdint.h>

typedef struct pqNode
{
	struct pqNode *next;
	unsigned int tIndex;
	uint8_t prio;
} pqNode;

void enqueue(pqNode **head, unsigned int tIndex, uint8_t prio);
unsigned int topPrio(pqNode *head);
unsigned int dequeue(pqNode **head);
void deleteAllNodes(pqNode **head);
