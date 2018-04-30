// For GPU-based particles.

#include "MyConstBuffers.hlsli"
#include "MyVertexTypes.hlsli"
#include "CommonDefs.hlsli"


StructuredBuffer<VSInputParticle2> UniParticleBuffer : register(t1);


#ifdef USE_INSTANCE_ID_FOR_PARTICLE_DRAW
VSOutputParticle main(uint instanceId : SV_InstanceID)
{
	VSInputParticle2 inVert = UniParticleBuffer[instanceId];
#else
VSOutputParticle main(uint vertexId : SV_VertexID)
{
	VSInputParticle2 inVert = UniParticleBuffer[vertexId];
#endif

	VSOutputParticle outVert;

	outVert.Position = mul(float4(inVert.Position, 1), UniMatrixView);
	outVert.Position /= outVert.Position.w;
	outVert.Angle = inVert.Angle;
	outVert.Size = inVert.Size;
	//outVert.Color = float4(0.9, 0.1, 0.2, 1);
	outVert.Color = float4(1.0, 0.9, 0.0, 1);
	//outVert.Color = float4(1.0, 1.0, 0.0, 1);

	return outVert;
}
