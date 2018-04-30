cbuffer vcbSceneParam : register(b0)
{
	float4x4 UniMatrixView : packoffset(c0);
	float4x4 UniMatrixProj : packoffset(c4);
	float UniNear                    : packoffset(c8.x);
	float UniFar                     : packoffset(c8.y);
	float UniGravity                 : packoffset(c8.z);
	uint UniSpawnTargetParticleIndex : packoffset(c8.w);
	float3 UniMaxParticleVelocity : packoffset(c9);
	float UniMaxAngularVelocity   : packoffset(c9.w); // [Radian]
	uint UniMaxParticleCount  : packoffset(c10.x);
	float UniParticleInitSize : packoffset(c10.y);
};

Texture2D UniParticleSpriteTex : register(t0);
SamplerState UniSamplerLinearWrap : register(s0);

