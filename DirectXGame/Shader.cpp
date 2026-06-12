#include "Shader.h"
#include <cassert>
#include <d3dcompiler.h>

void Shader::Load(const std::wstring& filePath, const std::string& shaderModel) {
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(
	    // シェーダーファイル名
	    filePath.c_str(), nullptr,
	    // インクルード可能にする
	    D3D_COMPILE_STANDARD_FILE_INCLUDE,
	    // エントリーポイント名
	    "main", shaderModel.c_str(),
	    // デバッグ用設定
	    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &shaderBlob, &errorBlob);

	// エラーが発生した場合止める
	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
		}
		assert(false);
	}
	// 生成したshaderBlobを返す
	blob_ = shaderBlob;
}

// コンパイル済のシェーダーデータを返す
ID3DBlob* Shader::GetBlob() { return blob_; }

// コンストラクタ
Shader::Shader() {}

// デストラクタ
Shader::~Shader() {
	if (blob_ != nullptr) {
		blob_->Release();
		blob_ = nullptr;
	}
}