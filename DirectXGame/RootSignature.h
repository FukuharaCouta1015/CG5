#pragma once

#include <d3d12.h>

class RootSignature {
public:
	void Create();

	ID3D12RootSignature* Get();

	// コンストラクタ
	RootSignature();

	// デストラクタ
	~RootSignature();

private:
	ID3D12RootSignature* rootSignature_ = nullptr;
};
