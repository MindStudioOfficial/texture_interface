
#include "dllmain.h"


EXTERNC void setD3D11DeviceForCuda(void* device) {
	setD3D11device(device);
}

EXTERNC void renderToTextureWithCuda(void* sharedHandle, size_t resourceSize, unsigned int width, unsigned int height) {
	renderToTexture(sharedHandle, resourceSize, width, height);
}