/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef __FrameStats_H__
#define __FrameStats_H__

#include "OgreMemoryAllocatorConfig.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup General
     *  @{
     */

    /// All returned values are either in seconds or frames per second.
    class _OgreExport FrameStats : public OgreAllocatedObj
    {
        uint64 mLastTimeAbsoluteUS;  /// Last sample in absolute time, in microseconds.

        double mNumSamples;

        double mLatestTimeSinceLast;
        double mRollingAvg;
        double mAllTimeAvg;

        double mBestFrameTime;
        double mWorstFrameTime;

        // For percentile calculation
        static constexpr uint32 kNumFrameStats = 1024u;
        uint32                  mFramesCount[kNumFrameStats];

        inline double getPercentileNthFrames( const double percentile ) const
        {
            const uint64 sampleCountToLookFor =
                static_cast<uint64>( mNumSamples * ( 1.0 - percentile ) );

            uint64 samplesSoFar = 0u;
            for( size_t i = 0u; i < kNumFrameStats; ++i )
            {
                samplesSoFar += mFramesCount[i];
                if( samplesSoFar >= sampleCountToLookFor )
                {
                    const double frameTime = 2.5 / double( i + 1u );
                    return frameTime;
                }
            }

            return -1.0;  // Should not happen
        }

        inline double getPercentileNthTime( const double percentile ) const
        {
            const double totalTimeTaken = mNumSamples * mAllTimeAvg;
            const double accumTimeToLookFor = totalTimeTaken * ( 1.0 - percentile );

            double timeSoFar = 0.0;
            for( size_t i = 0u; i < kNumFrameStats; ++i )
            {
                const double frameTime = 2.5 / double( i + 1u );
                timeSoFar += mFramesCount[i] * frameTime;
                if( timeSoFar >= accumTimeToLookFor )
                    return frameTime;
            }

            return -1.0;  // Should not happen
        }

    public:
        FrameStats() { reset( 0 ); }

        /** Returns latest record of the time since last measurement.
            i.e. the time spent between the last two addSample() calls.

            This is the same as getRollingAverage() except this value is too noisy.
        @remarks
            Multiply by 1000.0 to get the time in milliseconds per frame.
        @return
            Time in seconds spent between the last two addSample() calls.
        */
        double getLatestTimeSinceLast() const { return mLatestTimeSinceLast; }

        /// Same as getLatestTimeSinceLast() but expressed as frames per second instead of seconds.
        double getLatestTimeSinceLastFps() const { return 1.0 / mLatestTimeSinceLast; }

        /** Returns the rolling average. This is what you want to display for an FPS counter;
            Since the framerate during the last few seconds are what matter the most.
        @remarks
            Multiply by 1000.0 to get the time in milliseconds per frame.
        @return
            Average time in seconds.
        */
        double getRollingAverage() const { return mRollingAvg; }

        /** Same as getRollingAverage(), but expressed as frames per second instead of seconds.
        @return
            Average time in FPS.
        */
        double getRollingAverageFps() const { return 1.0 / mRollingAvg; }

        /** Returns the average since the last reset(). This is what you want if you're benchmarking an
            entire session, and want to know the average of all samples.
        @remarks
            Multiply by 1000.0 to get the time in milliseconds per frame.
        @return
            Average time since the last reset(), in seconds.
        */
        double getAllTimeAverage() const { return mAllTimeAvg; }

        /** Same as getAllTimeAverage(), but expressed as frames per second instead of seconds.
        @return
            Average time in FPS.
        */
        double getAllTimeAverageFps() const { return 1.0 / mAllTimeAvg; }

        /** Gets the 95th percentile, since the last reset().
        @remark
            For memory constraint reasons, we track framerates in 0.4 steps.
            Which means we will report percentiles of 0.4fps, 0.8fps, 1.2fps, ... 60.0, 60.4 and so on.
            <br/>
            If the framerate often stays above 409.6 fps, we may not be able to
            accurately calculate the percentiles.
        @remark
            Multiply by 1000.0 to get the time in milliseconds per frame.
            Perform 1.0 / getPercentile95th() to get the value in frames per second.
        @param bUseTime
            @parblock
            When true, we divide the total rendering time in percentiles.

            The advantage is that it is more homogeneous and biases towards slow frames.
            The disadvantage is that slow loading times can skew the results too much.

            When false, we divide the total number of samples in percentiles.

            The disadvantage is that a slow frame (i.e. spikes) matter less when running at
            144hz than when running at 60hz.

            See https://www.yosoygames.com.ar/wp/2023/09/youre-calculating-framerate-percentiles-wrong/
            @endparblock
        @return
            The frametime in seconds at the requested percentile.
        */
        double getPercentile95th( bool bUseTime ) const { return getPercentileNth( 0.95, bUseTime ); }

        /// Gets the 99th percentile, since the last reset. See getPercentile95th().
        double getPercentile99th( bool bUseTime ) const { return getPercentileNth( 0.99, bUseTime ); }

        /** Gets the desired percentile, since the last reset. See getPercentile95th().
        @param percentile
            Value in range (0; 1)
        @param bUseTime
            See getPercentile95th().
        @return
            See getPercentile95th().
        */
        double getPercentileNth( double percentile, bool bUseTime ) const
        {
            if( bUseTime )
                return getPercentileNthTime( percentile );
            else
                return getPercentileNthFrames( percentile );
        }

        /** Returns the best frame time in seconds since the last reset().
        @remarks
            Multiply by 1000.0 to get the time in milliseconds per frame.
        @return
            Best frame time in seconds.
        */
        double getBestTime() const { return mBestFrameTime; }

        /** Returns the worst frame time in seconds since the last reset().
        @remarks
            Multiply by 1000.0 to get the time in milliseconds per frame.
        @return
            Worst frame time in seconds.
        */
        double getWorstTime() const { return mWorstFrameTime; }

        uint64 getLastTimeRawMicroseconds() const { return mLastTimeAbsoluteUS; }

        /// Adds a new measured time, in *microseconds*
        void addSample( const uint64 timeUs )
        {
            const double timeSinceLast = static_cast<double>( timeUs - mLastTimeAbsoluteUS ) / 1000000;

            const double w = exp( -timeSinceLast ) * 0.97;

            mLatestTimeSinceLast = timeSinceLast;
            mRollingAvg = mRollingAvg * w + timeSinceLast * ( 1.0 - w );
            mAllTimeAvg = ( ( mAllTimeAvg * mNumSamples ) + timeSinceLast ) / ( mNumSamples + 1.0 );

            mBestFrameTime = std::min( timeSinceLast, mBestFrameTime );
            mWorstFrameTime = std::max( timeSinceLast, mWorstFrameTime );

            const uint32_t asRoundedFps = std::min(
                static_cast<uint32_t>( std::max( std::round( 2.5 / timeSinceLast ), 1.0 ) - 1u ),
                kNumFrameStats - 1u );
            ++mFramesCount[asRoundedFps];
            ++mNumSamples;

            mLastTimeAbsoluteUS = timeUs;
        }

        void reset( uint64 timeUs )
        {
            mLastTimeAbsoluteUS = timeUs;

            mNumSamples = 0.0;
            // Prevent division by zero using a sensible default.
            // Because mNumSamples = 0, mAllTimeAvg is not actually affected by this trick.
            // mLatestTimeSinceLast will be overwritten on next sample.
            // And mRollingAvg should converge quickly over time to its real value.
            mLatestTimeSinceLast = 1.0 / 60.0;
            mRollingAvg = 1.0 / 60.0;
            mAllTimeAvg = 1.0 / 60.0;

            mBestFrameTime = std::numeric_limits<double>::max();
            mWorstFrameTime = 0;

            memset( mFramesCount, 0, sizeof( mFramesCount ) );
        }

        /// @deprecated since 4.0 in favour of getLatestTimeSinceLastFps().
        OGRE_DEPRECATED_VER( 4 ) float getFps() const
        {
            return static_cast<float>( getLatestTimeSinceLastFps() );
        }

        /// @deprecated since 4.0 in favour of getRollingAverageFps().
        OGRE_DEPRECATED_VER( 4 ) float getAvgFps() const
        {
            return static_cast<float>( getRollingAverageFps() );
        }

        /// @deprecated since 4.0 in favour of getLatestTimeSinceLast().
        OGRE_DEPRECATED_VER( 4 ) double getLastTime() const { return getLatestTimeSinceLast() * 1000.0; }

        /// @deprecated since 4.0 in favour of getRollingAverage().
        OGRE_DEPRECATED_VER( 4 ) float getAvgTime() const
        {
            return static_cast<float>( getRollingAverage() * 1000.0 );
        }
    };

}  // Namespace Ogre
#endif
