Texture2D shaderTexture;
SamplerState samplerState;

float4 main(float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    return shaderTexture.Sample(samplerState, TexCoord);
}