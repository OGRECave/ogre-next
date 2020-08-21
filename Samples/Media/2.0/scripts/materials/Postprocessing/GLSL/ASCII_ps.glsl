#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture3D chars;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s1 ) uniform sampler samplerState1 );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec2 numTiles;
	uniform vec2 iNumTiles;
	uniform vec2 iNumTiles2;
	uniform vec4 lum;
	uniform float charBias;
vulkan( }; )

void main()
{
    vec3 local;

	//sample RT
	local.xy = mod(inPs.uv0, iNumTiles);
	vec2 middle = inPs.uv0 - local.xy;
	local.xy = local.xy * numTiles;
	
	//iNumTiles2 = iNumTiles / 2
	middle = middle + iNumTiles2;
	vec4 c = texture( vkSampler2D( RT, samplerState0 ), middle );
	
	//multiply luminance by charbias , beacause not all slices of the ascii
	//volume texture are used
	local.z = dot(c , lum)*charBias;
	
	//fix to brighten the dark pixels with small characters
	//c *= lerp(2.0,1.0, local.z);
	
	c *= texture( vkSampler3D( chars, samplerState1 ), local );
	fragColour = c;
}
