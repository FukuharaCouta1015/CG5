#include "KamataEngine.h"
#include <Windows.h>

#include "IndexBuffer.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexBuffer.h"

using namespace KamataEngine;

// 関数プロトタイプ宣言
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
#pragma region InputLayout

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

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

#pragma region PSO(PixelShaderObject)の生成

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPiplineStateDesc{};
	graphicPiplineStateDesc.pRootSignature = rs.Get();                                                    // RootSignature
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

	// 準備完了、PSOを生成
	pipelineState.Create(graphicPiplineStateDesc);
#pragma endregion
}

// RenderTextureResourceの生成
ID3D12Resource* CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT clearFormat, const FLOAT* clearColor) {
	// 1.生成するenderTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(width);                             // REnderTextureの幅
	resourceDesc.Height = UINT(height);                           // textureの高さ
	resourceDesc.MipLevels = 1;                                   // mipmapの数
	resourceDesc.DepthOrArraySize = 1;                            // 奥行 or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;        // TextureのFormat
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント 1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // Textureの次元数(普段は2次元)
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RenderTargetとして使う通知

	// 2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	// VRAM上に作る
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 3.ClearValueの用意
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = clearFormat;
	clearValue.Color[0] = clearColor[0];
	clearValue.Color[1] = clearColor[1];
	clearValue.Color[2] = clearColor[2];
	clearValue.Color[3] = clearColor[3];

	// 4.RenderTextureResourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                            // Heapの設定
	    D3D12_HEAP_FLAG_NONE,                       // Heapの特殊な設定
	    &resourceDesc,                              // Resourceの設定
	    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // PixelShaderでアクセスできるようにする
	    &clearValue,                                // Clear最適値
	    IID_PPV_ARGS(&resource)                     // 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

	return resource;
}

// DepthStencilTextureResourceの生成
ID3D12Resource* CreateDepthStencilTextureRecource(ID3D12Device* device, int32_t width, int32_t height) {
	// 1.生成するDepthStencilTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;                  // Textureの幅
	resourceDesc.Height = height;                // Textureの高さ
	resourceDesc.MipLevels = 1;                  // mipmapの数、DepthStencilなので、1つでいい
	resourceDesc.DepthOrArraySize = 1;           // Textureの配列数　DepthStencilは、1つでいい
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT; // DepthStencilとして利用可能なフォーマット
	                                             // ※KamataEngineと合わせる
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント 1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	// 3.Resourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                  // Heapの設定
	    D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定
	    &resourceDesc,                    // Resourceの設定
	    D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込み状態にしておく
	    &depthClearValue,                 // Clear最適値
	    IID_PPV_ARGS(&resource)           // 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

	return resource;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// エンジンの初期化
	KamataEngine::Initialize(L"LE3D_21_フクハラコウタ");

#pragma region DirectXCommonの管理の取得

	// DirectXCommonインスタンスを取得する
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// DirectXCommonクラスが管理している、ウインドウの高さと幅の値の取得
	int32_t w = dxCommon->GetBackBufferWidth();
	int32_t h = dxCommon->GetBackBufferHeight();
	DebugText::GetInstance()->ConsolePrintf(std::format("width: {}, hieght: {}\n", w, h).c_str());

	// DirectXCommonクラスが管理している、コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

#pragma endregion

#pragma region レンダリングパイプライン(main.cpp-WinMain)

	// RootSignature作成
	RootSignature rs;
	rs.Create();

	// VertexShaderをコンパイルする
	Shader vs;
	vs.LoadDxc(L"Resources/shaders/TestVS.hlsl", L"vs_6_0");
	assert(vs.GetDxcBlob() != nullptr);

	// PixelShaderをコンパイルする
	Shader ps;
	ps.LoadDxc(L"Resources/shaders/TestPS.hlsl", L"ps_6_0");
	assert(ps.GetDxcBlob() != nullptr);

	// PipelineStateの作成
	PipelineState pipelineState;
	SetupPipelineState(pipelineState, rs, vs, ps);

#pragma endregion

#pragma region 頂点リソースにデータを書き込む

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	VertexData vertices[] = {
	    {{-1.0f, 1.0f, 0.0f, 1.0f},  {0.0f, 0.0f}}, // 0: 左上
	    {{1.0f, 1.0f, 0.0f, 1.0f},   {1.0f, 0.0f}}, // 1: 右上
	    {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 2: 左下
	    {{1.0f, -1.0f, 0.0f, 1.0f},  {1.0f, 1.0f}}, // 3: 右下
	};

	// VertexResource,VertexBufferViewを生成する
	VertexBuffer vb;
	vb.Create(sizeof(vertices) * 3, sizeof(vertices[0]));
	VertexData* pGpuVertices = nullptr;
	vb.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuVertices));
	for (int i = 0; i < _countof(vertices); ++i) {
		pGpuVertices[i] = vertices[i];
	}

	// 頂点インデックスデータの準備
	uint16_t indices[] = {0, 1, 2, 1, 3, 2};

	IndexBuffer ib;
	ib.Create(sizeof(indices), sizeof(indices[0]));
	// 頂点インデックスにデータを書き込む
	uint16_t* pGpuIndices = nullptr;
	ib.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuIndices));
	for (int i = 0; i < _countof(indices); ++i) {
		pGpuIndices[i] = indices[i];
	}

#pragma endregion

#pragma region レンダーターゲットリソース

	ID3D12Device* device = dxCommon->GetDevice();
	HRESULT hr;

	const FLOAT kRenderTargetClearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

	ID3D12Resource* renderTextureResource = CreateRenderTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearColor);

	// 1.RTV用のDescriptorHeapを作成する
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NumDescriptors = 1;

	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側から見たHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2.RTV用のViewの生成
	device->CreateRenderTargetView(
	    renderTextureResource, // Viewと関連付けたリソース
	    nullptr,               // RTVの詳細情報(Desc:Description、構成内容の記述)
	                           // ※RTVの場合、nullptrにするとDirectX12が自動で推測してくれる
	    rtvHandleCPU           // RTVディスクリプタヒープの CPU Habdle
	);

#pragma endregion

#pragma region デプスステンシル・リソース・ヒープ・ビュー

	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureRecource(device, WinApp::kWindowWidth, WinApp::kWindowHeight);
	// DSV用のDescriptorHeapの作成
	ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHEapDesc{};
	dsvDescriptorHEapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;   // Heap Type
	dsvDescriptorHEapDesc.NumDescriptors = 1;                      // Heap Typeの個数
	dsvDescriptorHEapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // DSVはShaderで触らないとする

	hr = device->CreateDescriptorHeap(&dsvDescriptorHEapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側から見たHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleCPU = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2.DSV用のViewの生成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;                // 基本的にResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D Texture

	// DSVHeapの先頭に、DSVを作る
	device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvHandleCPU);

#pragma endregion

#pragma region SRV・ヒープ・ビュー

	// 1.SRV用のDescriptorHeapの作成
	ID3D12DescriptorHeap* srvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
	srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;     // SRV
	srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // PixelShaderから見える
	srvDescriptorHeapDesc.NumDescriptors = 1;

	hr = device->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&srvDescriptorHeap));
	assert(SUCCEEDED(hr));

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// 2.SRV(Shader Resource View)の作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;                           // RenderTargetResourceと同じにする
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBA値をそのまま Shaderに対応させる
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;                                            // MipLevelは1しかない

	device->CreateShaderResourceView(
	    renderTextureResource, // Viewと関連付けたいリソース
	    &srvDesc,              // SRVの詳細情報(Desc:Description、構成内容の記述)
	    srvHandleCPU           // SRV用ディスクリプタヒープの CPU Handle
	);

#pragma endregion

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

#pragma region リソースバリアSRVからRTV

		// TransitionBarrierをSRVからRTVに設定する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;                       // TransitionBarrierの設定
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;                            // フラグはNoneにしておく
		barrier.Transition.pResource = renderTextureResource;                        // バリアを張る対象のリソース
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // 遷移前
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;          // 遷移後
		commandList->ResourceBarrier(1, &barrier);                                   // バリアを張る

		// 描画用のRTVとDSVを設定する
		commandList->OMSetRenderTargets(1, &rtvHandleCPU, false, &dsvHandleCPU);

#pragma endregion

#pragma region ビューポート

		// Viewportの設定
		D3D12_VIEWPORT viewport{};
		viewport.Width = WinApp::kWindowWidth;
		viewport.Height = WinApp::kWindowHeight;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f; // 深度の最小値
		viewport.MaxDepth = 1.0f; // 深度の最大値

		commandList->RSSetViewports(1, &viewport);
#pragma endregion

#pragma region シザリング矩形
		// Scissorの設定
		D3D12_RECT scissorRect{};
		scissorRect.left = 0;
		scissorRect.right = WinApp::kWindowWidth;
		scissorRect.top = 0;
		scissorRect.bottom = WinApp::kWindowHeight;

		commandList->RSSetScissorRects(1, &scissorRect);
#pragma endregion

		// 全画面クリア
		commandList->ClearRenderTargetView(rtvHandleCPU, kRenderTargetClearColor, 0, nullptr);
		// 指定した深度で画面全体をクリアする
		commandList->ClearDepthStencilView(dsvHandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

#pragma region 3Dシーン

#pragma endregion

#pragma region リソースバリアRTVとSRV

		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;                      // TrabsitionBarrierの設定
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;                           // フラグはNoneにしておく
		barrier.Transition.pResource = renderTextureResource;                       // バリアを張る対象のリソース
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;        // 遷移前
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // 遷移後
		commandList->ResourceBarrier(1, &barrier);                                  // バリアを張る

#pragma endregion

		// 描画開始
		dxCommon->PreDraw();
#pragma region コマンド
		// コマンドを積む
		// RootSignatureの設定
		commandList->SetGraphicsRootSignature(rs.Get());
		// PSOの設定
		commandList->SetPipelineState(pipelineState.Get());

		// VBVの設定
		commandList->IASetVertexBuffers(0, 1, vb.GetView());

		commandList->IASetIndexBuffer(ib.GetView());
		// トポロジーの設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 使用するディスクリプタヒープの設定
		commandList->SetDescriptorHeaps(srvDescriptorHeap->GetDesc().NumDescriptors, &srvDescriptorHeap);
		// SRVのDescriptorTableの先頭を設定
		commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU);

#pragma endregion

#pragma region 三角形の描画
		// 頂点数、インデックス数、インデックスの開始位置、インデックスのオフセット
		// commandList->DrawInstanced(6, 1, 0, 0);

		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);

#pragma endregion

		// 描画終了
		dxCommon->PostDraw();
	}

#pragma region 解放処理

	renderTextureResource->Release();
	srvDescriptorHeap->Release();
	rtvDescriptorHeap->Release();

	depthStencilResource->Release();
	dsvDescriptorHeap->Release();

#pragma endregion

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}