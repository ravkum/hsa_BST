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
* @file <hsa_BST.cpp>
*
* @brief This file contains functions for initializing HSA CU.
* It creates a binary search tree in shared virtual memory and also 
* enqueues work to the CU for creating node and inserting in the same Binary Search Tree
*
* This shows SVM and atomics functionality of HSA.
********************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <process.h>  
#include <windows.h>
#include "hsa_helper.h"
#include "hsa_BST.h"
#include "svm_data_struct.h"
#include "cpu_BST.h"
#include "SDKUtil.hpp"
using namespace appsdk;

#define BUILD_LOG_SIZE 1024

double time_spent;

DeviceSVMFunctions dF;

void initialize_mutex_array(svm_mutex *mutex, int n)
{
	for (int i = 0; i < n; i++) {
		svm_mutex_init(&mutex[i], SVM_MUTEX_UNLOCK);
	}
}

int main(int argc, char* argv[])
{
	cl_int status = 0;

	/* HSA init */
	cl_platform_id platform = initializePlatform();
	if (platform == 0) {
		printf("No OpenCL platform found!\n");
		exit(!CL_SUCCESS);
	}

	cl_context context = createHSAContext(platform);

	cl_device_id  device = selectHSADevice(context);
	if (device == 0) {
		printf("No SVM device found\n");
		exit(1);
	}

	cl_command_queue gpuQueue;
	gpuQueue = clCreateCommandQueue(context, device, 0, &status);
	ASSERT_CL(status, "Error creating command queue");

	cl_device_svm_capabilities_amd svmCaps;
	status = clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES_AMD, sizeof(cl_device_svm_capabilities_amd), &svmCaps, NULL);
	ASSERT_CL(status, "Error when getting the device info");
	if (!(svmCaps & CL_DEVICE_SVM_ATOMICS_AMD)) {
		printf("Device doesn't support SVM atomics!\n");
		exit(1);
	}
	
	DeviceSVMMode deviceSVM = detectSVM(device);
	setDeviceSVMFunctions(platform, deviceSVM, &dF);

	std::string kernelString = readCLFile("bst.cl");
	const char* kernelCString = kernelString.c_str();
	cl_program program = clCreateProgramWithSource(context, 1, &kernelCString, NULL, &status);
	ASSERT_CL(status, "Error when creating CL program");

	status = clBuildProgram(program, 1, &device, "-I . -Wf,--support_all_extension", NULL, NULL);
	if (status != CL_SUCCESS) {
		char buildLog[BUILD_LOG_SIZE];
		status = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, BUILD_LOG_SIZE, buildLog, NULL);
		printf("Build log: %s\n", buildLog);
		ASSERT_CL(status, "Error when building CL program");
	}

	cl_kernel kernel = clCreateKernel(program, "bst_insert", &status);
	ASSERT_CL(status, "Error when creating bst_insert kernel");

	cl_command_queue queue = clCreateCommandQueue(context, device, 0, &status);
	ASSERT_CL(status, "Error when creating a command queue");

	int cpu_nodes = 1000000;
	int gpu_nodes = 1000000;
	int num_nodes = cpu_nodes + gpu_nodes;
	int num_gpu_wi = gpu_nodes;
	size_t globalSize = (size_t)num_gpu_wi;
	size_t preferredLocalSize = 64;

	node *data = NULL;
	svm_mutex *mutex = NULL;

	/* Data allocation and initialization */
	int flags = CL_MEM_READ_WRITE|CL_MEM_SVM_FINE_GRAIN_BUFFER_AMD;
	if ((data = (node *) dF.clSVMAlloc(context, flags, (unsigned int)num_nodes * sizeof(node), 0)) == NULL) {
		printf("Error allocating memory for nodes.\n");
		exit(1);
	}
	initialize_nodes(data, num_nodes);

	if ((mutex = (svm_mutex *) dF.clSVMAlloc(context, flags, (unsigned int)num_gpu_wi * sizeof(svm_mutex), 0)) == NULL) {
		printf("Error allocating memory for nodes.\n");
		exit(1);
	}
	initialize_mutex_array(mutex, num_gpu_wi);

	/* Timer init */
	SDKTimer *sdk_timer = new SDKTimer();
	int timer = sdk_timer->createTimer();
	sdk_timer->resetTimer(timer);
	sdk_timer->startTimer(timer);

	/* Construct initial BST in the cpu */
	node *root = construct_BST(cpu_nodes, data);
	//print_inorder(root);

	sdk_timer->stopTimer(timer);
	time_spent = sdk_timer->readTimer(timer);
	printf("Time to insert %d nodes on the CPU= %.10f ms\n", cpu_nodes, 1000*time_spent); 
	
	sdk_timer->resetTimer(timer);
	sdk_timer->startTimer(timer);
	
	/* Gpu work enqueue */
	status  = dF.clSetKernelArgSVMPointer(kernel, 0, root);
	status |= dF.clSetKernelArgSVMPointer(kernel, 1, data);
	status |= dF.clSetKernelArgSVMPointer(kernel, 2, &cpu_nodes);
	status |= dF.clSetKernelArgSVMPointer(kernel, 3, mutex);
	ASSERT_CL(status, "Error set kernel arg.");

	status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &preferredLocalSize, 0, NULL, NULL);
	ASSERT_CL(status, "Error when enqueuing kernel");

	clFinish(queue);
	
	sdk_timer->stopTimer(timer);
	time_spent = sdk_timer->readTimer(timer);
	printf("Time to insert %d nodes on the GPU= %.10f ms\n", gpu_nodes, 1000 * time_spent); 

	/* Verify the generated BST */
	if (!isBST(root)) 
		printf("GPU created binary tree is not BST.\n");
	else 
		printf("The generated BST is verified.\n");

	printf("Total nodes in the tree: %d\n\n", count_node(root));

	//print_inorder(root);

	/* cleanup */
	dF.clSVMFree(context, data);
	dF.clSVMFree(context, mutex);

	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseProgram(program);
	clReleaseContext(context);

	return 0;
}

