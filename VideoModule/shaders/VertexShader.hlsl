struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(float4 Pos : POSITION, float2 TexCoord : TEXCOORD0)
{
    VS_OUTPUT output;
    output.Pos = Pos;
    output.TexCoord = TexCoord;
    return output;
}