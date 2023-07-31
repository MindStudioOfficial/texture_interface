
#pragma once

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <cuda_d3d11_interop.h>


#include <string.h>

#include <stdio.h>


void gpuAssert(cudaError_t code, const char* file, int line);

#define cudaCheckPrintError(ans)          \
    {                                           \
        gpuAssert((ans), __FILE__, __LINE__);   \
    }


void setD3D11device(void* device);


void renderToTexture(void* sharedHandle, size_t resourceSize, unsigned int width, unsigned int height);