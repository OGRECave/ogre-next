#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec2 numTiles;
	uniform vec2 iNumTiles;
	uniform vec2 iNumTiles2;
	uniform vec4 lum;
vulkan( }; )

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture3D noise;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s1 ) uniform sampler samplerState1 );

void main()
{
	vec3 local;
	local.xy = mod(inPs.uv0, iNumTiles);
	vec2 middle = inPs.uv0 - local.xy;
	local.xy = local.xy * numTiles;
	middle +=  iNumTiles2;
	local.z = dot(texture( vkSampler2D( RT, samplerState0 ), middle ), lum);
	vec4 c = vec4(texture( vkSampler3D( noise, samplerState1 ), local ).r);
	fragColour = c;
}
