TextureCube skyMap : register(t[0]);

SamplerState samLinear : register(s[0]);

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_Position;
	float3 texCoord : TEXCOORD;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	return skyMap.Sample(samLinear, normalize(input.texCoord));
}
