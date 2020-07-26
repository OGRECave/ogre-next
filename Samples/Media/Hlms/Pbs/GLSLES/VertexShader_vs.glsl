@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

@property( GL3+ )
out gl_PerVertex
{
	vec4 gl_Position;
@property( hlms_global_clip_planes )
	float gl_ClipDistance[@value(hlms_global_clip_planes)];
@end
};
@end

layout(std140) uniform;

@insertpiece( Common_Matrix_DeclUnpackMatrix4x4 )
@insertpiece( Common_Matrix_DeclUnpackMatrix3x4 )

in vec4 vertex;

@property( hlms_normal )in vec3 normal;@end
@property( hlms_qtangent )in vec4 qtangent;@end

@property( normal_map && !hlms_qtangent )
in vec3 tangent;
@property( hlms_binormal )in vec3 binormal;@end
@end

@property( hlms_skeleton )
in uvec4 blendIndices;
in vec4 blendWeights;@end

@foreach( hlms_uv_count, n )
in vec@value( hlms_uv_count@n ) uv@n;@end

@property( GL_ARB_base_instance )
	in uint drawId;
@end

@insertpiece( custom_vs_attributes )

@property( !hlms_shadowcaster || !hlms_shadow_uses_depth_texture || alpha_test || exponential_shadow_maps )
out block
{
@insertpiece( VStoPS_block )
} outVs;
@end

// START UNIFORM DECLARATION
@insertpiece( PassDecl )
@property( hlms_skeleton || hlms_shadowcaster || hlms_pose )@insertpiece( InstanceDecl )@end
/*layout(binding = 0) */uniform samplerBuffer worldMatBuf;
@insertpiece( custom_vs_uniformDeclaration )
@property( !GL_ARB_base_instance )uniform uint baseInstance;@end
@property( hlms_pose )
	uniform samplerBuffer poseBuf;
@end
// END UNIFORM DECLARATION

@property( hlms_qtangent )
@insertpiece( DeclQuat_xAxis )
@property( normal_map )
@insertpiece( DeclQuat_yAxis )
@end @end

@property( !hlms_pose )
@piece( input_vertex )vertex@end
@end
@property( hlms_pose )
@piece( input_vertex )inputPos@end
@end

@property( !hlms_pose_normals )
@piece( input_normal )normal@end
@end
@property( hlms_pose_normals )
@piece( input_normal )inputNormal@end
@end

@property( !hlms_skeleton )
@piece( local_vertex )@insertpiece( input_vertex )@end
@piece( local_normal )@insertpiece( input_normal )@end
@piece( local_tangent )tangent@end
@end
@property( hlms_skeleton )
@piece( local_vertex )worldPos@end
@piece( local_normal )worldNorm@end
@piece( local_tangent )worldTang@end
@end

@property( hlms_skeleton )@piece( SkeletonTransform )
	uint _idx = (blendIndices[0] << 1u) + blendIndices[0]; //blendIndices[0] * 3u; a 32-bit int multiply is 4 cycles on GCN! (and mul24 is not exposed to GLSL...)
		uint matStart = instance.worldMaterialIdx[drawId].x >> 9u;
	vec4 worldMat[3];
		worldMat[0] = bufferFetch( worldMatBuf, int(matStart + _idx + 0u) );
		worldMat[1] = bufferFetch( worldMatBuf, int(matStart + _idx + 1u) );
		worldMat[2] = bufferFetch( worldMatBuf, int(matStart + _idx + 2u) );
    vec4 worldPos;
    worldPos.x = dot( worldMat[0], @insertpiece( input_vertex ) );
    worldPos.y = dot( worldMat[1], @insertpiece( input_vertex ) );
    worldPos.z = dot( worldMat[2], @insertpiece( input_vertex ) );
    worldPos.xyz *= blendWeights[0];
    @property( hlms_normal || hlms_qtangent )vec3 worldNorm;
    worldNorm.x = dot( worldMat[0].xyz, @insertpiece( input_normal ) );
    worldNorm.y = dot( worldMat[1].xyz, @insertpiece( input_normal ) );
    worldNorm.z = dot( worldMat[2].xyz, @insertpiece( input_normal ) );
    worldNorm *= blendWeights[0];@end
    @property( normal_map )vec3 worldTang;
    worldTang.x = dot( worldMat[0].xyz, tangent );
    worldTang.y = dot( worldMat[1].xyz, tangent );
    worldTang.z = dot( worldMat[2].xyz, tangent );
    worldTang *= blendWeights[0];@end

	@psub( NeedsMoreThan1BonePerVertex, hlms_bones_per_vertex, 1 )
	@property( NeedsMoreThan1BonePerVertex )vec4 tmp;
	tmp.w = 1.0;@end //!NeedsMoreThan1BonePerVertex
	@foreach( hlms_bones_per_vertex, n, 1 )
	_idx = (blendIndices[@n] << 1u) + blendIndices[@n]; //blendIndices[@n] * 3; a 32-bit int multiply is 4 cycles on GCN! (and mul24 is not exposed to GLSL...)
		worldMat[0] = bufferFetch( worldMatBuf, int(matStart + _idx + 0u) );
		worldMat[1] = bufferFetch( worldMatBuf, int(matStart + _idx + 1u) );
		worldMat[2] = bufferFetch( worldMatBuf, int(matStart + _idx + 2u) );
	tmp.x = dot( worldMat[0], @insertpiece( input_vertex ) );
	tmp.y = dot( worldMat[1], @insertpiece( input_vertex ) );
	tmp.z = dot( worldMat[2], @insertpiece( input_vertex ) );
	worldPos.xyz += (tmp * blendWeights[@n]).xyz;
	@property( hlms_normal || hlms_qtangent )
	tmp.x = dot( worldMat[0].xyz, @insertpiece( input_normal ) );
	tmp.y = dot( worldMat[1].xyz, @insertpiece( input_normal ) );
	tmp.z = dot( worldMat[2].xyz, @insertpiece( input_normal ) );
    worldNorm += tmp.xyz * blendWeights[@n];@end
	@property( normal_map )
	tmp.x = dot( worldMat[0].xyz, tangent );
	tmp.y = dot( worldMat[1].xyz, tangent );
	tmp.z = dot( worldMat[2].xyz, tangent );
    worldTang += tmp.xyz * blendWeights[@n];@end
	@end

	worldPos.w = 1.0;
@end @end //SkeletonTransform // !hlms_skeleton

@property( hlms_pose )@piece( PoseTransform )
	// Pose data starts after all 3x4 bone matrices
	int poseDataStart = int(instance.worldMaterialIdx[drawId].x >> 9u) @property( hlms_skeleton ) + @value(hlms_bones_per_vertex) * 3@end ;
	vec4 inputPos = vertex;
	@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )vec3 inputNormal = normal;@end

	vec4 poseData = bufferFetch( worldMatBuf, poseDataStart );
	int baseVertexID = int(floatBitsToUint( poseData.x ));
	int vertexID = gl_VertexID - baseVertexID;

	@psub( MoreThanOnePose, hlms_pose, 1 )
	@property( !MoreThanOnePose )
		vec4 poseWeights = bufferFetch( worldMatBuf, poseDataStart + 1 );
		vec4 posePos = bufferFetch( poseBuf, vertexID @property( hlms_pose_normals )<< 1@end );
		inputPos += posePos * poseWeights.x;
		@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )
			vec4 poseNormal = bufferFetch( poseBuf, (vertexID << 1) + 1 );
			inputNormal += poseNormal.xyz * poseWeights.x;
		@end
		@pset( NumPoseWeightVectors, 1 )
	@end @property( MoreThanOnePose )
		// NumPoseWeightVectors = (hlms_pose / 4) + min(hlms_pose % 4, 1)
		@pdiv( NumPoseWeightVectorsA, hlms_pose, 4 )
		@pmod( NumPoseWeightVectorsB, hlms_pose, 4 )
		@pmin( NumPoseWeightVectorsC, NumPoseWeightVectorsB, 1 )
		@padd( NumPoseWeightVectors, NumPoseWeightVectorsA, NumPoseWeightVectorsC)
		int numVertices = int(floatBitsToUint( poseData.y ));

		@psub( MoreThanOnePoseWeightVector, NumPoseWeightVectors, 1)
		@property( !MoreThanOnePoseWeightVector )
			vec4 poseWeights = bufferFetch( worldMatBuf, poseDataStart + 1 );
			@foreach( hlms_pose, n )
				inputPos += bufferFetch( poseBuf, (vertexID + numVertices * @n) @property( hlms_pose_normals )<< 1@end ) * poseWeights[@n];
				@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )
				inputNormal += bufferFetch( poseBuf, ((vertexID + numVertices * @n) << 1) + 1 ).xyz * poseWeights[@n];
				@end
			@end
		@end @property( MoreThanOnePoseWeightVector )
			float poseWeights[@value(NumPoseWeightVectors) * 4];
			@foreach( NumPoseWeightVectors, n)
				vec4 weights@n = bufferFetch( worldMatBuf, poseDataStart + 1 + @n );
				poseWeights[@n * 4 + 0] = weights@n[0];
				poseWeights[@n * 4 + 1] = weights@n[1];
				poseWeights[@n * 4 + 2] = weights@n[2];
				poseWeights[@n * 4 + 3] = weights@n[3];
			@end
			@foreach( hlms_pose, n )
				inputPos += bufferFetch( poseBuf, (vertexID + numVertices * @n) @property( hlms_pose_normals )<< 1@end ) * poseWeights[@n];
				@property( hlms_pose_normals && (hlms_normal || hlms_qtangent) )
				inputNormal += bufferFetch( poseBuf, ((vertexID + numVertices * @n) << 1) + 1 ).xyz * poseWeights[@n];
				@end
			@end
		@end
	@end

	// If hlms_skeleton is defined the transforms will be provided by bones.
	// If hlms_pose is not combined with hlms_skeleton the object's worldMat and worldView have to be set.
	@property( !hlms_skeleton )
		vec4 worldMat[3];
		worldMat[0] = bufferFetch( worldMatBuf, poseDataStart + @value(NumPoseWeightVectors) + 1 );
		worldMat[1] = bufferFetch( worldMatBuf, poseDataStart + @value(NumPoseWeightVectors) + 2 );
		worldMat[2] = bufferFetch( worldMatBuf, poseDataStart + @value(NumPoseWeightVectors) + 3 );
		vec4 worldPos;
		worldPos.x = dot( worldMat[0], inputPos );
		worldPos.y = dot( worldMat[1], inputPos );
		worldPos.z = dot( worldMat[2], inputPos );
		worldPos.w = 1.0;
		
		@property( hlms_normal || hlms_qtangent )
		@foreach( 4, n )
			vec4 row@n = bufferFetch( worldMatBuf, poseDataStart + @value(NumPoseWeightVectors) + 4 + @n ); @end
		mat4 worldView = mat4( row0, row1, row2, row3 );
		@end
	@end
@end @end // PoseTransform

@property( hlms_skeleton )
	@piece( worldViewMat )passBuf.view@end
@end @property( !hlms_skeleton )
    @piece( worldViewMat )worldView@end
@end

@piece( CalculatePsPos )(@insertpiece(local_vertex) * @insertpiece( worldViewMat )).xyz@end

@piece( VertexTransform )
@insertpiece( custom_vs_preTransform )
	//Lighting is in view space
	@property( hlms_normal || hlms_qtangent )outVs.pos		= @insertpiece( CalculatePsPos );@end
	@property( hlms_normal || hlms_qtangent )outVs.normal	= @insertpiece(local_normal) * mat3(@insertpiece( worldViewMat ));@end
	@property( normal_map )outVs.tangent	= @insertpiece(local_tangent) * mat3(@insertpiece( worldViewMat ));@end
@property( !hlms_dual_paraboloid_mapping )
	gl_Position = worldPos * passBuf.viewProj;@end
@property( hlms_dual_paraboloid_mapping )
	//Dual Paraboloid Mapping
	gl_Position.w	= 1.0f;
	@property( hlms_normal || hlms_qtangent )gl_Position.xyz	= outVs.pos;@end
	@property( !hlms_normal && !hlms_qtangent )gl_Position.xyz	= @insertpiece( CalculatePsPos );@end
	float L = length( gl_Position.xyz );
	gl_Position.z	+= 1.0f;
	gl_Position.xy	/= gl_Position.z;
	gl_Position.z	= (L - NearPlane) / (FarPlane - NearPlane);@end
@end

void main()
{
@property( !GL_ARB_base_instance )
    uint drawId = baseInstance + uint( gl_InstanceID );
@end

    @insertpiece( custom_vs_preExecution )

@property( !hlms_skeleton && !hlms_pose )

    mat3x4 worldMat = UNPACK_MAT3x4( worldMatBuf, drawId @property( !hlms_shadowcaster )<< 1u@end );
	@property( hlms_normal || hlms_qtangent )
	mat4 worldView = UNPACK_MAT4( worldMatBuf, (drawId << 1u) + 1u );
	@end

	vec4 worldPos = vec4( (vertex * worldMat).xyz, 1.0f );
@end

@property( hlms_qtangent )
	//Decode qTangent to TBN with reflection
	vec3 normal		= xAxis( normalize( qtangent ) );
	@property( normal_map )
	vec3 tangent	= yAxis( qtangent );
	outVs.biNormalReflection = sign( qtangent.w ); //We ensure in C++ qtangent.w is never 0
	@end
@end

	@insertpiece( PoseTransform )
	@insertpiece( SkeletonTransform )
	@insertpiece( VertexTransform )

	@insertpiece( DoShadowReceiveVS )
	@insertpiece( DoShadowCasterVS )

	/// hlms_uv_count will be 0 on shadow caster passes w/out alpha test
@foreach( hlms_uv_count, n )
	outVs.uv@n = uv@n;@end

@property( (!hlms_shadowcaster || alpha_test) && !lower_gpu_overhead )
	outVs.drawId = drawId;@end

	@property( hlms_use_prepass_msaa > 1 )
		outVs.zwDepth.xy = outVs.gl_Position.zw;
	@end

@property( hlms_global_clip_planes )
	gl_ClipDistance[0] = dot( float4( worldPos.xyz, 1.0 ), passBuf.clipPlane0.xyzw );
@end

	@insertpiece( custom_vs_posExecution )
}
