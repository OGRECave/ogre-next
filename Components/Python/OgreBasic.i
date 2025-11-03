%include "OgrePrerequisites.h"
%template(NameValuePairList) std::map<Ogre::String, Ogre::String>;

%ignore Ogre::HashedVector::operator=;
%ignore Ogre::TRect::operator=;
%include "OgreCommon.h"
%template(Rect) Ogre::TRect<long>;
%template(FloatRect) Ogre::TRect<float>;

%include "OgreMemoryAllocatorConfig.h"

%warnfilter(509) Ogre::IdObject::operator();
%include "OgreId.h"
%ignore Ogre::IdString::Seed; // is static const
%include "OgreIdString.h"

%include "OgreStringInterface.h"
%include "OgreStringVector.h"
%template(StringVector) std::vector<Ogre::String>;  // actual vector<T>
%template(StringVectorPtr) Ogre::SharedPtr<std::vector<Ogre::String> >;

%ignore Ogre::Any::operator=;
%ignore Ogre::any_cast;
%include "OgreAny.h"


%ignore Ogre::Angle;
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Radian, Radian, =, const Degree&, set)
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Radian, Radian, =, const Real&, set)
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Degree, Degree, =, const Radian&, set)
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Degree, Degree, =, const Real&, set)
%include "OgreMath.h"
ADD_REPR(Radian)
ADD_REPR(Degree)


%warnfilter(509) Ogre::Vector2::Vector2;
%warnfilter(509) Ogre::Vector2::operator[];
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Vector2, Vector2, =, const Real, set)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, *, const Vector2&,  Vector2, multi)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, /, const Vector2&,  Vector2, div)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, +, const Vector2&,  Vector2, add)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, -, const Vector2&,  Vector2, sub)
%ignore operator+( const Vector2&, const Real );
%ignore operator-( const Vector2&, const Real );
%include "OgreVector2.h"
ADD_REPR(Vector2)

%warnfilter(509) Ogre::Vector3::Vector3;
%warnfilter(509) Ogre::Vector3::operator[];
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Vector3, Vector3, =, const Real, set)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, *, const Vector3&,  Vector3, multi)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, /, const Vector3&,  Vector3, div)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, +, const Vector3&,  Vector3, add)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, -, const Vector3&,  Vector3, sub)
%ignore operator+( const Vector3&, const Real );
%ignore operator-( const Vector3&, const Real );
%include "OgreVector3.h"
ADD_REPR(Vector3)

%warnfilter(509) Ogre::Vector4::Vector4;
%warnfilter(509) Ogre::Vector4::operator[];
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Vector4, Vector4, =, const Real, set)
CLASS_OPERATOR_TO_MEMBER_FUNCTION(Vector4, Vector4, =, const Vector3&, set)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, *, const Vector4&,  Vector4, multi)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, /, const Vector4&,  Vector4, div)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, +, const Vector4&,  Vector4, add)
OPERATOR_TO_MEMBER_FUNCTION_SHORT(const Real, -, const Vector4&,  Vector4, sub)
%ignore operator+( const Vector4&, const Real );
%ignore operator-( const Vector4&, Real );
%include "OgreVector4.h"
ADD_REPR(Vector4)

OPERATOR_TO_MEMBER_FUNCTION(Vector3, const Vector3&, *, const Matrix3&,  Matrix3, multi)
OPERATOR_TO_MEMBER_FUNCTION(Matrix3, Real, *, const Matrix3&,  Matrix3, multi)
%include "OgreMatrix3.h"
ADD_REPR(Matrix3)

CLASS_OPERATOR_TO_MEMBER_FUNCTION(void, Matrix4, =, const Matrix3&, set)
%include "OgreMatrix4.h"
ADD_REPR(Matrix4)

%warnfilter(509) Ogre::Quaternion::operator[];
%rename(Real_Multiply_Quaternion) operator*( Real, const Quaternion& );
%include "OgreQuaternion.h"
ADD_REPR(Quaternion)

%warnfilter(509) Ogre::ColourValue::operator[];
%ignore operator*( const float, const ColourValue& );
%include "OgreColourValue.h"
ADD_REPR(ColourValue)

%include "Math/Simple/OgreAabb.h"

CLASS_OPERATOR_TO_MEMBER_FUNCTION(AxisAlignedBox, AxisAlignedBox, =, const AxisAlignedBox&, set)
%include "OgreAxisAlignedBox.h"

%include "OgreRay.h"
%include "OgrePlaneBoundedVolume.h"
%include "OgreSceneQuery.h"
%template(RaySceneQueryResult) std::vector<Ogre::RaySceneQueryResultEntry>;

%ignore Ogre::Exception::operator=;
%rename(OgreException) Ogre::Exception; // conflicts with Python Exception
%include "OgreException.h"

%ignore Ogre::SharedPtr::operator=;
%include "OgreSharedPtr.h" 

%ignore Ogre::ResourceAccess::toString;
%include "OgreIteratorWrapper.h"
%include "OgreResourceTransition.h"
