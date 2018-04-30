
struct VSInputParticle1
{
	float3 Position  : POSITION0;
	float Angle      : ANGLE0;
	float Size       : PSIZE0;
};

struct VSInputParticle2
{
	float3 Position   : POSITION0;
	float3 Velocity   : VELOCITY0;
	float Angle       : ANGLE0;
	float DeltaAngle  : DANGLE0;
	float Size        : PSIZE0;
	//uint Life         : LIFE0;
	uint4 RandomSeed  : RANDOM0;
};


struct VSOutputParticle
{
	float4 Position : SV_Position;
	float Angle     : ANGLE;
	float Size      : PSIZE;
	float4 Color    : COLOR;
};

struct GSOutputParticle
{
	float4 Position : SV_Position;
	float4 Color    : COLOR;
	float2 TexCoord : TEXCOORD;
};
