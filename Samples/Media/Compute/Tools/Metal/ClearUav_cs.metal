#include <metal_stdlib>
using namespace metal;

kernel void main_metal
(
	@insertpiece(uav0_data_type) dstTex [[texture(UAV_SLOT_START+0)]],

	@property( !uav0_is_integer )
		constant float4 &fClearValue    [[buffer(PARAMETER_SLOT)]],
		#define clearValue fClearValue
	@else
		@property( !uav0_is_signed )
			constant uint4 &uClearValue [[buffer(PARAMETER_SLOT)]],
			#define clearValue uClearValue
		@else
			constant int4 &iClearValue  [[buffer(PARAMETER_SLOT)]],
			#define clearValue iClearValue
		@end
	@end

	ushort3 gl_GlobalInvocationID       [[thread_position_in_grid]]
)
{
@property( uav0_texture_type == TextureTypes_Type3D || uav0_texture_type == TextureTypes_Type2DArray || uav0_texture_type == TextureTypes_TypeCube || uav0_texture_type == TextureTypes_TypeCubeArray )
	dstTex.write( (@insertpiece(uav0_pf_type)4)clearValue, gl_GlobalInvocationID.xyz );
@end
@property( uav0_texture_type == TextureTypes_Type2D || uav0_texture_type == TextureTypes_Type1DArray )
	dstTex.write( (@insertpiece(uav0_pf_type)4)clearValue, gl_GlobalInvocationID.xy );
@end
@property( uav0_texture_type == TextureTypes_Type1D )
	dstTex.write( (@insertpiece(uav0_pf_type)4)clearValue, gl_GlobalInvocationID.x );
@end
}
