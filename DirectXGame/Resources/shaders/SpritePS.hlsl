/*
float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
*/

// Test.hlsli (—á)
typedef float2 float32_t2;
typedef float4 float32_t4;

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};


struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t2 uv = input.texcoord.xy;
    
    output.color = float32_t4(uv.x, uv.y, 0.0f, 1.0f);
    return output;
}