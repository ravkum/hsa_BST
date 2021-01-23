/*******************************************************************************
Copyright ©2013 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1 Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2 Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/**
********************************************************************************
* @file <cpu_BST.cpp>
*
* @brief This file contains functions for creating bst in cpu.
*
********************************************************************************
*/

#include "cpu_bst.h"

static int iterative_insert(node **root, node *new_node)
{
	node *tmp = NULL;
	int key;
	key = new_node->value;

	if (!(*root)) {
		*root = new_node;
		return 0;
	}

	tmp = *root;

	while (1) {
		if (key < tmp->value) {
			if (tmp->left == NULL) {
				tmp->left = new_node;
				break;
			}
			else {
				tmp = tmp->left;
			}
		}

		else {
			if (tmp->right == NULL) {
				tmp->right = new_node;
				break;
			}
			else {
				tmp = tmp->right;
			}
		}
	}	

	return 0;
}

static void recursive_insert(node **root, node *new_node)
{
	int key = new_node->value;

	if(!(*root)) {
		*root = new_node;
		return;
	}
	else if(key < (*root)->value) {
		recursive_insert(&(*root)->left, new_node);
	}
	else if(key >= (*root)->value) {
		recursive_insert(&(*root)->right, new_node);
	}

	return;
}

node * construct_BST(int num_nodes, node *data)
{
	node *root = NULL;

	/* Inserting nodes into tree */
	for (int i = 0; i < num_nodes; i++)
	{
		recursive_insert(&root, &(data[i]));
	}

	return root;
}

void initialize_nodes(node *data, int num_nodes)
{
	int random_num;
	node *tmp_node;

	for (int i = 0; i < num_nodes; i++)
	{
		tmp_node = &(data[i]);

		random_num = rand();
		tmp_node->value = random_num;
		tmp_node->left = NULL;
		tmp_node->right = NULL;
	}
}

void print_inorder(node * leaf) 
{
	if (leaf) {
		print_inorder(leaf->left);
		printf("%d ",leaf->value);
		print_inorder(leaf->right);
	}
}

static int node_count = 0;
/* Returns true if the given tree is a BST and its */
static int isBSTUtil(node* node, int min, int max) 
{ 
	if (node==NULL) 
		return 1;

	if (node->value < min || node->value > max) 
		return 0; 

	return isBSTUtil(node->left, min, node->value) && 
			isBSTUtil(node->right, node->value, max);  
}

/* Returns true if a binary tree is a binary search tree */
int isBST(node* node) 
{ 
	return isBSTUtil(node, INT_MIN, INT_MAX); 
} 

int count_node(node *root)
{
	int count = -1;
	if (root)
		count = 1;

	if (root->left)
		count += count_node(root->left);

	if (root->right)
		count += count_node(root->right);

	return count;
}
