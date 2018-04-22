#include "MyVertex.hlsli"
#include "MyConstBuffer.hlsli"

MyVSOut main(float3 pos : POSITION, float4 color : COLOR, uint instanceId : SV_InstanceID)
{
	MyVSOut outVal = (MyVSOut)0;
	outVal.Position = float4(pos, 1);
	outVal.Position = mul(mul(outVal.Position, MyConstBuffer_UniWorld[instanceId]), MyConstBuffer_UniProjection);
	outVal.Position /= outVal.Position.w;
	outVal.Color = color;
	//outVal.Color.r = MyConstBuffer_UniWorld[0][0];
	return outVal;
}
