#ifndef CPU_BST_H_
#define CPU_BST_H_

#include "hsa_BST.h"

#include "cpu_bst.h"

node * construct_BST(int num_nodes, node *data);
void initialize_nodes(node *data, int num_nodes);
void print_inorder(node * leaf);
int isBST(node* root);
int count_node(node *root);


#endif
