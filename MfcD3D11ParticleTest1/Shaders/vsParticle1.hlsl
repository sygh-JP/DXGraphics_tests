// For CPU-based particles.

#include "MyConstBuffers.hlsli"
#include "MyVertexTypes.hlsli"


VSOutputParticle main(VSInputParticle1 inVert)
{
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
