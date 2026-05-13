#include <Windows.h>
#include "KamataEngine.h"
#include <d3dcompiler.h>

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	KamataEngine::Initialize(L"LE3D_21_フクハラ_コウタ");

	// DirectXCommonのシングルトンインスタンスを取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	//  DirectXCommonクラスが管理しているウィンドウの高さと幅
	int32_t w = dxCommon->GetBackBufferWidth();
	int32_t h = dxCommon->GetBackBufferHeight();
	DebugText::GetInstance()->ConsolePrintf(std::format("width: {},height: {}\n", w, h).c_str());

	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	//------------------------------------------------------------------------- RootSignature作成
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

	// InputLayoutの作成
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};		
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendState-------------------------------

	D3D12_BLEND_DESC blendDesc{};
	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerState-------------------------
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面をカリングする
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 塗りつぶしモードをソリッドにする
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// コンパイル済のShader、エラー情報の格納
	ID3DBlob* vsBlob = nullptr;    // 頂点シェーダオブジェクト
	ID3DBlob* psBlob = nullptr;    // ピクセルシェーダオブジェクト
	ID3DBlob* errorBlob = nullptr; // エラーオブジェクト

	// 頂点シェーダの読み込みとコンパイル
	std::wstring vsFile = L"Resources/shaders/TestVS.hlsl";
	hr = D3DCompileFromFile(
	    vsFile.c_str(), // シェーダファイル名
	    nullptr,
	    D3D_COMPILE_STANDARD_FILE_INCLUDE,               // インクルード可能にする
	    "main", "vs_5_0",                                // エントリーポイント & シェーダモデル指定
	    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ設定
	    0, &vsBlob, &errorBlob);

	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(std::system_category().message(hr).c_str());
		if (errorBlob) {
			DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}

	// ピクセルシェーダの読み込みとコンパイル
	std::wstring psFile = L"Resources/shaders/TestPS.hlsl";
		hr = D3DCompileFromFile(
			psFile.c_str(), // シェーダファイル名
			nullptr,
			 D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"main","ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ設定
	    0, &psBlob, &errorBlob);
		
	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(std::system_category().message(hr).c_str());
		if (errorBlob) {
			DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}

	// PSOの作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	//書き込むRTV
	graphicsPipelineStateDesc.NumRenderTargets = 1;		
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトポロジのタイプ
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むか
	graphicsPipelineStateDesc.SampleDesc.Count = 1; // マルチサンプリングしない
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定
	// PSOの生成		
	ID3D12PipelineState* pipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineState));

	// VertexResourceの生成 --------
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込むヒープ

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファ
	vertexResourceDesc.Width = sizeof(Vector4) * 3;                  // リソースのサイズ。今回は Vector4 を3頂点分
	// バッファの場合はこれらは使わない
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際に頂点リソースを生成する
	ID3D12Resource* vertexResource = nullptr;
	hr = dxCommon->GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr)); // うまくいかなかったときは起動できない

	// VertexBufferViewの作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // 頂点リソースのGPU仮想アドレス
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3; // 頂点全体のサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4); // 頂点1つ分のサイズ

	// 頂点リソースにデータを書き込む

	Vector4* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // 頂点リソースをCPUアドレス空間にマッピング
	vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
	vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};
	vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};
	
	vertexResource->Unmap(0, nullptr); // 頂点リソースのマッピングを解除

	//描画開始

	dxCommon->PreDraw();

	commandList->SetGraphicsRootSignature(rootSignature); // ルートシグネチャのセット
	commandList->SetPipelineState(pipelineState); // PSOのセット
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // 頂点バッファビューのセット

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジのセット

	commandList->DrawInstanced(3, 1, 0, 0); // 描画コマンド

	// 描画終了

	dxCommon->PostDraw();

	// 解放処理
	vertexResource->Release();
	//graphicsPipelineState->Release();
	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	rootSignature->Release();
	vsBlob->Release();
	psBlob->Release();

	// エンジンの終了処理
	KamataEngine::Finalize();


	return 0;
}
