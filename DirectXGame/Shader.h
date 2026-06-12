#pragma once

#include <d3d12.h> // ID3DBlob
#include <string>  // wstring, string

// Shaderクラス
class Shader {
public:
	// シェーダーファイルを読み込み、コンパイル済みデータを生成する
	void Load(const std::wstring& filePath, const std::string& shaderModel);

	// 生成したコンパイル済みデータを取得する
	ID3DBlob* GetBlob();

	// コンストラクタ
	Shader();
	// デストラクタ
	~Shader();

private:
	// コンストラクタで初期化しなくていい
	ID3DBlob* blob_ = nullptr;
};