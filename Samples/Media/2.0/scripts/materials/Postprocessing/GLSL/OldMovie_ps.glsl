#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D SplotchesTx;
vulkan_layout( ogre_t2 ) uniform texture1D Texture2;
vulkan_layout( ogre_t3 ) uniform texture1D SepiaTx;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );
vulkan( layout( ogre_s1 ) uniform sampler samplerState1 );
vulkan( layout( ogre_s2 ) uniform sampler samplerState2 );
vulkan( layout( ogre_s3 ) uniform sampler samplerState3 );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float time_cycle_period;
	uniform float flicker;
	uniform float DirtFrequency;
	uniform vec3 luminance;
	uniform float frameJitter;
	uniform float lumiShift;
vulkan( }; )

vec2 calcSpriteAddr(vec2 texCoord, float DirtFrequency1, float period)
{
   return texCoord + texture( vkSampler1D( Texture2, samplerState2 ), period * DirtFrequency1 ).xy;
}

float getSplotches(vec2 spriteAddr)
{
   // get sprite address into paged texture coords space
   spriteAddr = spriteAddr / 6.3;
   spriteAddr = spriteAddr - (spriteAddr / 33.3);

   return texture( vkSampler2D( SplotchesTx, samplerState1 ), spriteAddr ).x;
}

void main()
{
   // get sprite address
   vec2 spriteAddr = calcSpriteAddr(inPs.uv0, DirtFrequency, time_cycle_period);

   // add some dark and light splotches to the film
   float splotches = getSplotches(spriteAddr);
   float specs = 1.0 - getSplotches(spriteAddr / 3.0);

   // convert color to base luminance
   vec4 base = texture( vkSampler2D( RT, samplerState0 ),
						inPs.uv0 + vec2(0.0, spriteAddr.y * frameJitter) );
   float lumi = dot(base.rgb, luminance);
   // randomly shift luminance
   lumi -= spriteAddr.x * lumiShift;
   // tone map luminance
   base.rgb = texture( vkSampler1D( SepiaTx, samplerState3 ), lumi ).rgb;

   // calc flicker speed
   float darken = fract(flicker * time_cycle_period);

   // we want darken to cycle between 0.6 and 1.0
   darken = abs(darken - 0.5) * 0.4 + 0.6;
   // composite dirt onto film
   fragColour = base * splotches * darken + specs;
}
