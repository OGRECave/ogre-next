
//#include "SyntaxHighlightingMisc.h"

@property( hlms_particle_system )
@piece( ParticleSystemDeclVS )
	, device const uint4 *particleSystemGpuData [[buffer(TEX_SLOT_START+@value(particleSystemGpuData))]]
@end
@end
