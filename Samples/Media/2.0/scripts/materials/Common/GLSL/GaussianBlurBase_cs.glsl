@property( syntax != glslvk )
	#version 430
@else
	#version 450
@end

//Based on GPUOpen's samples SeparableFilter11
//https://github.com/GPUOpen-LibrariesAndSDKs/SeparableFilter11
//For better understanding, read "Efficient Compute Shader Programming" from Bill Bilodeau
//http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2012/10/Efficient%20Compute%20Shader%20Programming.pps

//TL;DR:
//	* Each thread works on 4 pixels at a time (for VLIW hardware, i.e. Radeon HD 5000 & 6000 series).
//	* 256 pixels per threadgroup. Each threadgroup works on 2 rows of 128 pixels each.
//	  That means 32x2 threads = 64. 64 threads x 4 pixels per thread = 256

// For this shader to work, several pieces need to be defined:
//	data_type (i.e. vec3)
//	lds_data_type (i.e. vec3, uint)
//	lds_definition
//	image_store
//	image_sample
//	decode_lds (optional, i.e. when lds_data_type != data_type)
//	Define the property "downscale" if you're doing a downsample.
//	Define "downscale_lq" (must also define downscale) for SLIGHTLY lower quality downscale
// The script uses the template syntax to automatically set the num. of threadgroups
// based on the bound input texture.

vulkan( layout( ogre_s0 ) uniform sampler inputSampler );
@property( texture0_texture_type == TextureTypes_Type2DArray )
	vulkan_layout( ogre_t0 ) uniform texture2DArray inputImage;
@else
	vulkan_layout( ogre_t0 ) uniform texture2D inputImage;
@end

layout( vulkan( ogre_u0 ) vk_comma @insertpiece(uav0_pf_type) )
@property( uav0_texture_type == TextureTypes_Type2DArray )
	uniform restrict writeonly image2DArray outputImage;
@else
	uniform restrict writeonly image2D outputImage;
@end

// 32 = 128 / 4
layout( local_size_x = 32,
		local_size_y = 2,
		local_size_z = 1 ) in;
@pset( threads_per_group_x, 32 )
@pset( threads_per_group_y, 2 )
@pset( threads_per_group_z, 1 )

@pmul( pixelsPerRow, threads_per_group_x, 4 )
@pset( rowsPerThreadGroup, threads_per_group_y )
@pset( num_thread_groups_z, 1 )

@set( input_width, uav0_width_with_lod )
@set( input_height, uav0_height_with_lod )

@property( horizontal_pass )
	@property( downscale ) @mul( input_width, 2 ) @end

	/// Calculate num_thread_groups_
	/// num_thread_groups_x = (texture0_width + pixelsPerRow - 1) / pixelsPerRow
	/// num_thread_groups_y = (texture0_height + rowsPerThreadGroup - 1) / rowsPerThreadGroup
	@add( num_thread_groups_x, input_width, pixelsPerRow )
	@sub( num_thread_groups_x, 1 )
	@div( num_thread_groups_x, pixelsPerRow )

	@add( num_thread_groups_y, input_height, rowsPerThreadGroup )
	@sub( num_thread_groups_y, 1 )
	@div( num_thread_groups_y, rowsPerThreadGroup )
@end @property( !horizontal_pass )
	@property( downscale ) @mul( input_height, 2 ) @end

	/// Calculate num_thread_groups_
	/// num_thread_groups_x = (texture0_width + rowsPerThreadGroup - 1) / rowsPerThreadGroup
	/// num_thread_groups_y = (texture0_height + pixelsPerRow - 1) / pixelsPerRow
	@add( num_thread_groups_x, input_width, rowsPerThreadGroup )
	@sub( num_thread_groups_x, 1 )
	@div( num_thread_groups_x, rowsPerThreadGroup )

	@add( num_thread_groups_y, input_height, pixelsPerRow )
	@sub( num_thread_groups_y, 1 )
	@div( num_thread_groups_y, pixelsPerRow )
@end

/// shared vec3 g_f3LDS[ 2 ] [ @value( samples_per_threadgroup ) ];
@insertpiece( lds_definition )

#define C_WEIGHTS( x ) c_weights[(x) >> 2u][(x) & 3u]

// weights_array_count = ( kernel_radius + 1u + 3u ) / 4u
@padd( weights_array_count, kernel_radius, 4 )
@pdiv( weights_array_count,  4 )

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec4 g_f4OutputSize;
	uniform vec4 c_weights[@value( weights_array_count )];
	@insertpiece( extra_params )
vulkan( }; )

@insertpiece( lds_data_type ) sampleTex( ivec2 i2Position , vec2 f2Offset )
{
	vec2 f2SamplePosition = vec2( i2Position ) + vec2( 0.5f, 0.5f );

	f2SamplePosition *= g_f4OutputSize.zw;
	///return textureLod( inputImage, f2SamplePosition, 0 ).xyz;
	@insertpiece( image_sample )
}

void ComputeFilterKernel( int iPixelOffset, int iLineOffset, ivec2 i2Center, ivec2 i2Inc )
{
	@property( !downscale_lq )
		@insertpiece( data_type ) outColour[ 4 ];
	@end @property( downscale_lq )
		@insertpiece( data_type ) outColour[ 2 ];
	@end
	@insertpiece( data_type ) RDI[ 4 ] ;

	@foreach( 4, iPixel )
		RDI[ @iPixel ] = @insertpiece( decode_lds )( g_f3LDS[ iLineOffset ][ iPixelOffset + @value( kernel_radius ) + @iPixel ] );@end

	@property( !downscale_lq )
		@foreach( 4, iPixel )
			outColour[ @iPixel ] = RDI[ @iPixel ] * C_WEIGHTS( @value( kernel_radius ) );@end
	@end @property( downscale_lq )
		@foreach( 2, iPixel )
			outColour[ @iPixel ] = RDI[ @iPixel * 2 ] * C_WEIGHTS( @value( kernel_radius ) );@end
	@end

	@foreach( 4, iPixel )
		RDI[ @iPixel ] = @insertpiece( decode_lds )( g_f3LDS[ iLineOffset ][ iPixelOffset + @iPixel ] );@end

	iPixelOffset += 4;

	/// Deal with taps to our left.
	/// for ( iIteration = 0; iIteration < radius; iIteration += 1 )
	@foreach( kernel_radius, iIteration )
		@property( !downscale_lq )
			@foreach( 4, iPixel )
				outColour[ @iPixel ] += RDI[ @iPixel ] * C_WEIGHTS( @iIteration );@end
		@end @property( downscale_lq )
			@foreach( 2, iPixel )
				outColour[ @iPixel ] += RDI[ @iPixel * 2 ] * C_WEIGHTS( @iIteration );@end
		@end
		@foreach( 3, iPixel )
			RDI[ @iPixel ] = RDI[ @iPixel + ( 1 ) ];@end
		@foreach( 1, iPixel )
			RDI[ 4 - 1 + @iPixel ] = @insertpiece( decode_lds )( g_f3LDS[ iLineOffset ][ iPixelOffset + @iIteration + @iPixel ] );@end
	@end

	@foreach( 4, iPixel )
		RDI[ @iPixel ] = @insertpiece( decode_lds )( g_f3LDS[ iLineOffset ][ iPixelOffset - 4 + @value( kernel_radius ) + 1 + @iPixel ] );@end

	@padd( kernel_radius_plus1, kernel_radius, 1 )
	@pmul( kernel_radius2x_plus1, kernel_radius, 2 )
	@padd( kernel_radius2x_plus1, 1 )

	@pmul( kernel_radius2x, kernel_radius, 2 )

	/// Deal with taps to our right.
	/// for ( iIteration = radius + 1; iIteration < ( radius * 2 + 1 ); iIteration += 1 )
	@foreach( kernel_radius2x_plus1, iIteration, kernel_radius_plus1 )
		@property( !downscale_lq )
			@foreach( 4, iPixel )
				outColour[ @iPixel ] += RDI[ @iPixel ] * C_WEIGHTS( @value( kernel_radius2x ) - @iIteration );@end
		@end @property( downscale_lq )
			@foreach( 2, iPixel )
				outColour[ @iPixel ] += RDI[ @iPixel * 2 ] * C_WEIGHTS( @value( kernel_radius2x ) - @iIteration );@end
		@end
		@foreach( 3, iPixel )
			RDI[ @iPixel ] = RDI[ @iPixel + ( 1 ) ];@end
		@foreach( 1, iPixel )
			RDI[ 4 - 1 + @iPixel ] = @insertpiece( decode_lds )( g_f3LDS[ iLineOffset ][ iPixelOffset + @iIteration + @iPixel ] );@end
	@end

	/*
	foreach( 4, iPixel )
		imageStore( outputImage, ivec2( i2Center +  iPixel * i2Inc ), vec4( outColour[ iPixel ], 1.0 ) );end
	*/
	@insertpiece( image_store )
}

void main()
{
	/// samples_per_threadgroup =   128 + ( ( kernel_radius * 2 + 1 ) - 1 )
	/// samples_per_thread		= ( 128 + ( ( kernel_radius * 2 + 1 ) - 1 ) ) / ( 128 / 4 )
	@padd( samples_per_threadgroup, 127, kernel_radius2x_plus1 )
	@pdiv( samples_per_thread, samples_per_threadgroup, 32 )

@property( horizontal_pass )
	int iSampleOffset	= int( gl_LocalInvocationID.x * @value( samples_per_thread ) );
	int iLineOffset		= int( gl_LocalInvocationID.y );

	ivec2 i2GroupCoord	= ivec2( ( gl_WorkGroupID.x << 7u ) - @value( kernel_radius )u, gl_WorkGroupID.y << 1u );
	ivec2 i2Coord		= ivec2( i2GroupCoord.x + iSampleOffset, i2GroupCoord.y );

	@foreach( samples_per_thread, i )
		g_f3LDS[ iLineOffset ][ iSampleOffset + @i ] = sampleTex( i2Coord + ivec2( @i, gl_LocalInvocationID.y ) , vec2( 0.5f, 0.0f ) );@end

	if( gl_LocalInvocationID.x < @value( samples_per_threadgroup )u - 32u * @value( samples_per_thread )u )
	{
		g_f3LDS[ iLineOffset ][ @value(samples_per_threadgroup)u - 1u - gl_LocalInvocationID.x ] =
				sampleTex( i2GroupCoord + ivec2( @value(samples_per_threadgroup)u - 1u - gl_LocalInvocationID.x, gl_LocalInvocationID.y ), vec2( 0.5f, 0.0f ) );
	}

	//memoryBarrierShared ensures our write is visible to everyone else (must be done BEFORE the barrier)
	//barrier ensures every thread's execution reached here.
	memoryBarrierShared();
	barrier();

	int iPixelOffset = int( gl_LocalInvocationID.x << 2u ); //gl_LocalInvocationID.x * 4u
	i2Coord = ivec2( i2GroupCoord.x + iPixelOffset, i2GroupCoord.y );
	i2Coord.x += @value( kernel_radius );

	if( i2Coord.x < int(g_f4OutputSize.x) )
	{
		ivec2 i2Center	= i2Coord + ivec2( 0, gl_LocalInvocationID.y );
		ivec2 i2Inc		= ivec2 ( 1, 0 );

		@property( downscale )
			i2Center.x = int( uint( i2Center.x ) >> 1u );
		@end

		ComputeFilterKernel( iPixelOffset, iLineOffset, i2Center, i2Inc );
	}
@end @property( !horizontal_pass )
	int iSampleOffset	= int( gl_LocalInvocationID.x * @value( samples_per_thread ) );
	int iLineOffset		= int( gl_LocalInvocationID.y );

	ivec2 i2GroupCoord	= ivec2( gl_WorkGroupID.x << 1u, ( gl_WorkGroupID.y << 7u ) - @value( kernel_radius )u );
	ivec2 i2Coord		= ivec2( i2GroupCoord.x, i2GroupCoord.y + iSampleOffset );

	@foreach( samples_per_thread, i )
		g_f3LDS[ iLineOffset ][ iSampleOffset + @i ] = sampleTex( i2Coord + ivec2( gl_LocalInvocationID.y, @i ) , vec2( 0.0f, 0.5f ) );@end

	if( gl_LocalInvocationID.x < @value( samples_per_threadgroup )u - 32u * @value( samples_per_thread )u )
	{
		g_f3LDS[ iLineOffset ][ @value(samples_per_threadgroup)u - 1u - gl_LocalInvocationID.x ] =
				sampleTex( i2GroupCoord + ivec2( gl_LocalInvocationID.y, @value(samples_per_threadgroup)u - 1u - gl_LocalInvocationID.x ), vec2( 0.0f, 0.5f ) );
	}

	//memoryBarrierShared ensures our write is visible to everyone else (must be done BEFORE the barrier)
	//barrier ensures every thread's execution reached here.
	memoryBarrierShared();
	barrier();

	int iPixelOffset = int( gl_LocalInvocationID.x << 2u ); //gl_LocalInvocationID.x * 4u
	i2Coord = ivec2( i2GroupCoord.x, i2GroupCoord.y + iPixelOffset );
	i2Coord.y += @value( kernel_radius );

	if( i2Coord.y < int(g_f4OutputSize.y) )
	{
		ivec2 i2Center	= i2Coord + ivec2( gl_LocalInvocationID.y, 0 );
		ivec2 i2Inc		= ivec2 ( 0, 1 );

		@property( downscale )
			i2Center.y = int( uint( i2Center.y ) >> 1u );
		@end

		ComputeFilterKernel( iPixelOffset, iLineOffset, i2Center, i2Inc );
	}
@end
}
