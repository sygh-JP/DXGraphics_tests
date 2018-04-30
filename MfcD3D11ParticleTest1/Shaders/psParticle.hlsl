#include "MyConstBuffers.hlsli"
#include "MyVertexTypes.hlsli"


float4 main(GSOutputParticle psIn) : SV_Target
{
	// Compose particle color and mask image texture.
	return float4(psIn.Color.rgb, UniParticleSpriteTex.Sample(UniSamplerLinearWrap, psIn.TexCoord).a);
	//return psIn.Color;
}
