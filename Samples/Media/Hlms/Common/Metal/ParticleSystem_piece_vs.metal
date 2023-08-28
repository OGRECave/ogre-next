
//#include "SyntaxHighlightingMisc.h"

@property( hlms_particle_system )
@piece( ParticleSystemDeclVS )
	, device const uint4 *particleSystemGpuData [[buffer(TEX_SLOT_START+@value(particleSystemGpuData))]]
	, constant GpuParticleCommon &particleCommon [[buffer(CONST_SLOT_START+@value(particleSystemConstSlot))]]
@end
@end
