#include "KamataEngine.h"
#include <Windows.h>

#include "Shader.h"

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// エンジンの初期化
	KamataEngine::Initialize(L"LE3D_21_フクハラコウタ");

	// DirectXCommonインスタンスを取得する
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

#pragma region レンダリングパイプライン(main.cpp-WinMain)

#pragma region DirectXCommonの管理の取得

	// DirectXCommonクラスが管理している、ウインドウの高さと幅の値の取得
	int32_t w = dxCommon->GetBackBufferWidth();
	int32_t h = dxCommon->GetBackBufferHeight();
	DebugText::GetInstance()->ConsolePrintf(std::format("width: {}, hieght: {}\n", w, h).c_str());

	// DirectXCommonクラスが管理している、コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

#pragma endregion

#pragma region RootSignature作成

	// 構造体にデータを用意する
	D3D12_ROOT_SIGNATURE_DESC descriptorRootSignature{};
	descriptorRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptorRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(false)) {
		DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリとともに生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

#pragma endregion

#pragma region BlendState

	D3D12_BLEND_DESC blendDesc{};
	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

#pragma endregion

#pragma region RasterizerState
	D3D12_RASTERIZER_DESC rastarizerDesc{};
	// 裏面(反時計回り)をカリング
	rastarizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 塗りつぶしモードをソリッドにする
	//(ワイヤーフレームなら D3D12_FILL_MODE_WiREFRAME)
	rastarizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#pragma endregion

#pragma region VertexShaderをコンパイルする

	Shader vs;
	vs.LoadDxc(L"Resources/shaders/TestVS.hlsl", L"vs_6_0");
	assert(vs.GetDxcBlob() != nullptr);

#pragma endregion

#pragma region PixelShaderをコンパイルする

	Shader ps;
	ps.LoadDxc(L"Resources/shaders/TestPS.hlsl", L"ps_6_0");
	assert(ps.GetDxcBlob() != nullptr);

#pragma endregion

#pragma region PSO(PixelShaderObject)の生成

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPiplineStateDesc{};
	graphicPiplineStateDesc.pRootSignature = rootSignature;                                               // RootSignature
	graphicPiplineStateDesc.InputLayout = inputLayoutDesc;                                                // InputLayout
	graphicPiplineStateDesc.VS = {vs.GetDxcBlob()->GetBufferPointer(), vs.GetDxcBlob()->GetBufferSize()}; // VertexShader
	graphicPiplineStateDesc.PS = {ps.GetDxcBlob()->GetBufferPointer(), ps.GetDxcBlob()->GetBufferSize()}; // PixelShader
	graphicPiplineStateDesc.BlendState = blendDesc;                                                       // BlendState
	graphicPiplineStateDesc.RasterizerState = rastarizerDesc;                                             // Rasterizer

	// 書き込むRTVの情報
	// 1つのRTVに書き込む(2つ同時も可能)
	graphicPiplineStateDesc.NumRenderTargets = 1;
	graphicPiplineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジー(形状)のタイプ 三角形
	//         []
	//        [][]
	//       [][][]
	//      [][][][]
	//     [][][][][]
	//    [][][][][][]
	//   [][][][][][][]
	//  [][][][][][][][]
	// [][][][][][][][][]
	//[][][][][][][][][][]
	graphicPiplineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むのかの設定
	graphicPiplineStateDesc.SampleDesc.Count = 1;
	graphicPiplineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// 準備完了、POSを生成
	ID3D12PipelineState* graphicPiplineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicPiplineStateDesc, IID_PPV_ARGS(&graphicPiplineState));
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region VertexResourceを生成する

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	// CPUから書き込むヒープ
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファ
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	// リソースのサイズ(三角形->Vector4を3頂点分)
	vertexResourceDesc.Width = sizeof(Vector4) * 3;
	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点リソースを生成する
	ID3D12Resource* vertexResource = nullptr;
	hr = dxCommon->GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region VertexBufferViewを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
	// 1つの頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4);
#pragma endregion

#pragma region 頂点リソースにデータを書き込む

	Vector4* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 左下
	vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
	// 上
	vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};
	// 右下
	vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};
	// 頂点リソースのマップを解除する
	vertexResource->Unmap(0, nullptr);

#pragma endregion

#pragma endregion

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// 描画開始
		dxCommon->PreDraw();

#pragma region 三角形の描画
		// コマンドを積む
		// RootSignatureの設定
		commandList->SetGraphicsRootSignature(rootSignature);
		// PSOの設定
		commandList->SetPipelineState(graphicPiplineState);
		// VBVの設定
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		// トポロジーの設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// 頂点数、インデックス数、インデックスの開始位置、インデックスのオフセット
		commandList->DrawInstanced(3, 1, 0, 0);

#pragma endregion

		// 描画終了
		dxCommon->PostDraw();
	}

#pragma region 解放処理
	//->Release();
	vertexResource->Release();
	graphicPiplineState->Release();
	signatureBlob->Release();

#pragma endregion

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}