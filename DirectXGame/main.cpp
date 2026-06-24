#include "KamataEngine.h"
#include <Windows.h>

#include "PipelineState.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexBuffer.h"

using namespace KamataEngine;

// 関数プロトタイプ宣言
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps);

void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
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

	// VertexResource,VertexBufferViewを生成する
	VertexBuffer vb;
	vb.Create(sizeof(Vector4) * 3, sizeof(Vector4));

#pragma region 頂点リソースにデータを書き込む

	Vector4* vertexData = nullptr;
	vb.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 左下
	vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
	// 上
	vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};
	// 右下
	vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};
	// 頂点リソースのマップを解除する
	// vertexResource->Unmap(0, nullptr);

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
		commandList->SetGraphicsRootSignature(rs.Get());
		// PSOの設定
		commandList->SetPipelineState(pipelineState.Get());
		// VBVの設定
		commandList->IASetVertexBuffers(0, 1, vb.GetView());
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
	// vertexResource->Release();
	// graphicPiplineState->Release();

#pragma endregion

	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}