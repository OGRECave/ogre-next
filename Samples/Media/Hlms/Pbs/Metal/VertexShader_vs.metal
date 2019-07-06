@insertpiece( SetCrossPlatformSettings )

@insertpiece( Common_Matrix_DeclUnpackMatrix4x4 )
@insertpiece( Common_Matrix_DeclUnpackMatrix3x4 )

@property( hlms_normal || hlms_qtangent || normal_map )
	@insertpiece( Common_Matrix_Conversions )
@end

struct VS_INPUT
{
	float4 position [[attribute(VES_POSITION)]];
@property( hlms_normal )	float3 normal [[attribute(VES_NORMAL)]];@end
@property( hlms_qtangent )	float4 qtangent [[attribute(VES_NORMAL)]];@end

@property( normal_map && !hlms_qtangent )
	float3 tangent	[[attribute(VES_TANGENT)]];
	@property( hlms_binormal )float3 binormal	[[attribute(VES_BINORMAL)]];@end
@end

@property( hlms_skeleton )
	uint4 blendIndices	[[attribute(VES_BLEND_INDICES)]];
	float4 blendWeights [[attribute(VES_BLEND_WEIGHTS)]];@end

@foreach( hlms_uv_count, n )
	float@value( hlms_uv_count@n ) uv@n [[attribute(VES_TEXTURE_COORDINATES@n)]];@end
@property( !iOS )
	ushort drawId [[attribute(15)]];
@end
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
@insertpiece( VStoPS_block )
	float4 gl_Position [[position]];
	@foreach( hlms_pso_clip_distances, n )
		float gl_ClipDistance@n [[clip_distance]];
	@end
};

// START UNIFORM STRUCT DECLARATION
@insertpiece( PassStructDecl )
@insertpiece( custom_vs_uniformStructDeclaration )
// END UNIFORM STRUCT DECLARATION

@property( hlms_qtangent )
@insertpiece( DeclQuat_xAxis )
@property( normal_map )
@insertpiece( DeclQuat_yAxis )
@end @end

@property( !hlms_skeleton )
@piece( local_vertex )input.position@end
@piece( local_normal )normal@end
@piece( local_tangent )tangent@end
@end
@property( hlms_skeleton )
@piece( local_vertex )worldPos@end
@piece( local_normal )worldNorm@end
@piece( local_tangent )worldTang@end
@end

@property( hlms_skeleton )@piece( SkeletonTransform )
	int _idx = int((input.blendIndices[0] << 1u) + input.blendIndices[0]); //blendIndices[0] * 3u; a 32-bit int multiply is 4 cycles on GCN! (and mul24 is not exposed to GLSL...)
		int matStart = int(worldMaterialIdx[drawId].x >> 9u);
	float4 worldMat[3];
		worldMat[0] = worldMatBuf[matStart + _idx + 0u];
		worldMat[1] = worldMatBuf[matStart + _idx + 1u];
		worldMat[2] = worldMatBuf[matStart + _idx + 2u];
	float4 worldPos;
	worldPos.x = dot( worldMat[0], input.position );
	worldPos.y = dot( worldMat[1], input.position );
	worldPos.z = dot( worldMat[2], input.position );
	worldPos.xyz *= input.blendWeights[0];
	@property( hlms_normal || hlms_qtangent )float3 worldNorm;
	worldNorm.x = dot( worldMat[0].xyz, normal );
	worldNorm.y = dot( worldMat[1].xyz, normal );
	worldNorm.z = dot( worldMat[2].xyz, normal );
	worldNorm *= input.blendWeights[0];@end
	@property( normal_map )float3 worldTang;
	worldTang.x = dot( worldMat[0].xyz, tangent );
	worldTang.y = dot( worldMat[1].xyz, tangent );
	worldTang.z = dot( worldMat[2].xyz, tangent );
	worldTang *= input.blendWeights[0];@end

	@psub( NeedsMoreThan1BonePerVertex, hlms_bones_per_vertex, 1 )
	@property( NeedsMoreThan1BonePerVertex )float4 tmp;
	tmp.w = 1.0;@end //!NeedsMoreThan1BonePerVertex
	@foreach( hlms_bones_per_vertex, n, 1 )
	_idx = (input.blendIndices[@n] << 1u) + input.blendIndices[@n]; //blendIndices[@n] * 3; a 32-bit int multiply is 4 cycles on GCN! (and mul24 is not exposed to GLSL...)
		worldMat[0] = worldMatBuf[matStart + _idx + 0u];
		worldMat[1] = worldMatBuf[matStart + _idx + 1u];
		worldMat[2] = worldMatBuf[matStart + _idx + 2u];
	tmp.x = dot( worldMat[0], input.position );
	tmp.y = dot( worldMat[1], input.position );
	tmp.z = dot( worldMat[2], input.position );
	worldPos.xyz += (tmp * input.blendWeights[@n]).xyz;
	@property( hlms_normal || hlms_qtangent )
	tmp.x = dot( worldMat[0].xyz, normal );
	tmp.y = dot( worldMat[1].xyz, normal );
	tmp.z = dot( worldMat[2].xyz, normal );
	worldNorm += tmp.xyz * input.blendWeights[@n];@end
	@property( normal_map )
	tmp.x = dot( worldMat[0].xyz, tangent );
	tmp.y = dot( worldMat[1].xyz, tangent );
	tmp.z = dot( worldMat[2].xyz, tangent );
	worldTang += tmp.xyz * input.blendWeights[@n];@end
	@end

	worldPos.w = 1.0;
@end @end  //SkeletonTransform // !hlms_skeleton

@property( hlms_pose )@piece( PoseTransform )
	// Pose data starts after all 3x4 bone matrices
	uint poseDataStart = (worldMaterialIdx[drawId].x >> 9u) @property( hlms_skeleton ) + @value(hlms_bones_per_vertex)u * 3u@end ;
	uint poseVertexId = vertexId - baseVertex;
	
	@psub( MoreThanOnePose, hlms_pose, 1 )
	@property( !MoreThanOnePose )
		float4 poseWeights = worldMatBuf[poseDataStart + 1u];
		uint idx = poseVertexId @property( hlms_pose_normals )<< 1u@end ;
		float4 posePos = static_cast<float4>( poseBuf[idx] );
		input.position += posePos * poseWeights.x;
		@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )
			float4 poseNormal = static_cast<float4>( poseBuf[idx + 1u] );
			normal += poseNormal.xyz * poseWeights.x;
		@end
		@pset( NumPoseWeightVectors, 1 )
	@end @property( MoreThanOnePose )
		// NumPoseWeightVectors = (hlms_pose / 4) + min(hlms_pose % 4, 1)
		@pdiv( NumPoseWeightVectorsA, hlms_pose, 4 )
		@pmod( NumPoseWeightVectorsB, hlms_pose, 4 )
		@pmin( NumPoseWeightVectorsC, NumPoseWeightVectorsB, 1 )
		@padd( NumPoseWeightVectors, NumPoseWeightVectorsA, NumPoseWeightVectorsC)
		float4 poseData = worldMatBuf[poseDataStart];
		uint numVertices = as_type<uint>( poseData.y );
		
		@psub( MoreThanOnePoseWeightVector, NumPoseWeightVectors, 1)
		@property( !MoreThanOnePoseWeightVector )
			float4 poseWeights = worldMatBuf[poseDataStart + 1u];
			@foreach( hlms_pose, n )
				uint idx@n = (poseVertexId + numVertices * @nu) @property( hlms_pose_normals )<< 1u@end ;
				input.position += static_cast<float4>( poseBuf[idx@n] ) * poseWeights[@n];
				@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )
				normal += static_cast<float4>( poseBuf[idx@n + 1u] ).xyz * poseWeights[@n];
				@end
			@end
		@end @property( MoreThanOnePoseWeightVector )
			float poseWeights[@value(NumPoseWeightVectors) * 4];
			@foreach( NumPoseWeightVectors, n)
				float4 weights@n = worldMatBuf[poseDataStart + 1u + @nu];
				poseWeights[@n * 4 + 0] = weights@n[0];
				poseWeights[@n * 4 + 1] = weights@n[1];
				poseWeights[@n * 4 + 2] = weights@n[2];
				poseWeights[@n * 4 + 3] = weights@n[3];
			@end
			@foreach( hlms_pose, n )
				uint idx@n = (poseVertexId + numVertices * @nu) @property( hlms_pose_normals )<< 1u@end ;
				input.position += static_cast<float4>( poseBuf[idx@n] ) * poseWeights[@n];
				@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )
				normal += static_cast<float4>( poseBuf[idx@n + 1u] ).xyz * poseWeights[@n];
				@end
			@end
		@end
	@end
	
	// If hlms_skeleton is defined the transforms will be provided by bones.
	// If hlms_pose is not combined with hlms_skeleton the object's worldMat and worldView have to be set.
	@property( !hlms_skeleton )
		float4 worldMat[3];
		worldMat[0] = worldMatBuf[poseDataStart + @value(NumPoseWeightVectors)u + 1u];
		worldMat[1] = worldMatBuf[poseDataStart + @value(NumPoseWeightVectors)u + 2u];
		worldMat[2] = worldMatBuf[poseDataStart + @value(NumPoseWeightVectors)u + 3u];
		float4 worldPos;
		worldPos.x = dot( worldMat[0], input.position );
		worldPos.y = dot( worldMat[1], input.position );
		worldPos.z = dot( worldMat[2], input.position );
		worldPos.w = 1.0;
		
		@property( hlms_normal || hlms_qtangent )
		@foreach( 4, n )
			float4 row@n = worldMatBuf[poseDataStart + @value(NumPoseWeightVectors)u + 4u + @nu];@end
		float4x4 worldView = float4x4( row0, row1, row2, row3 );
		@end
	@end
@end @end // PoseTransform

@property( hlms_skeleton )
	@piece( worldViewMat )passBuf.view@end
@end @property( !hlms_skeleton )
	@piece( worldViewMat )worldView@end
@end

@piece( CalculatePsPos )( @insertpiece(local_vertex) * @insertpiece( worldViewMat ) ).xyz@end

@piece( VertexTransform )
@insertpiece( custom_vs_preTransform )
	//Lighting is in view space
	@property( hlms_normal || hlms_qtangent || normal_map )float3x3 mat3x3 = toMat3x3( @insertpiece( worldViewMat ) );@end
	@property( hlms_normal || hlms_qtangent )outVs.pos		= @insertpiece( CalculatePsPos );@end
	@property( hlms_normal || hlms_qtangent )outVs.normal	= @insertpiece(local_normal) * mat3x3;@end
	@property( normal_map )outVs.tangent	= @insertpiece(local_tangent) * mat3x3;@end
@property( !hlms_dual_paraboloid_mapping )
	outVs.gl_Position = worldPos * passBuf.viewProj;@end
@property( hlms_dual_paraboloid_mapping )
	//Dual Paraboloid Mapping
	outVs.gl_Position.w	= 1.0f;
	@property( hlms_normal || hlms_qtangent )outVs.gl_Position.xyz	= outVs.pos;@end
	@property( !hlms_normal && !hlms_qtangent )outVs.gl_Position.xyz	= @insertpiece( CalculatePsPos );@end
	float L = length( outVs.gl_Position.xyz );
	outVs.gl_Position.z	+= 1.0f;
	outVs.gl_Position.xy	/= outVs.gl_Position.z;
	outVs.gl_Position.z	= (L - NearPlane) / (FarPlane - NearPlane);@end
@end

vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]]
	@property( iOS )
		, ushort instanceId [[instance_id]]
		, constant ushort &baseInstance [[buffer(15)]]
	@end
	// START UNIFORM DECLARATION
	@insertpiece( PassDecl )
	@insertpiece( InstanceDecl )
	, device const float4 *worldMatBuf [[buffer(TEX_SLOT_START+0)]]
	@property( hlms_pose )
		@property( !hlms_pose_half )
		, device const float4 *poseBuf [[buffer(TEX_SLOT_START+4)]]
		@end @property( hlms_pose_half )
		, device const half4 *poseBuf [[buffer(TEX_SLOT_START+4)]]
		@end
		, uint vertexId [[vertex_id]]
		, uint baseVertex [[base_vertex]]
	@end
	@insertpiece( custom_vs_uniformDeclaration )
	// END UNIFORM DECLARATION
)
{
	@property( iOS )
		ushort drawId = baseInstance + instanceId;
	@end @property( !iOS )
		ushort drawId = input.drawId;
	@end

	PS_INPUT outVs;
	@insertpiece( custom_vs_preExecution )
	
@property( !hlms_skeleton && !hlms_pose )
	float3x4 worldMat = UNPACK_MAT3x4( worldMatBuf, drawId @property( !hlms_shadowcaster )<< 1u@end );
	@property( hlms_normal || hlms_qtangent )
	float4x4 worldView = UNPACK_MAT4( worldMatBuf, (drawId << 1u) + 1u );
	@end

	float4 worldPos = float4( ( input.position * worldMat ).xyz, 1.0f );
@end

@property( hlms_qtangent )
	//Decode qTangent to TBN with reflection
	float3 normal	= xAxis( normalize( input.qtangent ) );
	@property( normal_map )
	float3 tangent	= yAxis( input.qtangent );
	outVs.biNormalReflection = sign( input.qtangent.w ); //We ensure in C++ qtangent.w is never 0
	@end
@end @property( !hlms_qtangent && hlms_normal )
	float3 normal	= input.normal;
	@property( normal_map )float3 tangent	= input.tangent;@end
@end

	@insertpiece( PoseTransform )
	@insertpiece( SkeletonTransform )
	@insertpiece( VertexTransform )

	@insertpiece( DoShadowReceiveVS )
	@insertpiece( DoShadowCasterVS )

	/// hlms_uv_count will be 0 on shadow caster passes w/out alpha test
@foreach( hlms_uv_count, n )
	outVs.uv@n = input.uv@n;@end

@property( (!hlms_shadowcaster || alpha_test) && !lower_gpu_overhead )
	outVs.materialId = worldMaterialIdx[drawId].x & 0x1FFu;@end

@property( hlms_fine_light_mask || hlms_forwardplus_fine_light_mask )
	outVs.objLightMask = worldMaterialIdx[drawId].z;@end

@property( use_planar_reflections )
	outVs.planarReflectionIdx = (ushort)(worldMaterialIdx[drawId].w);@end

@property( hlms_global_clip_planes )
	outVs.gl_ClipDistance0 = dot( float4( worldPos.xyz, 1.0 ), passBuf.clipPlane0.xyzw );
@end

	@insertpiece( custom_vs_posExecution )

	return outVs;
}
