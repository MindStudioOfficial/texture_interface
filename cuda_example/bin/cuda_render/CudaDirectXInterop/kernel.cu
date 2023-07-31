
#include "kernel.cuh"

ID3D11Device* d3d11Device = NULL;


void gpuAssert(cudaError_t code, const char* file, int line)
{
	if (code != cudaSuccess)
	{
		printf("GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
		fflush(stdout);
	}
}

void setD3D11device(void* device) {
	d3d11Device = reinterpret_cast<ID3D11Device*>(device);
}

void renderToTexture(void* sharedHandle, size_t resourceSize, unsigned int width, unsigned int height) {
	cudaExternalMemoryHandleDesc externalHandleDesc;
	memset(&externalHandleDesc, 0, sizeof(externalHandleDesc));

	externalHandleDesc.type = cudaExternalMemoryHandleTypeD3D11Resource;
	externalHandleDesc.handle.win32.handle = sharedHandle;
	externalHandleDesc.size = resourceSize;
	externalHandleDesc.flags = cudaExternalMemoryDedicated;

	cudaExternalMemory_t externalMemory;

	cudaError_t err = cudaImportExternalMemory(&externalMemory, &externalHandleDesc);
	cudaCheckPrintError(err);

	cudaExternalMemoryMipmappedArrayDesc cuExtmemMipDesc{};
	cuExtmemMipDesc.extent = make_cudaExtent(width, height, 0);
	cuExtmemMipDesc.formatDesc = cudaCreateChannelDesc<uint4>();
	cuExtmemMipDesc.numLevels = 1;
	cuExtmemMipDesc.flags = cudaArraySurfaceLoadStore;

	cudaMipmappedArray_t cuMipArray{};

	err = cudaExternalMemoryGetMappedMipmappedArray(&cuMipArray, externalMemory, &cuExtmemMipDesc);
	cudaCheckPrintError(err);

	cudaArray_t cuArray{};
	err = cudaGetMipmappedArrayLevel(&cuArray, cuMipArray,0);
	cudaCheckPrintError(err);

	cudaResourceDesc cuResDesc{};
	cuResDesc.resType = cudaResourceTypeArray;
	cuResDesc.res.array.array = cuArray;

	cudaSurfaceObject_t cuSurface{};
	err = cudaCreateSurfaceObject(&cuSurface, &cuResDesc);
	cudaCheckPrintError(err);

}