#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef struct {
    unsigned char red, green, blue;
} pixel;

typedef struct qtree {
    unsigned char medred, medgreen, medblue;
    uint32_t area;
    int32_t index;
    struct qtree *top_left, *top_right, *bottom_right, *bottom_left;
} qtree;


typedef struct QuadtreeNode {
    unsigned char blue, green, red;
    uint32_t area;
    int32_t top_left, top_right;
    int32_t bottom_left, bottom_right;
} __attribute__ ((packed)) QuadtreeNode;

typedef struct node {
    qtree *val;
    struct node *next;
} que_node;

pixel **readinput(const char *filename, int *width, int *height) {
    pixel **img;
    FILE *f;
    char type[3];
    int i, rgb;
    f = fopen(filename, "rb");
    fgets(type, sizeof(type), f);
    fscanf(f, "%d %d", width, height);
    fscanf(f, "%d", &rgb);
    fgetc(f);
    img = (pixel**)malloc((*width) * sizeof(pixel*));
    for (i = 0; i < *width; i++)
        img[i] = (pixel*)malloc((*width) * sizeof(pixel));
    for (i = 0; i < *width; i++)
        fread(img[i], sizeof(pixel), *width, f);
    fclose(f);
    return img;
}

qtree *alloc() {
    qtree *root = (qtree*)malloc(sizeof(qtree));
    root->top_right = NULL;
    root->top_left = NULL;
    root->bottom_right = NULL;
    root->bottom_left = NULL;
    return root;
}

uint32_t leaf_number(qtree *root) {
    if (root->top_right == NULL){
        return 1;
    }
    return leaf_number(root->top_right) + leaf_number(root->bottom_left) + leaf_number(root->top_left) + leaf_number(root->bottom_right);
}

uint32_t node_number(qtree *root) {
    if (root == NULL)
        return 0;
    return 1 + node_number(root->top_right) + node_number(root->bottom_left) + node_number(root->top_left) + node_number(root->bottom_right);
}

void getmed(pixel **img, int x, int y, int dist, long long *red, long long *green, long long *blue) {
    for (int i = 0; i < dist; i++)
        for (int j = 0; j < dist; j++) {
            *red += img[x+i][y+j].red;
            *green += img[x+i][y+j].green;
            *blue += img[x+i][y+j].blue;
        }

    *red /= (dist*dist);
    *green /= (dist*dist);
    *blue /= (dist*dist);
    return;
}

long long score(pixel **img, int x, int y, int dist) {
    long long mediag = 0, mediab = 0, mediar = 0;
    getmed(img, x, y, dist, &mediar, &mediag, &mediab);
    long long mean = 0;
    for (int i = x; i < x + dist; i++)
        for (int j = y; j < y + dist; j++){
            mean += (mediar - img[i][j].red)*(mediar - img[i][j].red);
            mean += (mediag - img[i][j].green)*(mediag - img[i][j].green);
            mean += (mediab - img[i][j].blue)*(mediab - img[i][j].blue);
        }
    mean /= (3LL*dist*dist);
    return mean;
}

void maketree(qtree **root, pixel **img, int x, int y, int dist, int prag) {
    *root = alloc();
    long long mediag = 0, mediab = 0, mediar = 0;
    getmed(img, x, y, dist, &mediar, &mediag, &mediab);
    (*root)->medred = mediar;
    (*root)->medgreen = mediag;
    (*root)->medblue = mediab;
    (*root)->area = (uint32_t) dist*dist;

    if(score(img, x, y, dist) <= prag)
        return;

    maketree(&(*root)->top_left, img, x, y, dist/2, prag);
    maketree(&(*root)->top_right, img, x, y + (dist/2), dist/2, prag);
    maketree(&(*root)->bottom_left, img, x + (dist/2), y, dist/2, prag);
    maketree(&(*root)->bottom_right, img, x + (dist/2), y + (dist/2), dist/2, prag);
    return;
}

void enq(que_node **head, que_node **last, qtree *val) {
    que_node *new = malloc(sizeof(que_node));
    new->val = val;
    new->next = NULL;
    if (*head == NULL) {
        *head = new;
        *last = *head;
        return;
    }
    (*last)->next = new;
    *last = (*last)->next;
}

qtree *deq(que_node **head) {
    qtree *val = (*head)->val;
    if (*head == NULL)
        return val;
    que_node *aux = *head;
    val = aux->val;
    *head = (*head)->next;
    free(aux);
    return val;
}

int empty_que(que_node *head) {
    if (head == NULL)
        return 1;
    else
        return 0;
}

void set_tree_index(qtree *root) {
    que_node *head = NULL;
    qtree *current = NULL;
    que_node *last = NULL;
    enq(&head, &last, root);
    int32_t i = 0;
    while (!empty_que(head)) {
        current = deq(&head);
        current->index = i;
        i += 1;
        if (current->top_left != NULL)
            enq(&head, &last, current->top_left);
        if (current->top_right != NULL)
            enq(&head, &last, current->top_right);
        if (current->bottom_right != NULL)
            enq(&head, &last, current->bottom_right);
        if (current->bottom_left != NULL)
            enq(&head, &last, current->bottom_left);
    }
    return;
}

void swap(qtree **node1, qtree **node2) {
    qtree *swap;
    swap = *node1;
    *node1 = *node2;
    *node2 = swap;
}

void mirror_h(qtree **root) {
    que_node *head = NULL;
    qtree *current = NULL;
    que_node *last = NULL;
    enq(&head, &last, *root);
    while (!empty_que(head)) {
        current = deq(&head);
        if (current->top_left != NULL)
            enq(&head, &last, current->top_left);
        if (current->top_right != NULL)
            enq(&head, &last, current->top_right);
        if (current->bottom_right != NULL)
            enq(&head, &last, current->bottom_right);
        if (current->bottom_left != NULL)
            enq(&head, &last, current->bottom_left);
        swap(&current->top_left, &current->top_right);
        swap(&current->bottom_left, &current->bottom_right);
    }
    return;
}

void mirror_v(qtree **root) {
    que_node *head = NULL;
    qtree *current = NULL;
    que_node *last = NULL;
    enq(&head, &last, *root);
    while (!empty_que(head)) {
        current = deq(&head);
        if (current->top_left != NULL)
            enq(&head, &last, current->top_left);
        if (current->top_right != NULL)
            enq(&head, &last, current->top_right);
        if (current->bottom_right != NULL)
            enq(&head, &last, current->bottom_right);
        if (current->bottom_left != NULL)
            enq(&head, &last, current->bottom_left);
        swap(&current->top_left, &current->bottom_left);
        swap(&current->top_right, &current->bottom_right);
    }
    return;
}

void create_vector(qtree *root, QuadtreeNode *vector) {
    que_node *head = NULL;
    qtree *current = NULL;
    que_node *last = NULL;
    enq(&head, &last, root);
    int i = 0;
    while (!empty_que(head)) {
        current = deq(&head);
        vector[i].red = current->medred;
        vector[i].green = current->medgreen;
        vector[i].blue = current->medblue;
        vector[i].area = current->area;
        if (current->top_left == NULL) {
            vector[i].top_left = -1;
            vector[i].top_right = -1;
            vector[i].bottom_right = -1;
            vector[i].bottom_left = -1;
        }
        else {
            vector[i].top_left = current->top_left->index;
            vector[i].top_right = current->top_right->index;
            vector[i].bottom_right = current->bottom_right->index;
            vector[i].bottom_left = current->bottom_left->index;
        }

        i += 1;

        if (current->top_left != NULL)
            enq(&head, &last, current->top_left);
        if (current->top_right != NULL)
            enq(&head, &last, current->top_right);
        if (current->bottom_right != NULL)
            enq(&head, &last, current->bottom_right);
        if (current->bottom_left != NULL)
            enq(&head, &last, current->bottom_left);
    }
    return;
}

qtree *create_node (QuadtreeNode val) {
    qtree *root = alloc();
    root->medred = val.red;
    root->medblue = val.blue;
    root->medgreen = val.green;
    root->area = val.area;
    return root;
}

void vector_to_tree(qtree **root, QuadtreeNode *vector, int32_t i, uint32_t nr_noduri) {
    if (*root == NULL)
        return;
    if (vector[i].top_left != -1) {
        (*root)->top_left = create_node(vector[vector[i].top_left]);
        (*root)->top_right = create_node(vector[vector[i].top_right]);
        (*root)->bottom_left = create_node(vector[vector[i].bottom_left]);
        (*root)->bottom_right = create_node(vector[vector[i].bottom_right]);
    }
    vector_to_tree(&(*root)->top_left, vector, vector[i].top_left, nr_noduri);
    vector_to_tree(&(*root)->top_right, vector, vector[i].top_right, nr_noduri);
    vector_to_tree(&(*root)->bottom_left, vector, vector[i].bottom_left, nr_noduri);
    vector_to_tree(&(*root)->bottom_right, vector, vector[i].bottom_right, nr_noduri);
    return;
}

void makeimg(qtree *root, pixel **img, int x, int y, int dist) {
    if (root->top_right == NULL) {
        for (int i = x; i < x + dist; i++)
            for (int j = y; j < y + dist; j++) {
                img[i][j].red = root->medred;
                img[i][j].green = root->medgreen;
                img[i][j].blue = root->medblue;
            }
        return;
    }

    if (root->top_left != NULL)
        makeimg(root->top_left, img, x, y, dist/2);
    if (root->top_right != NULL)
        makeimg(root->top_right, img, x, y + dist/2, dist/2);
    if (root->bottom_right != NULL)
        makeimg(root->bottom_right, img, x + dist/2, y + dist/2, dist/2);
    if (root->bottom_left != NULL)
        makeimg(root->bottom_left, img, x + dist/2, y, dist/2);
    return;
}

void write(const char *filename, pixel **img, int width) {
    FILE *f;
    f = fopen(filename, "wb");
    fprintf(f, "P6\n");
    fprintf(f, "%d %d\n", width, width);
    fprintf(f, "%d\n", 255);
    for (int i = 0; i < width; i++)
        fwrite(img[i], sizeof(pixel), width, f);
    fclose(f);
}

void free_mem(qtree **root) {
    que_node *head = NULL;
    qtree *current = NULL;
    que_node *last = NULL;
    enq(&head, &last, *root);
    while (!empty_que(head)) {
        current = deq(&head);
        if (current->top_left != NULL)
            enq(&head, &last, current->top_left);
        if (current->top_right != NULL)
            enq(&head, &last, current->top_right);
        if (current->bottom_right != NULL)
            enq(&head, &last, current->bottom_right);
        if (current->bottom_left != NULL)
            enq(&head, &last, current->bottom_left);
        free(current);
    }
    return;
}

void combine_trees(qtree *current1, qtree *current2, qtree **rootfinal) {
    *rootfinal = alloc();
    (*rootfinal)->medred = (current1->medred + current2->medred) / 2;
    (*rootfinal)->medgreen = (current1->medgreen + current2->medgreen) / 2;
    (*rootfinal)->medblue = (current1->medblue + current2->medblue) / 2;
    if (current1->top_left == NULL && current2->top_left != NULL)
        combine_trees(current1, current2->top_left, &(*rootfinal)->top_left);
    else
        if (current1->top_left != NULL && current2->top_left == NULL)
            combine_trees(current1->top_left, current2, &(*rootfinal)->top_left);
        else
            if (current1->top_left != NULL && current2->top_left != NULL)
                combine_trees(current1->top_left, current2->top_left, &(*rootfinal)->top_left);

    if (current1->top_right == NULL && current2->top_right != NULL)
        combine_trees(current1, current2->top_right, &(*rootfinal)->top_right);
    else
        if (current1->top_right != NULL && current2->top_right == NULL)
            combine_trees(current1->top_right, current2, &(*rootfinal)->top_right);
        else
            if (current1->top_right != NULL && current2->top_right != NULL)
                combine_trees(current1->top_right, current2->top_right, &(*rootfinal)->top_right);

    if (current1->bottom_left == NULL && current2->bottom_left != NULL)
        combine_trees(current1, current2->bottom_left, &(*rootfinal)->bottom_left);
    else
        if (current1->bottom_left != NULL && current2->bottom_left == NULL)
            combine_trees(current1->bottom_left, current2, &(*rootfinal)->bottom_left);
        else
            if (current1->bottom_left != NULL && current2->bottom_left != NULL)
                combine_trees(current1->bottom_left, current2->bottom_left, &(*rootfinal)->bottom_left);

    if (current1->bottom_right == NULL && current2->bottom_right != NULL)
        combine_trees(current1, current2->bottom_right, &(*rootfinal)->bottom_right);
    else
        if (current1->bottom_right != NULL && current2->bottom_right == NULL)
            combine_trees(current1->bottom_right, current2, &(*rootfinal)->bottom_right);
        else
            if (current1->bottom_right != NULL && current2->bottom_right != NULL)
                combine_trees(current1->bottom_right, current2->bottom_right, &(*rootfinal)->bottom_right);
    return;
}

void compression (char *argv[]) {
    qtree *root = NULL;
    int factor = atoi(argv[2]);
    pixel **img;
    int width, height;
    img = readinput(argv[3], &width, &height);

    maketree(&root, img, 0, 0, width, factor);
    uint32_t nr_frunze = leaf_number(root);
    uint32_t nr_noduri = node_number(root);
    QuadtreeNode *vector = (QuadtreeNode*)malloc(nr_noduri * sizeof(QuadtreeNode));

    set_tree_index(root);
    create_vector(root, vector);

    FILE *f = fopen(argv[4], "wb");
    fwrite(&nr_frunze, sizeof(uint32_t), 1, f);
    fwrite(&nr_noduri, sizeof(uint32_t), 1, f);
    fwrite(vector, sizeof(QuadtreeNode), nr_noduri, f);
    fclose(f);

    free(vector);
    free_mem(&root);
    for (int i = 0; i < width; i++)
        free(img[i]);
    free(img);
}

void decompress(char *argv[]) {
    uint32_t nr_noduri;
    uint32_t nr_frunze;
    int width;
    FILE *f = fopen(argv[2], "rb");
    fread(&nr_frunze, sizeof(uint32_t), 1, f);
    fread(&nr_noduri, sizeof(uint32_t), 1, f);
    QuadtreeNode *vector = (QuadtreeNode*)malloc(nr_noduri * sizeof(QuadtreeNode));
    fread(vector, sizeof(QuadtreeNode), nr_noduri, f);
    fclose(f);
    qtree *root;
    root = create_node(vector[0]);
    vector_to_tree(&root, vector, 0, nr_noduri);
    width = (int) sqrt(vector[0].area);
    pixel **img;
    img = (pixel**)malloc(width * sizeof(pixel*));
    for (int i = 0; i < width; i++)
        img[i] = (pixel*)malloc(width * sizeof(pixel));
    makeimg(root, img, 0, 0, width);
    write(argv[3], img, width);

    free(vector);
    free_mem(&root);
    for (int i = 0; i < width; i++)
        free(img[i]);
    free(img);
}

void mirror(char *argv[]) {
    qtree *root = NULL;
    pixel **img;
    int width, height;
    int prag = atoi(argv[3]);
    img = readinput(argv[4], &width, &height);
    maketree(&root, img, 0, 0, width, prag);
    if (argv[2][0] == 'h')
        mirror_h(&root);
    else
        mirror_v(&root);
    makeimg(root, img, 0, 0, width);
    write(argv[5], img, width);
    free_mem(&root);
    for (int i = 0; i < width; i++)
        free(img[i]);
    free(img);
}

void overlay(char *argv[]) {
    qtree *root1 = NULL;
    qtree *root2 = NULL;
    int factor = atoi(argv[2]);
    pixel **img1, **img2;
    int width, height;
    img1 = readinput(argv[3], &width, &height);
    img2 = readinput(argv[4], &width, &height);
    qtree *root3 = NULL;
    maketree(&root1, img1, 0, 0, width, factor);
    maketree(&root2, img2, 0, 0, width, factor);

    combine_trees(root1, root2, &root3);
    makeimg(root3, img1, 0, 0, width);
    write(argv[5], img1, width);

    free_mem(&root1);
    free_mem(&root2);
    free_mem(&root3);
    for (int i = 0; i < width; i++) {
        free(img1[i]);
        free(img2[i]);
    }
    free(img1);
    free(img2);
}

int main(int argc, char *argv[]) {
    /* compression */
    if (strcmp(argv[1],"-c") == 0) {
        compression(argv);
        return 0;
    }
    else
        /* decompression */
        if (strcmp(argv[1],"-d") == 0) {
            decompress(argv);
            return 0;
        }
        else
            /* mirroring */
            if (strcmp(argv[1],"-m") == 0) {
                mirror(argv);
                return 0;
            }
            else
                /* overlay of two images */
                if (strcmp(argv[1], "-o") == 0) {
                    overlay(argv);
                return 0;
            }
}