
Ogre Next Samples (Feature demonstrations) {#Samples}
==========================================

Here is a list of all the samples provided with Ogre Next and the features that they demonstrate. They are seperated into three categories: Showcases, API usage & Tutorials. The samples can be downloaded as a binary package from [github.](https://github.com/OGRECave/ogre-next/releases)

@tableofcontents

# Showcase: Forward3D {#forward3d}
This sample demonstrates Forward3D & Clustered techniques. These techniques are capable of rendering many non shadow casting lights.

![](./SampleImages/showcase_forward3d.png)

# Showcase: HDR {#hdr}
This sample demonstrates the HDR (High Dynamic Range) pipeline in action. HDR combined with PBR let us use real world values as input for our lighting and a real world scale such as lumen, lux and EV Stops.

![](./SampleImages/showcase_hdr.png)

# Showcase: HDR/SMAA {#hdrsmaa}
This sample demonstrates HDR in combination with SMAA.

![](./SampleImages/showcase_hdrsmaa.png)

# Showcase: PBS Materials {#pbsmaterials}
This sample demonstrates the PBS material system.

![](./SampleImages/showcase_pbsmaterial.png)

# Showcase: Post Processing {#postprocessing}
This sample demonstrates using the compositor for postprocessing effects.

![](./SampleImages/showcase_postprocessing.png)

# API Usage: Animation tag points {#animationtagpoints}
This sample demonstrates multiple ways in which TagPoints are used to attach to bones.

![](./SampleImages/api_animationtag.png)

# API Usage: Area Light Approximation {#arealighapprox}
This sample demonstrates area light approximation methods.

![](./SampleImages/api_arealightapprox.png)

# API Usage: Custom Renderable {#customrenderable}
This sample demonstrates creating a custom class derived from both MovableObject and Renderable for fine control over rendering. Also see related DynamicGeometry sample.

![](./SampleImages/api_customrenderable.png)

# API Usage: Decals {#decals}
This sample demonstrates screen space decals.

![](./SampleImages/api_decal.png)

# API Usage: Dynamic Geometry {#dynamicgeometry}
This sample demonstrates creating a Mesh programmatically from code and updating it.

![](./SampleImages/api_dynamicgeom.png)

# API Usage: IES Photometric Profiles {#iesprofiles}
This sample demonstrates the use of IES photometric profiles.

![](./SampleImages/api_lesprofiles.png)

# API Usage: Shared Skeleton {#sharedskeleton}
This sample demonstrates the importing of animation clips from multiple .skeleton files directly into a single skeleton from a v2Mesh and also how to share the same skeleton instance between components of the same actor/character. For example, an RPG player wearing armour, boots, helmets, etc. In this sample, the feet, hands, head, legs and torso are all separate items using the same skeleton.

![](./SampleImages/api_sharedskeleton.png)

# API Usage: Instanced Stereo {#instancedstero}
This sample demonstrates instanced stereo rendering. Related to VR.

# API Usage: Instant Radiosity {#instantradiosity}
This sample demonstrates the use of 'Instant Radiosity' (IR). IR traces rays in CPU and creates a VPL (Virtual Point Light) at every hit point to mimic the effect of Global Illumination.

![](./SampleImages/api_ir.png)

# API Usage: Local Reflections Using Parallax Corrected Cubemaps {#localcubemaps}
This sample demonstrates using parallax reflect cubemaps for accurate local reflections.

![](./SampleImages/api_localcubemap.png)

# API Usage: Local Reflections Using Parallax Corrected Cubemaps With Manual Probes {#localcubemapsmp}
This sample demonstrates using parallax reflect cubemaps for accurate local reflections. This time, we showcase the differences between manual and automatic modes. Manual probes are camera independent and work best for static objects.

![](./SampleImages/api_localcubemap_manual.png)

# API Usage: Automatic LOD Generation {#autolod}
This sample demonstrates the automatic generation of LODs from an existing mesh.

![](./SampleImages/api_meshlod.png)

# API Usage: Morph Animations {#morphanimation}
This sample demonstrates morph animations.

![](./SampleImages/api_morphanimation.png)

# API Usage: Automatically Placed Parallax Corrected Cubemap Probes Via PccPerPixelGridPlacement {#autopcc}
This sample demonstrates placing multiple PCC probes automatically.

![](./SampleImages/api_autopcc.png)

# API Usage: Planar Reflections {#planarreflections}
This sample demonstrates planar reflections.

![](./SampleImages/api_planarreflections.png)

# API Usage: Refractions {#refractions}
This sample demonstrates refractions.

![](./SampleImages/api_refractions.png)

# API Usage: SceneFormat Export / Import Sample {#sceneformat}
This sample demonstrates exporting/importing of a scene to JSON format. Includes the exporting of meshes and textures to a binary format.

# API Usage: Screen Space Reflections {#ssreflections}
This sample demonstrates screen space reflections

![](./SampleImages/api_ssreflections.png)

# API Usage: Shadow Map Debugging {#shadowdebug}
This sample demonstrates rendering the shadow map textures from a shadow node (compositor).

![](./SampleImages/api_shadowdebug.png)

# API Usage: Shadow Map From Code {#shadowfromcode}
This is similar to 'Shadow Map Debugging' sample. The main difference is that the shadow nodes are being generated programmatically via ShadowNodeHelper::createShadowNodeWithSettings instead of relying on Compositor scripts.

![](./SampleImages/api_shadowfromcode.png)

# API Usage: Static Shadow Map {#staticshadowmap}
This sample demonstrates the use of static/fixed shadow maps to increase performance.

![](./SampleImages/api_shadowstaticmap.png)

# API Usage: Stencil Test {#stenciltest}
This sample demonstrates the use of stencil test. A sphere is drawn in one pass, filling the stencil with 1s. The next pass a cube is drawn, only drawing when stencil == 1.

![](./SampleImages/api_stenciltest.png)

# API Usage: Stereo Rendering {#stereorendering}
This sample demonstrates stereo rendering.

![](./SampleImages/api_stereorendering.png)

# API Usage: Updating Decals And Area Lights' Textures {#updatedecal}
This sample demonstrates the creation of area light textures dynamically and individually. This can also be used for decals texture use.

![](./SampleImages/api_updatedecal.png)

# API Usage: Ogre V1 Interfaces {#v1interface}
This sample demonstrates the use of Ogre V1 objects (e.g. Entity) in Ogre Next.

![](./SampleImages/api_v1interface.png)

# API Usage: Ogre Next Manual Object {#onmanualobject}
This sample demonstrates the use of Ogre Next manual objects. This eases porting code from Ogre V1. For increased speed, see sample: 'Dynamic Geometry'.

![](./SampleImages/api_v2manualobject.png)

# API Usage: Ogre Next V2 Meshes {#v2Mesh}
This sample demonstrates converting Ogre V1 meshes to Ogre Next V2 format.

![](./SampleImages/api_v2mesh.png)

# Tutorial: Tutorial 00 - Basic {#tutorial00}
This tutorial demonstrates the basic setup of Ogre Next to render to a window. Uses hardcoded paths. See next tutorial to properly handle all OS's setup a render loop.

# Tutorial: Tutorial 01 - Initialisation  {#tutorial01}
This tutorial demonstrates the setup of Ogre Next in a basis framework.

# Tutorial: Tutorial 02 - Variable Framerate {#tutorial02}
This tutorial demonstrates the most basic rendering loop: Variable framerate. Variable framerate means the application adapts to the current frame rendering performance and boosts or decreases the movement speed of objects to maintain the appearance that objects are moving at a constant velocity. Despite what it seems, this is the most basic form of updating. Progress through the tutorials for superior methods of updating the rendering loop. Note: The cube is black because there is no lighting.

![](./SampleImages/tutorial_02.png)

# Tutorial: Tutorial 03 - Deterministic Loop {#tutorial03}
This is very similar to Tutorial 02, however it uses a fixed framerate instead of a variable one. There are many reasons to using a fixed framerate instead of a variable one:
 - It is more stable. High framerates (i.e. 10000 fps) cause floating point precision issues in 'timeSinceLast' as it becomes very small. The value may even round to 0!
 - Physics stability, physics and logic simulations don't like variable framerate.
 - Determinism: given the same input, every run of the program will always return the same output. Variable framerate and determinism don't play together.
For more information, see [Fix Your TimeStep!](http://gafferongames.com/game-physics/fix-your-timestep/")

![](./SampleImages/tutorial_03.png)

# Tutorial: Tutorial 04 - Interpolation Loop {#tutorial04}
This tutorial demonstrates combined fixed and variable framerate: Logic is executed at 25hz, while graphics are being rendered at a variable rate, interpolating between frames to achieve a smooth result. When OGRE or the GPU is taking too long, you will see a 'frame skip' effect, when the CPU is taking too long to process the Logic code, you will see a 'slow motion' effect. This combines the best of both worlds and is the recommended approach for serious game development. The only two disadvantages from this technique are:
- We introduce 1 frame of latency.
- Teleporting may be shown as very fast movement; as the graphics state will try to blend between the last and current position. This can be solved though, by writing to both the previous and current position in case of teleportation. We purposely don't do this to show the effect/'glitch'.

![](./SampleImages/tutorial_04.png)

# Tutorial: Tutorial 05 - Multithreading Basics {#tutorial05}
This tutorial demonstrates how to setup two update loops: One for graphics, another for logic, each in its own thread. We don't render anything because we will now need to do a robust synchronization for creating, destroying and updating Entities. This is potentially too complex to show in just one tutorial step.

# Tutorial: Tutorial 06 - Multithreading {#tutorial06}
This tutorial demonstrates advanced multithreadingl. We introduce the 'GameEntity' structure which encapsulates a game object data. It contains its graphics (i.e. Entity and SceneNode) and its physics/logic data (a transform, the hkpEntity/btRigidBody pointers, etc). The GameEntityManager is responsible for telling the render thread to create the graphics and delays deleting the GameEntity until all threads are done using it.

![](./SampleImages/tutorial_06.png)

# Tutorial: Compute 01 - UAV Textures {#compute01}
This tutorial demonstrates how to setup and use UAV textures with compute shaders.

![](./SampleImages/tutorial_compute01.png)

# Tutorial: Compute 02 - UAV Buffers {#compute02}
This tutorial demonstrates how to setup and use UAV buffers with compute shaders.

![](./SampleImages/tutorial_compute02.png)

# Tutorial: Distortion {#tut_distortion}
This tutorial demonstrates how to make compositing setup that renders different parts of the scene to different textures. Here we will render distortion pass to its own texture and use shader to compose the scene and distortion pass. Distortion setup can be used to create blastwave effects, mix with fire particle effects to get heat distortion etc. You can use this setup with all kind of objects but in this example we are using only textured simple spheres. For proper use, you should use particle systems to get better results.

![](./SampleImages/tutorial_distortion.png)

# Tutorial: Dynamic Cube Map {#dyncubemap}
This tutorial demonstrates how to setup dynamic cubemapping via the compositor, so that it can be used for dynamic reflections. 

![](./SampleImages/tutorial_dynamiccubemap.png)

# Tutorial: EGL Headless {#eglheadless}
This tutorial demonstrates how to run Ogre in EGL headless, which can be useful for running in a VM or in the Cloud.

# Tutorial: Open VR {#openVR}
This tutorial demonstrates the use of Open VR.

# Tutorial: Reconstructing Position From Depth {#reconpfromd}
This tutorial demonstrates how to reconstruct the position from only the depth buffer in a very efficient way. This is very useful for Deferred Shading, SSAO, etc.

![](./SampleImages/tutorial_posfromdepth.png)

# Tutorial: Rendering Sky As A Postprocess With A Single Shader {#skypost}
This tutorial demonstrates how to create a sky as simple postprocess effect.

![](./SampleImages/tutorial_skypost.png)

# Tutorial: SMAA {#SMAA}
This tutorial demonstrates SMAA.

![](./SampleImages/tutorial_smaa.png)

# Tutorial: SSAO {#ssao}
This tutorial demonstrates SSAO.

![](./SampleImages/tutorial_ssao.png)

# Tutorial: Terra Terrain {#terrain}
This tutorial is advanced and shows several things working together:
 - Own Hlms implementation to render the terrain
 - Vertex buffer-less rendering: The terrain is generated purely using SV_VertexID tricks and a heightmap texture.
 - Hlms customizations to PBS to make terrain shadows affect regular objects
 - Compute Shaders to generate terrain shadows every frame
 - Common terrain functionality such as loading the heightmap, generating normals, LOD.

![](./SampleImages/tutorial_terrain.png)

# Tutorial: Texture Baking {#texturebaking}
This tutorial demonstrates how to bake the render result of Ogre into a texture (e.g. for lightmaps).

![](./SampleImages/tutorial_texturebake.png)

# Tutorial: UAV Setup 1 Example {#uav1}
This tutorial demonstrates how to setup an UAV (Unordered Access View). UAVs are complex and for advanced users, but they're very powerful and enable a whole new level of features and possibilities. This sample first fills an UAV with some data, then renders to screen sampling from it as a texture.

![](./SampleImages/tutorial_uav1.png)

# Tutorial: UAV Setup 2 Example {#uav2}
This sample is exactly as 'UAV Setup 1 Example', except that it shows reading from the UAVs as an UAV (e.g. use imageLoad) instead of using it as a texture for reading.

![](./SampleImages/tutorial_uav2.png)
