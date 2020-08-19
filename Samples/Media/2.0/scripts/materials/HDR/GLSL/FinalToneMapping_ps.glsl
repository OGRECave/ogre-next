#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

//See Hable_John_Uncharted2_HDRLighting.pptx
//See http://filmicgames.com/archives/75
//See https://expf.wordpress.com/2010/05/04/reinhards_tone_mapping_operator/
/*const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;
const float W = 11.2;*/
const float A = 0.22;
const float B = 0.3;
const float C = 0.10;
const float D = 0.20;
const float E = 0.01;
const float F = 0.30;
const float W = 11.2;

vec3 FilmicTonemap( vec3 x )
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
float FilmicTonemap( float x )
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 fromSRGB( vec3 x )
{
	return x * x;
}

vulkan_layout( ogre_t0 ) uniform texture2D rt0;
vulkan_layout( ogre_t1 ) uniform texture2D lumRt;
vulkan_layout( ogre_t2 ) uniform texture2D bloomRt;

vulkan( layout( ogre_s0 ) uniform sampler samplerPoint );
vulkan( layout( ogre_s2 ) uniform sampler samplerBilinear );

void main()
{
	float fInvLumAvg = texture( vkSampler2D( lumRt, samplerPoint ), vec2( 0.0, 0.0 ) ).x;

	vec4 vSample = texture( vkSampler2D( rt0, samplerPoint ), inPs.uv0 );

	vSample.xyz *= fInvLumAvg;
	vSample.xyz	+= fromSRGB( texture( vkSampler2D( bloomRt, samplerBilinear ),
									  inPs.uv0 ).xyz ) * 16.0;
	vSample.xyz  = FilmicTonemap( vSample.xyz ) / FilmicTonemap( W );
	//vSample.xyz  = vSample.xyz / (1 + vSample.xyz); //Reinhard Simple
	vSample.xyz  = ( vSample.xyz - 0.5 ) * 1.25 + 0.5 + 0.11;

	fragColour = vSample;
}
