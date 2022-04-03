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
#ifndef __MeshManager_H__
#define __MeshManager_H__

#include "OgrePrerequisites.h"

#include "OgreHardwareBuffer.h"
#include "OgrePatchSurface.h"
#include "OgreResourceManager.h"
#include "OgreSingleton.h"
#include "OgreVector3.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    namespace v1
    {
        class MeshSerializerListener;

        /** \addtogroup Core
         *  @{
         */
        /** \addtogroup Resources
         *  @{
         */
        /** Handles the management of mesh resources.
            @remarks
                This class deals with the runtime management of
                mesh data; like other resource managers it handles
                the creation of resources (in this case mesh data),
                working within a fixed memory budget.
        */
        class _OgreExport MeshManager final : public ResourceManager,
                                              public Singleton<MeshManager>,
                                              public ManualResourceLoader
        {
        public:
            MeshManager();
            ~MeshManager() override;

            /** Initialises the manager, only to be called by OGRE internally. */
            void _initialise();

            /// Get a resource by name
            /// @see ResourceManager::getResourceByName
            MeshPtr getByName(
                const String &name,
                const String &groupName = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );

            /// Create a new mesh
            /// @see ResourceManager::createResource
            MeshPtr create( const String &name, const String &group, bool isManual = false,
                            ManualResourceLoader    *loader = 0,
                            const NameValuePairList *createParams = 0 );

#if OGRE_COMPILER == OGRE_COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

            /** Create a new mesh, or retrieve an existing one with the same
                name if it already exists.
                @param vertexBufferUsage The usage flags with which the vertex buffer(s)
                    will be created
                @param indexBufferUsage The usage flags with which the index buffer(s) created for
                    this mesh will be created with.
                @param vertexBufferShadowed If true, the vertex buffers will be shadowed by system memory
                    copies for faster read access
                @param indexBufferShadowed If true, the index buffers will be shadowed by system memory
                    copies for faster read access
            @see ResourceManager::createOrRetrieve
            */
            ResourceCreateOrRetrieveResult createOrRetrieve(
                const String &name, const String &group, bool isManual = false,
                ManualResourceLoader *loader = 0, const NameValuePairList *params = 0,
                HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool vertexBufferShadowed = true, bool indexBufferShadowed = true );

            /** Prepares a mesh for loading from a file.  This does the IO in advance of the call to
               load().
                @note
                    If the model has already been created (prepared or loaded), the existing instance
                    will be returned.
                @remarks
                    Ogre loads model files from it's own proprietary
                    format called .mesh. This is because having a single file
                    format is better for runtime performance, and we also have
                    control over pre-processed data (such as
                    collision boxes, LOD reductions etc).
                @param filename The name of the .mesh file
                @param groupName The name of the resource group to assign the mesh to
                @param vertexBufferUsage The usage flags with which the vertex buffer(s)
                    will be created
                @param indexBufferUsage The usage flags with which the index buffer(s) created for
                    this mesh will be created with.
                @param vertexBufferShadowed If true, the vertex buffers will be shadowed by system memory
                    copies for faster read access
                @param indexBufferShadowed If true, the index buffers will be shadowed by system memory
                    copies for faster read access
            */
            MeshPtr prepare(
                const String &filename, const String &groupName,
                HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool vertexBufferShadowed = true, bool indexBufferShadowed = true );

            /** Loads a mesh from a file, making it immediately available for use.
                @note
                    If the model has already been created (prepared or loaded), the existing instance
                    will be returned.
                @remarks
                    Ogre loads model files from it's own proprietary
                    format called .mesh. This is because having a single file
                    format is better for runtime performance, and we also have
                    control over pre-processed data (such as
                    collision boxes, LOD reductions etc).
                @param filename The name of the .mesh file
                @param groupName The name of the resource group to assign the mesh to
                @param vertexBufferUsage The usage flags with which the vertex buffer(s)
                    will be created
                @param indexBufferUsage The usage flags with which the index buffer(s) created for
                    this mesh will be created with.
                @param vertexBufferShadowed If true, the vertex buffers will be shadowed by system memory
                    copies for faster read access
                @param indexBufferShadowed If true, the index buffers will be shadowed by system memory
                    copies for faster read access
            */
            MeshPtr load(
                const String &filename, const String &groupName,
                HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool vertexBufferShadowed = true, bool indexBufferShadowed = true );

#if OGRE_COMPILER == OGRE_COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

            /** Creates a new Mesh specifically for manual definition rather
                than loading from an object file.
            @remarks
                Note that once you've defined your mesh, you must call Mesh::_setBounds and
                Mesh::_setBoundingRadius in order to define the bounds of your mesh. In previous
                versions of OGRE you could call Mesh::_updateBounds, but OGRE's support of
                write-only vertex buffers makes this no longer appropriate.
            @param name The name to give the new mesh
            @param groupName The name of the resource group to assign the mesh to
            @param loader ManualResourceLoader which will be called to load this mesh
                when the time comes. It is recommended that you populate this field
                in order that the mesh can be rebuilt should the need arise
            */
            MeshPtr createManual( const String &name, const String &groupName,
                                  ManualResourceLoader *loader = 0 );

            /** Creates a basic plane, by default majoring on the x/y axes facing positive Z.
                @param name
                    The name to give the resulting mesh
                @param groupName
                    The name of the resource group to assign the mesh to
                @param plane
                    The orientation of the plane and distance from the origin
                @param width
                    The width of the plane in world coordinates
                @param height
                    The height of the plane in world coordinates
                @param xsegments
                    The number of segments to the plane in the x direction
                @param ysegments
                    The number of segments to the plane in the y direction
                @param normals
                    If true, normals are created perpendicular to the plane
                @param numTexCoordSets
                    The number of 2D texture coordinate sets created - by default the
                    corners are created to be the corner of the texture.
                @param uTile
                    The number of times the texture should be repeated in the u direction
                @param vTile
                    The number of times the texture should be repeated in the v direction
                @param upVector
                    The 'Up' direction of the plane texture coordinates.
                @param vertexBufferUsage
                    The usage flag with which the vertex buffer for this plane will be created
                @param indexBufferUsage
                    The usage flag with which the index buffer for this plane will be created
                @param vertexShadowBuffer
                    If this flag is set to true, the vertex buffer will be created
                    with a system memory shadow buffer,
                    allowing you to read it back more efficiently than if it is in hardware
                @param indexShadowBuffer
                    If this flag is set to true, the index buffer will be
                    created with a system memory shadow buffer,
                    allowing you to read it back more efficiently than if it is in hardware
            */
            MeshPtr createPlane(
                const String &name, const String &groupName, const Plane &plane, Real width, Real height,
                uint32 xsegments = 1, uint32 ysegments = 1, bool normals = true,
                unsigned short numTexCoordSets = 1, Real uTile = 1.0f, Real vTile = 1.0f,
                const Vector3        &upVector = Vector3::UNIT_Y,
                HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool vertexShadowBuffer = true, bool indexShadowBuffer = true );

            /** Creates a plane, which because of it's texture coordinates looks like a curved
                surface, useful for skies in a skybox.
                @param name
                    The name to give the resulting mesh
                @param groupName
                    The name of the resource group to assign the mesh to
                @param plane
                    The orientation of the plane and distance from the origin
                @param width
                    The width of the plane in world coordinates
                @param height
                    The height of the plane in world coordinates
                @param curvature
                    The curvature of the plane. Good values are
                    between 2 and 65. Higher values are more curved leading to
                    a smoother effect, lower values are less curved meaning
                    more distortion at the horizons but a better distance effect.
                @param xsegments
                    The number of segments to the plane in the x direction
                @param ysegments
                    The number of segments to the plane in the y direction
                @param normals
                    If true, normals are created perpendicular to the plane
                @param numTexCoordSets
                    The number of 2D texture coordinate sets created - by default the corners
                    are created to be the corner of the texture.
                @param uTile
                    The number of times the texture should be repeated in the u direction
                @param vTile
                    The number of times the texture should be repeated in the v direction
                @param upVector
                    The 'Up' direction of the plane.
                @param orientation
                    The orientation of the overall sphere that's used to create the illusion
                @param vertexBufferUsage
                    The usage flag with which the vertex buffer for this plane will be created
                @param indexBufferUsage
                    The usage flag with which the index buffer for this plane will be created
                @param vertexShadowBuffer
                    If this flag is set to true, the vertex buffer will be created
                    with a system memory shadow buffer,
                    allowing you to read it back more efficiently than if it is in hardware
                @param indexShadowBuffer
                    If this flag is set to true, the index buffer will be
                    created with a system memory shadow buffer,
                    allowing you to read it back more efficiently than if it is in hardware
                @param ySegmentsToKeep The number of segments from the top of the dome
                    downwards to keep. -1 keeps all of them. This can save fillrate if
                    you cannot see much of the sky lower down.
            */
            MeshPtr createCurvedIllusionPlane(
                const String &name, const String &groupName, const Plane &plane, Real width, Real height,
                Real curvature, uint32 xsegments = 1, uint32 ysegments = 1, bool normals = true,
                unsigned short numTexCoordSets = 1, Real uTile = 1.0f, Real vTile = 1.0f,
                const Vector3        &upVector = Vector3::UNIT_Y,
                const Quaternion     &orientation = Quaternion::IDENTITY,
                HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool vertexShadowBuffer = true, bool indexShadowBuffer = true,
                uint32 ysegmentsToKeep = std::numeric_limits<uint32>::max() );

            /** Creates a genuinely curved plane, by default majoring on the x/y axes facing positive Z.
                @param name
                    The name to give the resulting mesh
                @param groupName
                    The name of the resource group to assign the mesh to
                @param plane
                    The orientation of the plane and distance from the origin
                @param width
                    The width of the plane in world coordinates
                @param height
                    The height of the plane in world coordinates
                @param bow
                    The amount of 'bow' in the curved plane.  (Could also be considered the depth.)
                @param xsegments
                    The number of segments to the plane in the x direction
                @param ysegments
                    The number of segments to the plane in the y direction
                @param normals
                    If true, normals are created perpendicular to the plane
                @param numTexCoordSets
                    The number of 2D texture coordinate sets created - by default the corners
                    are created to be the corner of the texture.
                @param uTile
                    The number of times the texture should be repeated in the u direction
                @param vTile
                    The number of times the texture should be repeated in the v direction
                @param upVector
                    The 'Up' direction of the plane.
                @param vertexBufferUsage
                    The usage flag with which the vertex buffer for this plane will be created
                @param indexBufferUsage
                    The usage flag with which the index buffer for this plane will be created
                @param vertexShadowBuffer
                    If this flag is set to true, the vertex buffer will be created
                    with a system memory shadow buffer,
                    allowing you to read it back more efficiently than if it is in hardware
                @param indexShadowBuffer
                    If this flag is set to true, the index buffer will be
                    created with a system memory shadow buffer,
                    allowing you to read it back more efficiently than if it is in hardware
            */
            MeshPtr createCurvedPlane(
                const String &name, const String &groupName, const Plane &plane, Real width, Real height,
                Real bow = 0.5f, uint32 xsegments = 1, uint32 ysegments = 1, bool normals = false,
                unsigned short numTexCoordSets = 1, Real uTile = 1.0f, Real vTile = 1.0f,
                const Vector3        &upVector = Vector3::UNIT_Y,
                HardwareBuffer::Usage vertexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool vertexShadowBuffer = true, bool indexShadowBuffer = true );

            /** Creates a Bezier patch based on an array of control vertices.
                @param name
                    The name to give the newly created mesh.
                @param groupName
                    The name of the resource group to assign the mesh to
                @param controlPointBuffer
                    A pointer to a buffer containing the vertex data which defines control points
                    of the curves rather than actual vertices. Note that you are expected to provide not
                    just position information, but potentially normals and texture coordinates too. The
                    format of the buffer is defined in the VertexDeclaration parameter
                @param declaration
                    VertexDeclaration describing the contents of the buffer.
                    Note this declaration must _only_ draw on buffer source 0!
                @param width
                    Specifies the width of the patch in control points.
                    Note this parameter must greater than or equal to 3.
                @param height
                    Specifies the height of the patch in control points.
                    Note this parameter must greater than or equal to 3.
                @param uMaxSubdivisionLevel, vMaxSubdivisionLevel
                    If you want to manually set the top level of subdivision,
                    do it here, otherwise let the system decide.
                @param visibleSide
                    Determines which side of the patch (or both) triangles are generated for.
                @param vbUsage
                    Vertex buffer usage flags. Recommend the default since vertex buffer should be
                    static.
                @param ibUsage
                    Index buffer usage flags. Recommend the default since index buffer should
                    be dynamic to change levels but not readable.
                @param vbUseShadow
                    Flag to determine if a shadow buffer is generated for the vertex buffer. See
                    HardwareBuffer for full details.
                @param ibUseShadow
                    Flag to determine if a shadow buffer is generated for the index buffer. See
                    HardwareBuffer for full details.
            */
            PatchMeshPtr createBezierPatch(
                const String &name, const String &groupName, void *controlPointBuffer,
                VertexDeclaration *declaration, size_t width, size_t height,
                size_t                    uMaxSubdivisionLevel = (size_t)PatchSurface::AUTO_LEVEL,
                size_t                    vMaxSubdivisionLevel = (size_t)PatchSurface::AUTO_LEVEL,
                PatchSurface::VisibleSide visibleSide = PatchSurface::VS_FRONT,
                HardwareBuffer::Usage     vbUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                HardwareBuffer::Usage     ibUsage = HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY,
                bool vbUseShadow = true, bool ibUseShadow = true );

            /** Tells the mesh manager that all future meshes should prepare themselves for
                shadow volumes on loading.
            */
            void setPrepareAllMeshesForShadowVolumes( bool enable );
            /** Retrieves whether all Meshes should prepare themselves for shadow volumes. */
            bool getPrepareAllMeshesForShadowVolumes();

            /** Override standard Singleton retrieval.
            @remarks
            Why do we do this? Well, it's because the Singleton
            implementation is in a .h file, which means it gets compiled
            into anybody who includes it. This is needed for the
            Singleton template to work, but we actually only want it
            compiled into the implementation of the class based on the
            Singleton, not all of them. If we don't change this, we get
            link errors when trying to use the Singleton-based class from
            an outside dll.
            @par
            This method just delegates to the template version anyway,
            but the implementation stays in this single compilation unit,
            preventing link errors.
            */
            static MeshManager &getSingleton();
            /** Override standard Singleton retrieval.
            @remarks
            Why do we do this? Well, it's because the Singleton
            implementation is in a .h file, which means it gets compiled
            into anybody who includes it. This is needed for the
            Singleton template to work, but we actually only want it
            compiled into the implementation of the class based on the
            Singleton, not all of them. If we don't change this, we get
            link errors when trying to use the Singleton-based class from
            an outside dll.
            @par
            This method just delegates to the template version anyway,
            but the implementation stays in this single compilation unit,
            preventing link errors.
            */
            static MeshManager *getSingletonPtr();

            /** Gets the factor by which the bounding box of an entity is padded.
                Default is 0.01
            */
            Real getBoundsPaddingFactor();

            /** Sets the factor by which the bounding box of an entity is padded
             */
            void setBoundsPaddingFactor( Real paddingFactor );

            /** Called when we you use a mesh which has shared vertices, the function creates separate
                vertex/index buffers and also recreates the bone assignments.
            */
            static void unshareVertices( Mesh *mesh );

            /** Sets the listener used to control mesh loading through the serializer.
             */
            void setListener( MeshSerializerListener *listener );

            /** Gets the listener used to control mesh loading through the serializer.
             */
            MeshSerializerListener *getListener();

            /** @see ManualResourceLoader::loadResource */
            void loadResource( Resource *res ) override;

        protected:
            /// @copydoc ResourceManager::createImpl
            Resource *createImpl( const String &name, ResourceHandle handle, const String &group,
                                  bool isManual, ManualResourceLoader *loader,
                                  const NameValuePairList *createParams ) override;

            /** Utility method for tessellating 2D meshes.
             */
            void tesselate2DMesh(
                Mesh *pMesh, SubMesh *pSub, uint32 meshWidth, uint32 meshHeight,
                bool                  doubleSided = false,
                HardwareBuffer::Usage indexBufferUsage = HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                bool                  indexSysMem = false );

            void createPrefabPlane();
            void createPrefabCube();
            void createPrefabSphere();

            /** Enum identifying the types of manual mesh built by this manager */
            enum MeshBuildType
            {
                MBT_PLANE,
                MBT_CURVED_ILLUSION_PLANE,
                MBT_CURVED_PLANE
            };
            /** Saved parameters used to (re)build a manual mesh built by this class */
            struct MeshBuildParams
            {
                MeshBuildType         type;
                Plane                 plane;
                Real                  width;
                Real                  height;
                Real                  curvature;
                uint32                xsegments;
                uint32                ysegments;
                bool                  normals;
                unsigned short        numTexCoordSets;
                Real                  xTile;
                Real                  yTile;
                Vector3               upVector;
                Quaternion            orientation;
                HardwareBuffer::Usage vertexBufferUsage;
                HardwareBuffer::Usage indexBufferUsage;
                bool                  vertexShadowBuffer;
                bool                  indexShadowBuffer;
                uint32                ySegmentsToKeep;
            };
            /** Map from resource pointer to parameter set */
            typedef map<Resource *, MeshBuildParams>::type MeshBuildParamsMap;
            MeshBuildParamsMap                             mMeshBuildParams;

            /** Utility method for manual loading a plane */
            void loadManualPlane( Mesh *pMesh, MeshBuildParams &params );
            /** Utility method for manual loading a curved plane */
            void loadManualCurvedPlane( Mesh *pMesh, MeshBuildParams &params );
            /** Utility method for manual loading a curved illusion plane */
            void loadManualCurvedIllusionPlane( Mesh *pMesh, MeshBuildParams &params );

            bool mPrepAllMeshesForShadowVolumes;

            // the factor by which the bounding box of an entity is padded
            Real mBoundsPaddingFactor;

            // The listener to pass to serializers
            MeshSerializerListener *mListener;
        };

        /** @} */
        /** @} */

    }  // namespace v1
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
