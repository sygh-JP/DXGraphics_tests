#include "MyConstBuffers.hlsli"
#include "MyVertexTypes.hlsli"
#include "RandomNumHelper.hlsli"
#include "CommonDefs.hlsli"


RWStructuredBuffer<VSInputParticle2> UniParticleBuffer : register(u0);


void ResetParticle(uint globalIndex)
{
	VSInputParticle2 inoutData = UniParticleBuffer[globalIndex];

	const uint4 random0 = inoutData.RandomSeed;
	const uint4 random1 = Xorshift128Random::CreateNext(random0);
	const uint4 random2 = Xorshift128Random::CreateNext(random1);
	const uint4 random3 = Xorshift128Random::CreateNext(random2);
	const uint4 random4 = Xorshift128Random::CreateNext(random3);
	inoutData.Position = float3(0,0,0);
	inoutData.Velocity = UniMaxParticleVelocity * float3(
		Xorshift128Random::GetRandomComponentSF(random0),
		Xorshift128Random::GetRandomComponentUF(random1),
		Xorshift128Random::GetRandomComponentSF(random2));
	inoutData.Angle = 0;
	inoutData.DeltaAngle = UniMaxAngularVelocity * Xorshift128Random::GetRandomComponentSF(random3);
	inoutData.Position += inoutData.Velocity;
	inoutData.Velocity.y -= UniGravity;
	inoutData.Angle += inoutData.DeltaAngle;
	inoutData.Size = UniParticleInitSize;
	inoutData.RandomSeed = random4;
	// This random number will be used on next initialization.

	UniParticleBuffer[globalIndex] = inoutData;
}

void UpdateParticle(uint globalIndex)
{
	VSInputParticle2 inoutData = UniParticleBuffer[globalIndex];

	inoutData.Position += inoutData.Velocity;
	inoutData.Velocity.y -= UniGravity;
	inoutData.Angle += inoutData.DeltaAngle;
	inoutData.Size = UniParticleInitSize;

	UniParticleBuffer[globalIndex] = inoutData;
}


[numthreads(MY_CS_LOCAL_GROUP_SIZE, 1, 1)]
void main(uint3 did : SV_DispatchThreadID)
{
	[unroll]
	for (uint i = 0; i < MY_CS_PARTICLES_NUM_PER_THREAD; ++i)
	{
		const uint globalIndex = MY_CS_PARTICLES_NUM_PER_THREAD * did.x + i;

		if (globalIndex == UniSpawnTargetParticleIndex)
		{
			// Reset the particle.
			ResetParticle(globalIndex);
		}
		else if (globalIndex < UniMaxParticleCount)
		{
			// Update the particle.
			UpdateParticle(globalIndex);
		}
	}
}
