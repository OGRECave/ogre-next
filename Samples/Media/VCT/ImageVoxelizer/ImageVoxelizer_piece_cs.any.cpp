// Heavily modified version based on voxelization shader written by mattatz
//	https://github.com/mattatz/unity-voxel
// Original work Copyright (c) 2018 mattatz under MIT license.
//
// mattatz's work is a full voxelization algorithm.
// We only need to voxelize the shell (aka the contour),
// not to fill the entire voxel.
//
// Adapted for Ogre and for use for Voxel Cone Tracing by
// Matias N. Goldberg Copyright (c) 2019

#include "/media/matias/Datos/SyntaxHighlightingMisc.h"

#define p_instanceStart 0
#define p_instanceEnd 5
#define p_voxelOrigin float3( 0, 0, 0 )
#define p_voxelCellSize float3( 1.0, 1.0, 1.0 )

@piece( PreBindingsHeaderCS )
	struct InstanceBuffer
	{
		float4 worldTransformRow0;
		float4 worldTransformRow1;
		float4 worldTransformRow2;
		float4 aabb0;
		float4 aabb1;
	};
@end

@piece( HeaderCS )
	@insertpiece( Common_Matrix_DeclLoadOgreFloat4x3 )
	struct Aabb
	{
		float3 center;
		float3 halfSize;
	};

	struct Instance
	{
		ogre_float4x3 worldTransform;
		uint albedoTexIdx;
		Aabb bounds;
	};

	INLINE Instance getInstance( uint instanceIdx PARAMS_ARG_DECL )
	{
		Instance retVal;

		retVal.worldTransform = makeOgreFloat4x3( instanceBuffer[instanceIdx].worldTransformRow0,
												  instanceBuffer[instanceIdx].worldTransformRow1,
												  instanceBuffer[instanceIdx].worldTransformRow2 );

		retVal.bounds.center = instanceBuffer[instanceIdx].aabb0.xyz;
		retVal.bounds.halfSize = instanceBuffer[instanceIdx].aabb1.xyz;

		retVal.albedoTexIdx = floatBitsToUint( instanceBuffer[instanceIdx].aabb0.w );

		return retVal;
	}

	INLINE bool intersects( Aabb a, Aabb b )
	{
		float3 absDistance = abs( a.center - b.center );
		float3 sumHalfSizes = a.halfSize + b.halfSize;

		// ( abs( center.x - center2.x ) <= halfSize.x + halfSize2.x &&
		//   abs( center.y - center2.y ) <= halfSize.y + halfSize2.y &&
		//   abs( center.z - center2.z ) <= halfSize.z + halfSize2.z )
		return absDistance.x <= sumHalfSizes.x &&  //
			   absDistance.y <= sumHalfSizes.y &&  //
			   absDistance.z <= sumHalfSizes.z;
	}

	INLINE float3 mixAverage3( float3 newAccumValues, float numHits, float3 oldAveragedValue,
							   float oldNumHits )
	{
		return newAccumValues / ( numHits + oldNumHits ) +
			   oldAveragedValue * ( oldNumHits / ( numHits + oldNumHits ) );
	}
	INLINE float4 mixAverage4( float4 newAccumValues, float numHits, float4 oldAveragedValue,
							   float oldNumHits )
	{
		return newAccumValues / ( numHits + oldNumHits ) +
			   oldAveragedValue * ( oldNumTris / ( numTris + oldNumTris ) );
	}
@end

//in uvec3 gl_NumWorkGroups;
//in uvec3 gl_WorkGroupID;
//in uvec3 gl_LocalInvocationID;
//in uvec3 gl_GlobalInvocationID;
//in uint  gl_LocalInvocationIndex;

@piece( BodyCS )
	Aabb voxelAabb;
	voxelAabb.center = p_voxelOrigin + p_voxelCellSize * ( float3( gl_GlobalInvocationID.xyz ) + 0.5f );
	voxelAabb.halfSize = p_voxelCellSize * 0.5f;

	Aabb groupVoxelAabb;
	groupVoxelAabb.center =
		p_voxelOrigin + 4.0f * p_voxelCellSize * ( float3( gl_WorkGroupID.xyz ) + 0.5f );
	groupVoxelAabb.halfSize = 4.0f * p_voxelCellSize * 0.5f;

	bool doubleSided = false;
	float accumHits = 0;
	float4 voxelAlbedo = float4( 0, 0, 0, 0 );
	float4 voxelNormal = float4( 0, 0, 0, 0 );
	float4 voxelEmissive = float4( 0, 0, 0, 0 );

	for( uint i = p_instanceStart; i < p_instanceEnd; ++i )
	{
		Instance instance = getInstance( i PARAMS_ARG );

		// Broadphase culling.
		// Cull against groupVoxelAabb so that anyInvocationARB can work (all lanes
		// must be active for GroupMemoryBarrierWithGroupSync to work, and even in the
		// non-emulated version we need all threads to be in the same iteration)
		if( intersects( groupVoxelAabb, instance.bounds ) )
		{
			uint albedoTexIdx = instance.albedoTexIdx;
			uint normalTexIdx = instance.albedoTexIdx + 1u;
			uint emissiveIdx = instance.albedoTexIdx + 2u;

			float3 instanceUvw = mul( instance.worldTransform, float4( voxelAabb.center, 1.0f ) ).xyz;

			float4 instanceAlbedo = OGRE_Sample( meshTextures[albedoTexIdx], instanceUvw, lodLevel );
			float3 instanceEmissive =
				OGRE_Sample( meshTextures[emissiveIdx], instanceUvw, lodLevel ).xyz;
			float3 instanceNormal = OGRE_Sample( meshTextures[normalTexIdx], instanceUvw, lodLevel );

			if( instanceUvw.x >= 0.0f && instanceUvw.x <= 1.0f &&  //
				instanceUvw.y >= 0.0f && instanceUvw.y <= 1.0f &&  //
				instanceUvw.z >= 0.0f && instanceUvw.z <= 1.0f &&  //
				abs( instanceAlbedo.a ) +                          //
						abs( instanceEmissive.x ) +                //
						abs( instanceEmissive.y ) +                //
						abs( instanceEmissive.z ) >
					0.0f )
			{
				voxelAlbedo += instanceAlbedo;
				voxelEmissive.xyz += instanceEmissive.xyz;

				instanceNormal = normalize( instanceNormal );

				// voxelNormal is not normalized for properly blend-averaging in multiple
				// passes. But we need it normalized for this check to work
				float cosAngle = dot( normalize( voxelNormal.xyz ), instanceNormal );
				float cos120 = -0.5f;
				/*Rewrote as ternary operator because it was glitching in iOS Metal
				if( cosAngle <= cos120 )
					doubleSided = true;
				else
					voxelNormal.xyz += instanceNormal;*/
				doubleSided = cosAngle <= cos120 ? true : doubleSided;
				voxelNormal.xyz += cosAngle <= cos120 ? float3( 0, 0, 0 ) : instanceNormal;

				++accumHits;
			}
		}
	}

	wshort3 voxelCelUvw = wshort3( gl_GlobalInvocationID.xyz + p_voxelPixelOrigin );

	@property( syntax != hlsl || typed_uav_load )
		float4 origAlbedo	= OGRE_imageLoad3D( voxelAlbedoTex, voxelCelUvw );
		float4 origNormal	= OGRE_imageLoad3D( voxelNormalTex, voxelCelUvw );
		float4 origEmissive = OGRE_imageLoad3D( voxelEmissiveTex, voxelCelUvw );
		float origAccumHits = OGRE_imageLoad3D( voxelAccumVal, voxelCelUvw ).x;
	@else
		float4 origAlbedo	= unpackUnorm4x8( OGRE_imageLoad3D( voxelAlbedoTex, voxelCelUvw ) );
		float4 origNormal	= unpackUnormRGB10A2( OGRE_imageLoad3D( voxelNormalTex, voxelCelUvw ) );
		float4 origEmissive = unpackUnorm4x8( OGRE_imageLoad3D( voxelEmissiveTex, voxelCelUvw ) );
		uint origAccumHitsVal = OGRE_imageLoad3D( voxelAccumVal,
												  wshort3( voxelCelUvw.x >> 1u, voxelCelUvw.yz ) ).x;
		float origAccumHits = (gl_LocalInvocationIndex & 0x1u) ? (origAccumHitsVal >> 16u) :
																 (origAccumHitsVal & 0xFFFFu);
	@end

	origNormal.xyz = origNormal.xyz * 2.0f - 1.0f;

	if( accumHits + origAccumHits > 0 )
	{
		voxelAlbedo			= mixAverage4( voxelAlbedo, accumHits, origAlbedo, origAccumHits );
		voxelNormal.xyz		= mixAverage3( voxelNormal.xyz, accumHits, origNormal.xyz, origAccumHits );
		voxelEmissive.xyz	= mixAverage3( voxelEmissive.xyz, accumHits, origEmissive.xyz, origAccumHits );

		voxelNormal.a	= doubleSided ? 1.0f : 0.0f;
		voxelNormal.a	= max( voxelNormal.a, voxelNormal.a );
		voxelNormal.xyz	= voxelNormal.xyz * 0.5 + 0.5f;

		@property( syntax != hlsl || typed_uav_load )
			OGRE_imageWrite3D4( voxelAlbedoTex, voxelCelUvw, voxelAlbedo );
			OGRE_imageWrite3D4( voxelNormalTex, voxelCelUvw, voxelNormal );
			OGRE_imageWrite3D4( voxelEmissiveTex, voxelCelUvw, voxelEmissive );
			OGRE_imageWrite3D1( voxelAccumVal, voxelCelUvw,
								uint4( accumHits + origAccumHits, 0, 0, 0 ) );
		@else
			OGRE_imageWrite3D4( voxelAlbedoTex, voxelCelUvw, packUnorm4x8( voxelAlbedo ) );
			OGRE_imageWrite3D4( voxelNormalTex, voxelCelUvw, packUnormRGB10A2( voxelNormal ) );
			OGRE_imageWrite3D4( voxelEmissiveTex, voxelCelUvw, packUnorm4x8( voxelEmissive ) );
		@end
	}

	@property( syntax == hlsl && !typed_uav_load )
		if( gl_LocalInvocationIndex & 0x1u )
			g_voxelAccumValue[gl_LocalInvocationIndex >> 1u] = 0;
		GroupMemoryBarrier();

		origAccumHitsVal = uint( accumHits + origAccumHits );
		origAccumHitsVal = (gl_LocalInvocationIndex & 0x1u) ? (origAccumHitsVal << 16u) :
															  origAccumHitsVal;
		InterlockedOr( g_voxelAccumValue[gl_LocalInvocationIndex >> 1u], origAccumHitsVal );
		if( gl_LocalInvocationIndex & 0x1u )
		{
			OGRE_imageWrite3D1( voxelAccumVal, wshort3( voxelCelUvw.x >> 1u, voxelCelUvw.yz ),
								uint4( g_voxelAccumValue[gl_LocalInvocationIndex >> 1u], 0, 0, 0 ) );
		}
	@end
@end
