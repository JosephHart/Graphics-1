
//
// Model a simple light
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer basicCBuffer : register(b0) {

	float4x4			worldViewProjMatrix;
	float4x4			worldITMatrix; // Correctly transform normals to world space
	float4x4			worldMatrix;
	float4				eyePos;
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
	float4				lightVec2; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient2;
	float4				lightDiffuse2;
	float4				lightSpecular2;
	float4				lightVec3; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient3;
	float4				lightDiffuse3;
	float4				lightSpecular3;
	float4				windDir;
	float				Timer;
	float				grassHeight;
};


//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D myTexture : register(t0);
SamplerState linearSampler : register(s0);




//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {

	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float4				matDiffuse		: DIFFUSE; // a represents alpha.
	float4				matSpecular		: SPECULAR; // a represents specular power. 
	float2				texCoord		: TEXCOORD;
	float4				posH			: SV_POSITION;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) { 

	FragmentOutputPacket outputFragment;


	float3 N = normalize(v.normalW);

	float4 baseColour = v.matDiffuse;

	baseColour = baseColour * myTexture.Sample(linearSampler, v.texCoord);

	//Initialise returned colour to ambient component
	float3 colour = baseColour.xyz* (lightAmbient + lightAmbient2 + lightAmbient3);

	// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	float3 lightDir = -lightVec.xyz; // Directional light
	if (lightVec.w == 1.0) lightDir =lightVec.xyz - v.posW; // Positional light
	lightDir=normalize(lightDir);

	float3 lightDir2 = -lightVec2.xyz; // Directional light
	if (lightVec2.w == 1.0) lightDir2 = lightVec2.xyz - v.posW; // Positional light
	lightDir2 = normalize(lightDir2);

	float3 lightDir3 = -lightVec3.xyz; // Directional light
	if (lightVec3.w == 1.0) lightDir3 = lightVec3.xyz - v.posW; // Positional light
	lightDir3 = normalize(lightDir3);

	// Add diffuse light if relevant (otherwise we end up just returning the ambient light colour)
	colour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lightDiffuse;
	colour += max(dot(lightDir2, N), 0.0f) * baseColour.xyz * lightDiffuse2;
	colour += max(dot(lightDir3, N), 0.0f) * baseColour.xyz * lightDiffuse3;

	// Calc specular light
	float specPower = max(v.matSpecular.a*1000.0, 1.0f);

	float3 eyeDir = normalize(eyePos - v.posW);
	float3 R = reflect(-lightDir,N );
	float3 R2 = reflect(-lightDir2, N);
	float3 R3 = reflect(-lightDir3, N);
	float specFactor = pow(max(dot(R, eyeDir), 0.0f), specPower);
	float specFactor2 = pow(max(dot(R2, eyeDir), 0.0f), specPower);
	float specFactor3 = pow(max(dot(R3, eyeDir), 0.0f), specPower);
	colour += specFactor * v.matSpecular.xyz * lightSpecular;
	colour += specFactor2 * v.matSpecular.xyz * lightSpecular2;
	colour += specFactor3 * v.matSpecular.xyz * lightSpecular3;


	outputFragment.fragmentColour = float4(colour,  baseColour.a);
	return outputFragment;

}
