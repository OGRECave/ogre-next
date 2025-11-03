%module(directors="1") Ogre

// SWIG setting for expose Python API
%include PythonAPI.i

// Global rename politics:
// - rename all nested classes "Ogre::A::B" into "A_B"
%rename("%(regex:/^(Ogre)?(::)?([^:]+)(::)?(.*?)$/\\3_\\5/)s", fullname=1, %$isnested, regextarget=1) "^Ogre::[^:]+::";
// - rename elements from Ogre::XX namespace to XX_{element_name},
%rename("%(regex:/^(Ogre)?(::)?([^:]+)(::)?(.*?)$/\\3_\\5/)s", fullname=1, %$not %$ismember, %$not %$isconstructor, %$not %$isdestructor, regextarget=1) "^Ogre::[^:<]+::[^:<]+";


// STD
%include std_shared_ptr.i
%include std_string.i
%include std_pair.i
%include std_deque.i
%include std_list.i
%include std_map.i
%include std_set.i
%include std_multimap.i
%include std_unordered_map.i
%include std_unordered_set.i
%include std_vector.i
%include exception.i
%include typemaps.i
%include stdint.i

// Ogre STD
%include ogrestd/deque.h
%include ogrestd/list.h
%include ogrestd/map.h
%include ogrestd/set.h
%include ogrestd/unordered_map.h
%include ogrestd/unordered_set.h
%include ogrestd/vector.h


// define some macros (to make possible parsing headers via %include)
%include "OgrePlatform.h"
#define _OgreExport
#define _OgrePrivate
#define OGRE_DEPRECATED
#undef  FORCEINLINE
#define FORCEINLINE
#define final
#define noexcept(a)

// add headers and some definition to output C++ file
%{
	#include "Ogre.h"
	using namespace Ogre;
	
	#ifdef __GNUC__
	#pragma GCC diagnostic ignored "-Wcast-function-type"
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	#pragma GCC diagnostic ignored "-Wunused-label"
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
	#pragma GCC diagnostic ignored "-Wunused-variable"
	#endif
	
	#include <sstream>
%}


// basic Ogre types
%include "OgreBasic.i"

// node, scene node, movable object

%include "OgreNode.h"
%include "OgreSceneNode.h"
%ignore Ogre::MovableObject::calculateCameraDistance; // is not defined (FIXME why public member is 'static inline'?)
%include "OgreMovableObject.h"
%include "OgreUserObjectBindings.h"

// scene manager

%{
	#include "OgreSceneManager.h"
	typedef SceneManager::RenderContext RenderContext;
%}
%ignore Ogre::SceneManager::updateAllBounds;
%ignore Ogre::SceneManager::getCameras; // FIXME protected typedef for CameraList
%ignore Ogre::SceneManager::getCameraIterator;
%ignore Ogre::SceneManager::getAnimations;
%ignore Ogre::SceneManager::getAnimationIterator;
%ignore Ogre::SceneManager::getAnimationStateIterator;
%include "OgreSceneManager.h"

// compositor

%include "initHLMS.i" // TODO see comment in this file

%{
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorNode.h>
%}
%ignore Ogre::CompositorManager2::connectOutput; // FIXME is not defined (only declaration in OgreCompositorManager2.h file)
%include "Compositor/OgreCompositorManager2.h"
%include "Compositor/OgreCompositorWorkspace.h"
%include "Compositor/OgreCompositorNode.h"

// resources

%include "OgreResourceGroupManager.h"
%include "OgreResourceManager.h"
//%include "OgreResourceBackgroundQueue.h"
%{
#include "OgreResourceManager.h"
typedef  Ogre::ResourceManager::ResourceCreateOrRetrieveResult ResourceCreateOrRetrieveResult;
#include "OgreArchiveFactory.h"
%}
%include "OgreArchive.h"
%include "OgreArchiveFactory.h"
%include "OgreArchiveManager.h"
%{
#include "OgreZip.h"
%}
%include "OgreZip.h"
%{
#include "OgreSerializer.h"
typedef Ogre::Serializer::Endian Endian;
%}
%include "OgreSerializer.h"

%include "OgreResource.h"

%include "OgreMaterialManager.h"
%include "OgreRenderable.h"

// textures

%{
#include <OgreTextureGpu.h>
#include <OgreTextureGpuManager.h>
#include <OgreTextureBox.h>
%}
%ignore Ogre::GpuResidency::toString;
%include "OgreGpuResource.h"
%ignore Ogre::TextureGpuManager::taskLoadToSysRamOrResident; // FIXME ScheduledTasks (argument type of public member) is protected within this context
%ignore Ogre::TextureGpuManager::taskToUnloadOrDestroy; // FIXME ScheduledTasks (argument type of public member) is protected within this context
%ignore Ogre::TextureGpuManager::executeTask; // FIXME ScheduledTasks (argument type of public member) is protected within this context
%include "OgreTextureGpu.h"
%include "OgreTextureGpuManager.h"
%include "OgreTextureGpuListener.h"

%include "OgreImage2.h"
%include "OgreBlendMode.h"

// item and other movable object derived class

%{
	#include "OgreItem.h"
%}
%ignore Ogre::Item::clone; // FIXME is not defined (disabled by #if 0 in OgreItem.cpp, but not in OgreItem.h)
%include "OgreItem.h"
%include "OgreSubItem.h"

%ignore Ogre::Light::isInLightRange; // FIXME is not defined (only declaration in OgreLight.h file)
%ignore Ogre::Light::getTypeFlags; // FIXME is not defined (only declaration in OgreLight.h file and use in disabled by #ifdef ENABLE_INCOMPATIBLE_OGRE_2_0 part of OgreDefaultSceneQueries.cpp)
%include "OgreLight.h"
%include "OgreFrustum.h"
%include "OgreCamera.h"

%{
	typedef Ogre::v1::Billboard Billboard;
	typedef Ogre::v1::BillboardSet BillboardSet;
	typedef Ogre::v1::RenderOperation RenderOperation;
%}
%include "OgreBillboard.h"
%include "OgreBillboardChain.h"
%include "OgreRibbonTrail.h"
%include "OgreBillboardSet.h"

%{
	typedef Ogre::v1::SubEntity SubEntity;
	typedef Ogre::v1::VertexData VertexData;
	typedef Ogre::v1::EdgeData EdgeData;
	typedef Ogre::v1::AnimationStateSet AnimationStateSet;
	typedef Ogre::v1::AnimationState AnimationState;
	typedef Ogre::v1::OldSkeletonInstance OldSkeletonInstance;
%}
%ignore Ogre::v1::Entity::getTypeFlags; // FIXME is not defined (only declaration in OgreEntity.h file and use in disabled by #ifdef ENABLE_INCOMPATIBLE_OGRE_2_0 part of OgreDefaultSceneQueries.cpp)
%include "OgreEntity.h"
%include "OgreSubEntity.h"


// Listeners and Utils

%include "OgreFrameListener.h"
%include "OgreLodListener.h"
%include "OgreController.h"
%{
typedef Ogre::ControllerFunction< Ogre::Real > ControllerFunction_Real;
#include "OgrePredefinedControllers.h"
%}
%template(ControllerFunction_Real) Ogre::ControllerFunction< Ogre::Real >;
%template(ControllerValue_Real) Ogre::ControllerValue< Ogre::Real >;
%include "OgrePredefinedControllers.h"

// Root, Window and RenderSystem

%ignore Ogre::Root::getSceneManagerMetaDataIterator;
%ignore Ogre::Root::getSceneManagerIterator;
%include "OgreRoot.h"
%{
#include <OgreWindow.h>
%}
%include "OgreWindow.h"
%include "OgreRenderSystem.h"
