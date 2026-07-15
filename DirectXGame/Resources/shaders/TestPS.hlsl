/*
float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
*/

#include "Test.hlsli"



Texture2D<float32_t4> gTexture : register(t0); //SRV     register => t
SamplerState fSampler : register(s0); //Sampler register => s

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t2 uv = input.texcoord.xy;
    float32_t4 textureColor = gTexture.Sample(fSampler, uv);
    
    //output.color = float32_t4(uv.x, uv.y, 0.0f, 1.0f);
    
    //output.color = float32_t4(1.0f, 0.0f, 0.0f, 1.0f);
    
    //output.color.a = 1.0f;
    
    
    
    
    //カラー
    //output.color = textureColor;
    
    //グレースケール
    float32_t value = dot(textureColor.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
    output.color = float32_t4(value, value, value, textureColor.a);
    
    
   
    return output;
}