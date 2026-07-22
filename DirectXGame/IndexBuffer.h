#pragma once

#include <d3d12.h>
class IndexBuffer {
public:
	void Create(const UINT size, const UINT stride);

	ID3D12Resource* Get();
	D3D12_INDEX_BUFFER_VIEW* GetView();

	IndexBuffer();
	~IndexBuffer();

private:
	ID3D12Resource* indexBuffer_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
};