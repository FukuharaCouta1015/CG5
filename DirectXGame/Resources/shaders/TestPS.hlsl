//float4 main( float4 pos : POSITION ) : SV_POSITION
//{
//	return pos;
//}

struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

PixelShaderOutput main()
{
    PixelShaderOutput output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}

