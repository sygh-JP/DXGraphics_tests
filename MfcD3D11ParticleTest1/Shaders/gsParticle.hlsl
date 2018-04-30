#include "MyConstBuffers.hlsli"
#include "MyVertexTypes.hlsli"


[maxvertexcount(4)]
void main(point VSOutputParticle gsIn[1], inout TriangleStream<GSOutputParticle> outStream)
{
	if ((UniNear <= -gsIn[0].Position.z) && (-gsIn[0].Position.z <= UniFar))
	{
		GSOutputParticle gsOut;

		float ss, cs;
		sincos(gsIn[0].Angle, ss, cs);

		float4 coord[4];
		const float size = gsIn[0].Size;
		coord[0] = gsIn[0].Position + size * float4(-cs - ss, +cs - ss, 0.0, 0.0);
		coord[1] = gsIn[0].Position + size * float4(+cs - ss, +cs + ss, 0.0, 0.0);
		coord[2] = gsIn[0].Position + size * float4(-cs + ss, -cs - ss, 0.0, 0.0);
		coord[3] = gsIn[0].Position + size * float4(+cs + ss, -cs + ss, 0.0, 0.0);

		const float2 texCoord[4] =
		{
			{ 0.0, 0.0 },
			{ 1.0, 0.0 },
			{ 0.0, 1.0 },
			{ 1.0, 1.0 },
		};

		[unroll]
		for (int v = 0; v < 4; ++v)
		{
			gsOut.Position = mul(coord[v], UniMatrixProj);
			gsOut.TexCoord = texCoord[v];
			gsOut.Color = gsIn[0].Color;

			outStream.Append(gsOut);
		}
		outStream.RestartStrip();
	}
}
