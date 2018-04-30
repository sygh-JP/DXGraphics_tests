#include "MyConstBuffers.hlsli"
#include "MyVertexTypes.hlsli"
#include "RandomNumHelper.hlsli"
#include "CommonDefs.hlsli"


RWByteAddressBuffer UniParticleBuffer : register(u0);


#define SIZEOF_FLOAT  (4)
// cf. VSInputParticle2.
#define VERTEX_ELEM_SIZE  (SIZEOF_FLOAT * 13)
#define POSITION_OFFSET   (SIZEOF_FLOAT * 0)
#define VELOCITY_OFFSET   (SIZEOF_FLOAT * 3)
#define ANGLE_OFFSET      (SIZEOF_FLOAT * 6)
#define DANGLE_OFFSET     (SIZEOF_FLOAT * 7)
#define PSIZE_OFFSET      (SIZEOF_FLOAT * 8)
#define RANDOM_OFFSET     (SIZEOF_FLOAT * 9)

void ResetParticle(uint globalIndex)
{
	const uint vbBaseOffset = globalIndex * VERTEX_ELEM_SIZE;

	const uint4 random0 = UniParticleBuffer.Load4(vbBaseOffset + RANDOM_OFFSET);
	const uint4 random1 = Xorshift128Random::CreateNext(random0);
	const uint4 random2 = Xorshift128Random::CreateNext(random1);
	const uint4 random3 = Xorshift128Random::CreateNext(random2);
	const uint4 random4 = Xorshift128Random::CreateNext(random3);
	float3 position = float3(0,0,0);
	float3 velocity = UniMaxParticleVelocity * float3(
		Xorshift128Random::GetRandomComponentSF(random0),
		Xorshift128Random::GetRandomComponentUF(random1),
		Xorshift128Random::GetRandomComponentSF(random2));
	float angle = 0;
	const float deltaAngle = UniMaxAngularVelocity * Xorshift128Random::GetRandomComponentSF(random3);
	position += velocity;
	velocity.y -= UniGravity;
	angle += deltaAngle;
	UniParticleBuffer.Store3(vbBaseOffset + POSITION_OFFSET, asuint(position));
	UniParticleBuffer.Store3(vbBaseOffset + VELOCITY_OFFSET, asuint(velocity));
	UniParticleBuffer.Store(vbBaseOffset + ANGLE_OFFSET, asuint(angle));
	UniParticleBuffer.Store(vbBaseOffset + DANGLE_OFFSET, asuint(deltaAngle));
	UniParticleBuffer.Store(vbBaseOffset + PSIZE_OFFSET, asuint(UniParticleInitSize));
	UniParticleBuffer.Store4(vbBaseOffset + RANDOM_OFFSET, random4);
	// This random number will be used on next initialization.
}

void UpdateParticle(uint globalIndex)
{
	const uint vbBaseOffset = globalIndex * VERTEX_ELEM_SIZE;

	float3 position = asfloat(UniParticleBuffer.Load3(vbBaseOffset + POSITION_OFFSET));
	float3 velocity = asfloat(UniParticleBuffer.Load3(vbBaseOffset + VELOCITY_OFFSET));
	float angle = asfloat(UniParticleBuffer.Load(vbBaseOffset + ANGLE_OFFSET));
	const float deltaAngle = asfloat(UniParticleBuffer.Load(vbBaseOffset + DANGLE_OFFSET));
	position += velocity;
	velocity.y -= UniGravity;
	angle += deltaAngle;
	UniParticleBuffer.Store3(vbBaseOffset + POSITION_OFFSET, asuint(position));
	UniParticleBuffer.Store3(vbBaseOffset + VELOCITY_OFFSET, asuint(velocity));
	UniParticleBuffer.Store(vbBaseOffset + ANGLE_OFFSET, asuint(angle));
	UniParticleBuffer.Store(vbBaseOffset + PSIZE_OFFSET, asuint(UniParticleInitSize));
	// The original program stored delta-angle instead of angle (Maybe just a bug)
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
