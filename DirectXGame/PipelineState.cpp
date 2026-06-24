#include "PipelineState.h"

#include "KamataEngine.h"

using namespace KamataEngine;

void PipelineState::Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStatedesc) {
	// クラス内で取得する為に追加
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	ID3D12PipelineState* graphicPipelineState = nullptr;
	HRESULT hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStatedesc, IID_PPV_ARGS(&graphicPipelineState));
	assert(SUCCEEDED(hr));

	// 生成したPipelineStateを取っておく
	pipelineState_ = graphicPipelineState;
}

ID3D12PipelineState* PipelineState::Get() { return pipelineState_; }

PipelineState::PipelineState() {}

PipelineState::~PipelineState() {
	if (pipelineState_) {
		pipelineState_->Release();
		pipelineState_ = nullptr;
	}
}