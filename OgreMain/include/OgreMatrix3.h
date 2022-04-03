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
#ifndef __Matrix3_H__
#define __Matrix3_H__

#include "OgrePrerequisites.h"

#include "OgreVector3.h"

// NB All code adapted from Wild Magic 0.2 Matrix math (free source code)
// http://www.geometrictools.com/

// NOTE.  The (x,y,z) coordinate system is assumed to be right-handed.
// Coordinate axis rotation matrices are of the form
//   RX =    1       0       0
//           0     cos(t) -sin(t)
//           0     sin(t)  cos(t)
// where t > 0 indicates a counterclockwise rotation in the yz-plane
//   RY =  cos(t)    0     sin(t)
//           0       1       0
//        -sin(t)    0     cos(t)
// where t > 0 indicates a counterclockwise rotation in the zx-plane
//   RZ =  cos(t) -sin(t)    0
//         sin(t)  cos(t)    0
//           0       0       1
// where t > 0 indicates a counterclockwise rotation in the xy-plane.

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Math
     *  @{
     */
    /** A 3x3 matrix which can represent rotations around axes.
        @note
            <b>All the code is adapted from the Wild Magic 0.2 Matrix
            library (http://www.geometrictools.com/).</b>
        @par
            The coordinate system is assumed to be <b>right-handed</b>.
    */
    class _OgreExport Matrix3
    {
    public:
        /** Default constructor.
            @note
                It does <b>NOT</b> initialize the matrix for efficiency.
        */
        inline Matrix3() {}
        inline explicit Matrix3( const Real arr[3][3] ) { memcpy( m, arr, 9 * sizeof( Real ) ); }

        Matrix3( Real fEntry00, Real fEntry01, Real fEntry02,  //
                 Real fEntry10, Real fEntry11, Real fEntry12,  //
                 Real fEntry20, Real fEntry21, Real fEntry22 )
        {
            m[0][0] = fEntry00;
            m[0][1] = fEntry01;
            m[0][2] = fEntry02;
            m[1][0] = fEntry10;
            m[1][1] = fEntry11;
            m[1][2] = fEntry12;
            m[2][0] = fEntry20;
            m[2][1] = fEntry21;
            m[2][2] = fEntry22;
        }

        /** Exchange the contents of this matrix with another.
         */
        inline void swap( Matrix3 &other )
        {
            std::swap( m[0][0], other.m[0][0] );
            std::swap( m[0][1], other.m[0][1] );
            std::swap( m[0][2], other.m[0][2] );
            std::swap( m[1][0], other.m[1][0] );
            std::swap( m[1][1], other.m[1][1] );
            std::swap( m[1][2], other.m[1][2] );
            std::swap( m[2][0], other.m[2][0] );
            std::swap( m[2][1], other.m[2][1] );
            std::swap( m[2][2], other.m[2][2] );
        }

        /// Member access, allows use of construct mat[r][c]
        inline const Real *operator[]( size_t iRow ) const { return m[iRow]; }

        inline Real *operator[]( size_t iRow ) { return m[iRow]; }

        /*inline operator Real* ()
        {
            return (Real*)m[0];
        }*/
        Vector3 GetColumn( size_t iCol ) const;

        void SetColumn( size_t iCol, const Vector3 &vec );
        void FromAxes( const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis );

        /** Tests 2 matrices for equality.
         */
        bool operator==( const Matrix3 &rkMatrix ) const;

        /** Tests 2 matrices for inequality.
         */
        inline bool operator!=( const Matrix3 &rkMatrix ) const { return !operator==( rkMatrix ); }

        // arithmetic operations
        /** Matrix addition.
         */
        Matrix3 operator+( const Matrix3 &rkMatrix ) const;

        /** Matrix subtraction.
         */
        Matrix3 operator-( const Matrix3 &rkMatrix ) const;

        /** Matrix concatenation using '*'.
         */
        Matrix3 operator*( const Matrix3 &rkMatrix ) const;
        Matrix3 operator-() const;

        /// Matrix * vector [3x3 * 3x1 = 3x1]
        Vector3 operator*( const Vector3 &rkVector ) const;

        /// Vector * matrix [1x3 * 3x3 = 1x3]
        _OgreExport friend Vector3 operator*( const Vector3 &rkVector, const Matrix3 &rkMatrix );

        /// Matrix * scalar
        Matrix3 operator*( Real fScalar ) const;

        /// Scalar * matrix
        _OgreExport friend Matrix3 operator*( Real fScalar, const Matrix3 &rkMatrix );

        // utilities
        Matrix3 Transpose() const;
        bool    Inverse( Matrix3 &rkInverse, Real fTolerance = Real( 1e-06 ) ) const;
        Matrix3 Inverse( Real fTolerance = Real( 1e-06 ) ) const;
        Real    Determinant() const;

        /// Singular value decomposition
        void SingularValueDecomposition( Matrix3 &rkL, Vector3 &rkS, Matrix3 &rkR ) const;
        void SingularValueComposition( const Matrix3 &rkL, const Vector3 &rkS, const Matrix3 &rkR );

        /// Gram-Schmidt orthonormalization (applied to columns of rotation matrix)
        void Orthonormalize();

        /// Orthogonal Q, diagonal D, upper triangular U stored as (u01,u02,u12)
        void QDUDecomposition( Matrix3 &rkQ, Vector3 &rkD, Vector3 &rkU ) const;

        Real SpectralNorm() const;

        /// Note: Matrix must be orthonormal
        void        ToAngleAxis( Vector3 &rkAxis, Radian &rfAngle ) const;
        inline void ToAngleAxis( Vector3 &rkAxis, Degree &rfAngle ) const
        {
            Radian r;
            ToAngleAxis( rkAxis, r );
            rfAngle = r;
        }
        void FromAngleAxis( const Vector3 &rkAxis, const Radian &fRadians );

        /** The matrix must be orthonormal.  The decomposition is yaw*pitch*roll
            where yaw is rotation about the Up vector, pitch is rotation about the
            Right axis, and roll is rotation about the Direction axis. */
        bool ToEulerAnglesXYZ( Radian &rfYAngle, Radian &rfPAngle, Radian &rfRAngle ) const;
        bool ToEulerAnglesXZY( Radian &rfYAngle, Radian &rfPAngle, Radian &rfRAngle ) const;
        bool ToEulerAnglesYXZ( Radian &rfYAngle, Radian &rfPAngle, Radian &rfRAngle ) const;
        bool ToEulerAnglesYZX( Radian &rfYAngle, Radian &rfPAngle, Radian &rfRAngle ) const;
        bool ToEulerAnglesZXY( Radian &rfYAngle, Radian &rfPAngle, Radian &rfRAngle ) const;
        bool ToEulerAnglesZYX( Radian &rfYAngle, Radian &rfPAngle, Radian &rfRAngle ) const;
        void FromEulerAnglesXYZ( const Radian &fYAngle, const Radian &fPAngle, const Radian &fRAngle );
        void FromEulerAnglesXZY( const Radian &fYAngle, const Radian &fPAngle, const Radian &fRAngle );
        void FromEulerAnglesYXZ( const Radian &fYAngle, const Radian &fPAngle, const Radian &fRAngle );
        void FromEulerAnglesYZX( const Radian &fYAngle, const Radian &fPAngle, const Radian &fRAngle );
        void FromEulerAnglesZXY( const Radian &fYAngle, const Radian &fPAngle, const Radian &fRAngle );
        void FromEulerAnglesZYX( const Radian &fYAngle, const Radian &fPAngle, const Radian &fRAngle );
        /// Eigensolver, matrix must be symmetric
        void EigenSolveSymmetric( Real afEigenvalue[3], Vector3 akEigenvector[3] ) const;

        static void TensorProduct( const Vector3 &rkU, const Vector3 &rkV, Matrix3 &rkProduct );

        /** Determines if this matrix involves a scaling. */
        inline bool hasScale() const
        {
            // check magnitude of column vectors (==local axes)
            Real t = m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0];
            if( !Math::RealEqual( t, 1.0, (Real)1e-04 ) )
                return true;
            t = m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1];
            if( !Math::RealEqual( t, 1.0, (Real)1e-04 ) )
                return true;
            t = m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2];
            if( !Math::RealEqual( t, 1.0, (Real)1e-04 ) )
                return true;

            return false;
        }

        /** Function for writing to a stream.
         */
        _OgreExport friend std::ostream &operator<<( std::ostream &o, const Matrix3 &mat );

        static const Real    EPSILON;
        static const Matrix3 ZERO;
        static const Matrix3 IDENTITY;

    protected:
        // support for eigensolver
        void Tridiagonal( Real afDiag[3], Real afSubDiag[3] );
        bool QLAlgorithm( Real afDiag[3], Real afSubDiag[3] );

        // support for singular value decomposition
        static const unsigned int msSvdMaxIterations;

        static void Bidiagonalize( Matrix3 &kA, Matrix3 &kL, Matrix3 &kR );
        static void GolubKahanStep( Matrix3 &kA, Matrix3 &kL, Matrix3 &kR );

        // support for spectral norm
        static Real MaxCubicRoot( Real afCoeff[3] );

        Real m[3][3];

        // for faster access
        friend class Matrix4;
    };
    /** @} */
    /** @} */
}  // namespace Ogre
#endif
