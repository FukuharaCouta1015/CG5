#include "RootSignature.h"
#include "KamataEngine.h"

using namespace KamataEngine;

void RootSignature::Create() {
	// 既にインスタンスがあるなら解放する
	// Createメンバ関数が2度実行された時の対処
	if (rootSignature_) {
		rootSignature_->Release();
		rootSignature_ = nullptr;
	}
	// クラス内で取得するために追加
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// RootSignature作成
	// 構造体にデータを用意する
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリをもとに生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	// SignatureBlobはRootSignature精製後解放してもいい
	signatureBlob->Release();
	// 生成したRootSignatureを取っておく
	rootSignature_ = rootSignature;
}

// 生成したRootSignatureを返す
ID3D12RootSignature* RootSignature::Get() { return rootSignature_; }

// コンストラクタ
RootSignature::RootSignature() {}

// デストラクタ
RootSignature::~RootSignature() {
	if (rootSignature_) {
		rootSignature_->Release();
		rootSignature_ = nullptr;
	}
}