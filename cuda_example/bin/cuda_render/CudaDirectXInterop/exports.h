#pragma once


void setD3D11DeviceForCuda(void* device);


void renderToTexture(void* sharedHandle, size_t resourceSize, unsigned int width, unsigned int height);