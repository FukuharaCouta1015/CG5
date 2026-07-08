/*
float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}
*/

#include "Test.hlsli"

/*
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};
*/


struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t4 texcoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = input.position;
    output.texcoord = input.texcoord;
    return output;

}