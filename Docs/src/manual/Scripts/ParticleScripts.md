# Particle Scripts {#Particle-Scripts}

Particle scripts allow you to define particle systems to be instantiated in your code without having to hard-code the settings themselves in your source code, allowing a very quick turnaround on any changes you make. Particle systems which are defined in scripts are used as templates, and multiple actual systems can be created from them at runtime.

@tableofcontents

Once scripts have been parsed, your code is free to instantiate systems based on them using either the Ogre::SceneManager::createParticleSystem() method which can take both a name for the new system, and the name of the template to base it on (this template name is in the script) or the Ogre::SceneManager::createParticleSystem2() if you're using the [ParticleFX2 Plugin](@ref ParticleSystem2).

@snippet Samples/Media/particle/Examples.particle manual_sample

A system can have top-level attributes set using the scripting commands available, such as ’quota’ to set the maximum number of particles allowed in the system. Emitters (which create particles) and affectors (which modify particles) are added as nested definitions within the script. The parameters available in the emitter and affector sections are entirely dependent on the type of emitter / affector.

For a detailed description of the core particle system attributes, see the list below:

## Particle System Attributes {#Particle-System-Attributes}

This section describes to attributes which you can set on every particle system using scripts. All attributes have default values so all settings are optional in your script.

-   [quota](@ref ParticleSystemAttributes_quota)
-   [material](@ref ParticleSystemAttributes_material)
-   [particle_width](@ref ParticleSystemAttributes_particle_width)
-   [particle_height](@ref ParticleSystemAttributes_particle_height)
-   [cull_each](@ref ParticleSystemAttributes_cull_each)
-   [renderer](@ref ParticleSystemAttributes_renderer)
-   [sorted](@ref ParticleSystemAttributes_sorted)
-   [local_space](@ref ParticleSystemAttributes_local_space)
-   [iteration_interval](@ref ParticleSystemAttributes_iteration_interval)
-   [nonvisible_update_timeout](@ref ParticleSystemAttributes_nonvisible_update_timeout)

@par Billboard Renderer Attributes

-   [billboard_type](@ref BillboardRendererAttributes_billboard_type)
-   [billboard_origin](@ref BillboardRendererAttributes_origin)
-   [billboard_rotation_type](@ref BillboardRendererAttributes_billboard_rotation_type)
-   [common_direction](@ref BillboardRendererAttributes_common_direction)
-   [common_up_vector](@ref BillboardRendererAttributes_common_up_vector)
-   [point_rendering](@ref BillboardRendererAttributes_point_rendering)
-   [accurate_facing](@ref BillboardRendererAttributes_accurate_facing)
-   [texture_sheet_size](@ref BillboardRendererAttributes_texture_sheet_size)

@see @ref Particle-Emitters
@see @ref Particle-Affectors


### quota {#ParticleSystemAttributes_quota}

Sets the maximum number of particles this system is allowed to contain at one time. When this limit is exhausted, the emitters will not be allowed to emit any more particles until some destroyed (e.g. through their time\_to\_live running out). Note that you will almost always want to change this, since it defaults to a very low value (particle pools are only ever increased in size, never decreased).

```cpp
format:  quota <max_particles>
example: quota 10000
default: 10
```

@note **PFX2:** quota is globally shared by all system instances of the same template. In PFX1, the quota is independent for each ParticleSystem instance.

### material {#ParticleSystemAttributes_material}

Sets the name of the material which all particles in this system will use. All particles in a system use the same material, although each particle can tint this material through the use of it’s colour property.

```cpp
format:  material <material_name>
example: material Examples/Flare
default: none (blank material)
```

@note **PFX1:** Can be an HLMS or Low Level Material.
@par
@note **PFX2:** Must be an HLMS Material.

### particle\_width {#ParticleSystemAttributes_particle_width}

Sets the width of particles in world coordinates. Note that this property is absolute when billboard\_type (see below) is set to ’point’ or ’perpendicular\_self’, but is scaled by the length of the direction vector when billboard\_type is ’oriented\_common’, ’oriented\_self’ or ’perpendicular\_common’.<br>

```cpp
format:  particle_width <width>
example: particle_width 20
default: 100
```

### particle\_height {#ParticleSystemAttributes_particle_height}

Sets the height of particles in world coordinates. Note that this property is absolute when billboard\_type (see below) is set to ’point’ or ’perpendicular\_self’, but is scaled by the length of the direction vector when billboard\_type is ’oriented\_common’, ’oriented\_self’ or ’perpendicular\_common’.<br>

```cpp
particle_height <height>

example:
particle_width 20
default: 100
```

### cull\_each {#ParticleSystemAttributes_cull_each}

All particle systems are culled by the bounding box which contains all the particles in the system. This is normally sufficient for fairly locally constrained particle systems where most particles are either visible or not visible together. However, for those that spread particles over a wider area (e.g. a rain system), you may want to actually cull each particle individually to save on time, since it is far more likely that only a subset of the particles will be visible. You do this by setting the cull\_each parameter to true.

```cpp
format:  cull_each <true|false>
example: cull_each true
default: false
```

@note **PFX2:** This setting is not available. Does not apply.

### renderer {#ParticleSystemAttributes_renderer}

Particle systems do not render themselves, they do it through ParticleRenderer classes. Those classes are registered with a manager in order to provide particle systems with a particular ’look’. OGRE comes configured with a default billboard-based renderer, but more can be added through plugins. Particle renders are registered with a unique name, and you can use that name in this attribute to determine the renderer to use. The default is ’billboard’.

Particle renderers can have attributes, which can be passed by setting them on the root particle system.

```cpp
format:  renderer <renderer_name>
example: renderer billboard
default: billboard
```

@note **PFX2:** This setting is not available. Does not apply.

### sorted {#ParticleSystemAttributes_sorted}

By default, particles are not sorted. By setting this attribute to ’true’, the particles will be sorted with respect to the camera, furthest first. This can make certain rendering effects look better at a small sorting expense.

```cpp
format:  sorted <true|false>
example: sorted true
default: false
```

@note **PFX2:** This setting is not available. See [Using OIT (Order Independent Transparency)](@ref ParticleSystem2Oit).

### local\_space {#ParticleSystemAttributes_local_space}

By default, particles are emitted into world space, such that if you transform the node to which the system is attached, it will not affect the particles (only the emitters). This tends to give the normal expected behaviour, which is to model how real world particles travel independently from the objects they are emitted from. However, to create some effects you may want the particles to remain attached to the local space the emitter is in and to follow them directly. This option allows you to do that.

```cpp
format:  local_space <true|false>
example: local_space true
default: false
```

@note **PFX2:** This setting is not available.

### iteration\_interval {#ParticleSystemAttributes_iteration_interval}

Usually particle systems are updated based on the frame rate; however this can give variable results with more extreme frame rate ranges, particularly at lower frame rates. You can use this option to make the update frequency a fixed interval, whereby at lower frame rates, the particle update will be repeated at the fixed interval until the frame time is used up. A value of 0 means the default frame time iteration.

```cpp
format:  iteration_interval <secs>
example: iteration_interval 0.01
default: 0
```

@note **PFX2:** This setting is not available. But it can be tweaked globally for all particle systems, just not per particle.

### nonvisible\_update\_timeout {#ParticleSystemAttributes_nonvisible_update_timeout}

Sets when the particle system should stop updating after it hasn’t been visible for a while. By default, visible particle systems update all the time, even when not in view. This means that they are guaranteed to be consistent when they do enter view. However, this comes at a cost, updating particle systems can be expensive, especially if they are perpetual.  This option lets you set a ’timeout’ on the particle system, so that if it isn’t visible for this amount of time, it will stop updating until it is next visible. A value of 0 disables the timeout and always updates.

```cpp
format:  nonvisible_update_timeout <secs>
example: nonvisible_update_timeout 10
default: 0
```

@note **PFX2:** This setting is not available. Does not apply. PFX2 relies on global quotas instead; particles closer to camera are given higher priority to consume the global shared pool of particles.

## Billboard Renderer Attributes {#Billboard-Renderer-Attributes}

These are actually attributes of the @c billboard particle renderer (the default), but can be passed to a particle renderer by declaring them directly within the system declaration. Particles using the default renderer are rendered using billboards, which are rectangles formed by 2 triangles which rotate to face the given direction.

### billboard\_type {#BillboardRendererAttributes_billboard_type}

There is more than 1 way to orient a billboard. The classic approach is for the billboard to directly face the camera: this is the default behaviour. However this arrangement only looks good for particles which are representing something vaguely spherical like a light flare. For more linear effects like laser fire, you actually want the particle to have an orientation of it’s own.

```cpp
format:  billboard_type <point|oriented_common|oriented_self|perpendicular_common|perpendicular_self>
example: billboard_type perpendicular_self
default: point
```

The options for this parameter are:

<dl compact="compact">
<dt>point</dt> <dd>

The default arrangement, this approximates spherical particles and the billboards always fully face the camera.

</dd> <dt>oriented\_common</dt> <dd>

Particles are oriented around a common, typically fixed direction vector (see [common_direction](@ref BillboardRendererAttributes_common_direction)), which acts as their local Y axis. The billboard rotates only around this axis, giving the particle some sense of direction. Good for rainstorms, starfields etc where the particles will traveling in one direction - this is slightly faster than oriented\_self (see below).

</dd> <dt>oriented\_self</dt> <dd>

Particles are oriented around their own direction vector, which acts as their local Y axis. As the particle changes direction, so the billboard reorients itself to face this way. Good for laser fire, fireworks and other ’streaky’ particles that should look like they are traveling in their own direction.

</dd> <dt>perpendicular\_common</dt> <dd>

Particles are perpendicular to a common, typically fixed direction vector (see [common_direction](@ref BillboardRendererAttributes_common_direction)), which acts as their local Z axis, and their local Y axis coplanar with common direction and the common up vector (see [common_up_vector](@ref BillboardRendererAttributes_common_up_vector)). The billboard never rotates to face the camera, you might use double-side material to ensure particles never culled by back-facing. Good for aureolas, rings etc where the particles will perpendicular to the ground - this is slightly faster than perpendicular\_self (see below).

</dd> <dt>perpendicular\_self</dt> <dd>

Particles are perpendicular to their own direction vector, which acts as their local Z axis, and their local Y axis coplanar with their own direction vector and the common up vector (see [common_up_vector](@ref BillboardRendererAttributes_common_up_vector)). The billboard never rotates to face the camera, you might use double-side material to ensure particles never culled by back-facing. Good for rings stack etc where the particles will perpendicular to their traveling direction.

### billboard\_origin {#BillboardRendererAttributes_origin}

@copydetails Ogre::v1::BillboardOrigin

```cpp
format:  billboard_origin <top_left|top_center|top_right|center_left|center|center_right|bottom_left|bottom_center|bottom_right>
example: billboard_type top_right
default: center
```

@note **PFX2:** This setting is not available. It is currently not implemented and only ’center’ is supported at the moment.

### billboard\_rotation\_type {#BillboardRendererAttributes_billboard_rotation_type}

@copydetails Ogre::v1::BillboardRotationType

```cpp
format:  billboard_rotation_type <vertex|texcoord|none>
example: billboard_rotation_type vertex
default: texcoord (PFX1)
default: none (PFX2 w/out rotator affector)
default: texcoord (PFX2 w/ rotator affector)
```

The options for this parameter are:

<dl compact="compact">
<dt>vertex</dt> <dd>

Billboard particles will rotate the vertices around their facing direction to according with particle rotation. Rotate vertices guarantee texture corners exactly match billboard corners, thus has advantage mentioned above, but should take more time to generate the vertices.

</dd> <dt>texcoord</dt> <dd>

Billboard particles will rotate the texture coordinates to according with particle rotation. Rotate texture coordinates is faster than rotate vertices, but has some disadvantage mentioned above.

</dd> <dt>none</dt> <dd>

Billboard particles will not rotate. This saves performance if rotation is not needed.

@note **PFX1:** ’none’ is not available.
@par
@note **PFX2:** If the [Rotator Affector](@ref Rotator-Affector) is present and the setting is set to ’none’, it will be automatically overriden in favour of ’texcoord’.

</dd> </dl>

### common\_direction {#BillboardRendererAttributes_common_direction}

Only required if [billboard_type](@ref BillboardRendererAttributes_billboard_type) is set to oriented\_common or perpendicular\_common, this vector is the common direction vector used to orient all particles in the system.


```cpp
format:  common_direction <x> <y> <z>
example: common_direction  0  -1   0
default: common_direction  0   0   1
```

### common\_up\_vector {#BillboardRendererAttributes_common_up_vector}

Only required if [billboard_type](@ref BillboardRendererAttributes_billboard_type) is set to perpendicular\_self or perpendicular\_common, this vector is the common up vector used to orient all particles in the system.

```cpp
format:  common_up_vector <x> <y> <z>
example: common_up_vector  0   1   0
default: common_up_vector  0   1   0
```

### point\_rendering {#BillboardRendererAttributes_point_rendering}

This sets whether or not the BillboardSet will use point rendering rather than manually generated quads.

@copydetails Ogre::v1::BillboardSet::setPointRenderingEnabled

@note **PFX2:** This setting is not available. Except for Quadro and other professional lines, GPU point rendering support isn't really good. PFX2 uses modern techniques where data is stored internally as points and quads are generated procedurally on the vertex shader.

### accurate\_facing {#BillboardRendererAttributes_accurate_facing}

This sets whether or not the BillboardSet will use a slower but more accurate calculation for facing the billboard to the camera. By default it uses the camera direction, which is faster but means the billboards don’t stay in the same orientation as you rotate the camera. The ’accurate\_facing on’ option makes the calculation based on a vector from each billboard to the camera, which means the orientation is constant even whilst the camera rotates.

```cpp
format:  accurate_facing on|off
example: accurate_facing on
default: accurate_facing off
```

@note **PFX2:** This setting is not available. `accurate_facing` is forced on for all systems.

### texture_sheet_size {#BillboardRendererAttributes_texture_sheet_size}

```cpp
format:  texture_sheet_size <stacks> <slices>
example: texture_sheet_size 2 3
default: texture_sheet_size 0 0
```

@copydetails Ogre::v1::BillboardSet::setTextureStacksAndSlices

@note **PFX2:** This setting is not available. It hasn't been implemented yet.

# Particle Emitters {#Particle-Emitters}

Particle emitters are classified by ’type’ e.g. ’Point’ emitters emit from a single point whilst ’Box’ emitters emit randomly from an area. New emitters can be added to Ogre by creating plugins. You add an emitter to a system by nesting another section within it, headed with the keyword ’emitter’ followed by the name of the type of emitter (case sensitive). Ogre currently supports ’Point’, ’Box’, ’Cylinder’, ’Ellipsoid’, ’HollowEllipsoid’ and ’Ring’ emitters.

@see @ref Particle-Affectors

## Emitting Emitters {#Emitting-Emitters}

It is possible to ’emit emitters’ - that is, have new emitters spawned based on the position of particles,, for example to product ’firework’ style effects.

This is controlled via the following directives:

<dl compact="compact">
<dt>emit\_emitter\_quota</dt> <dd>

This parameter is a system-level parameter telling the system how many emitted emitters may be in use at any one time. This is just to allow for the space allocation process.

</dd> <dt>name</dt> <dd>

This parameter is an emitter-level parameter, giving a name to an emitter. This can then be referred to in another emitter as the new emitter type to spawn.

</dd> <dt>emit\_emitter</dt> <dd>

This is an emitter-level parameter, and if specified, it means that the particles spawned by this emitter, are themselves emitters of the named type.

</dd> </dl>

@note **PFX2:** It is not currently possible to emit emitters.

<a name="Particle-Emitter-Attributes"></a> <a name="Particle-Emitter-Attributes-1"></a>

## Common Emitter Attributes

This section describes the common attributes of all particle emitters. Specific emitter types may also support their own extra attributes.

-   [angle](@ref ParticleEmitterAttributes_angle)
-   [colour](@ref ParticleEmitterAttributes_colour)
-   [colour_range_start](@ref ParticleEmitterAttributes_colour_range_start)
-   [colour_range_end](@ref ParticleEmitterAttributes_colour_range_start)
-   [direction](@ref ParticleEmitterAttributes_direction)
-   [direction_position_reference](@ref ParticleEmitterAttributes_direction_position_reference)
-   [emission_rate](@ref ParticleEmitterAttributes_emission_rate)
-   [position](@ref ParticleEmitterAttributes_position)
-   [velocity](@ref ParticleEmitterAttributes_velocity)
-   [velocity_min](@ref ParticleEmitterAttributes_velocity_min_max)
-   [velocity_max](@ref ParticleEmitterAttributes_velocity_min_max)
-   [time_to_live](@ref ParticleEmitterAttributes_time_to_live)
-   [time_to_live_min](@ref ParticleEmitterAttributes_time_to_live_min_max)
-   [time_to_live_max](@ref ParticleEmitterAttributes_time_to_live_min_max)
-   [duration](@ref ParticleEmitterAttributes_duration)
-   [duration_min](@ref ParticleEmitterAttributes_duration_min_max)
-   [duration_max](@ref ParticleEmitterAttributes_duration_min_max)
-   [repeat_delay](@ref ParticleEmitterAttributes_repeat_delay)
-   [repeat_delay_min](@ref ParticleEmitterAttributes_repeat_delay_min_max)
-   [repeat_delay_max](@ref ParticleEmitterAttributes_repeat_delay_min_max)

## angle {#ParticleEmitterAttributes_angle}

Sets the maximum angle (in degrees) which emitted particles may deviate from the direction of the emitter (see direction). Setting this to 10 allows particles to deviate up to 10 degrees in any direction away from the emitter’s direction. A value of 180 means emit in any direction, whilst 0 means emit always exactly in the direction of the emitter.

```cpp
format:  angle <degrees>
example: angle 30
default: 0
```

## colour {#ParticleEmitterAttributes_colour}

Sets a static colour for all particle emitted. Also see the colour\_range\_start and colour\_range\_end attributes for setting a range of colours. The format of the colour parameter is "r g b a", where each component is a value from 0 to 1, and the alpha value is optional (assumes 1 if not specified).

```cpp
format:  colour <r> <g> <b> <a>
example: colour 1 0 0 1
default: colour 1 1 1 1
```

## colour\_range\_start & colour\_range\_end {#ParticleEmitterAttributes_colour_range_start}

As the ’colour’ attribute, except these 2 attributes must be specified together, and indicate the range of colours available to emitted particles. The actual colour will be randomly chosen between these 2 values.

```cpp
format:  same as colour
example (generates random colours between red and blue):
   colour_range_start 1 0 0
   colour_range_end 0 0 1
default: both 1 1 1 1
```

## direction {#ParticleEmitterAttributes_direction}

Sets the direction of the emitter. This is relative to the SceneNode which the particle system is attached to, meaning that as with other movable objects changing the orientation of the node will also move the emitter.

```cpp
format:  direction <x> <y> <z>
example: direction 0 1 0
default: direction 1 0 0
```

## direction\_position\_reference {#ParticleEmitterAttributes_direction_position_reference}

Sets the position reference of the emitter. This supersedes direction when present. The last parameter must be 1 to enable it, 0 to disable. You may still want to set the direction to setup orientation of the emitter’s dimensions. When present, particles direction is calculated at the time of emission by doing (particlePosition - referencePosition); therefore particles will travel in a particular direction or in every direction depending on where the particles are originated, and the location of the reference position. Note angle still works to apply some randomness after the direction vector is generated. This parameter is specially useful to create explosions and implosions (when velocity is negative) best paired with HollowEllipsoid and Ring emitters. This is relative to the SceneNode which the particle system is attached to, meaning that as with other movable objects changing the orientation of the node will also move the emitter.

```cpp
format:  direction_position_reference <x> <y> <z> <enable>
example: direction_position_reference 0 -10 0 1
default: direction_position_reference 0 0 0 0
```

## emission\_rate {#ParticleEmitterAttributes_emission_rate}

Sets how many particles per second should be emitted. The specific emitter does not have to emit these in a continuous manner - this is a relative parameter and the emitter may choose to emit all of the second’s worth of particles every half-second for example, the behaviour depends on the emitter. The emission rate will also be limited by the particle system’s ’quota’ setting.

```cpp
format:  emission_rate <particles_per_second>
example: emission_rate 50
default: 10
```

## position {#ParticleEmitterAttributes_position}

Sets the position of the emitter relative to the SceneNode the particle system is attached to.

```cpp
format:  position <x> <y> <z>
example: position 10 0 40
default: 0 0 0
```

## velocity {#ParticleEmitterAttributes_velocity}

Sets a constant velocity for all particles at emission time. See also the velocity\_min and velocity\_max attributes which allow you to set a range of velocities instead of a fixed one.

```cpp
format:  velocity <world_units_per_second>
example: velocity 100
default: 1
```

## velocity\_min & velocity\_max {#ParticleEmitterAttributes_velocity_min_max}

As ’velocity’ except these attributes set a velocity range and each particle is emitted with a random velocity within this range.

```cpp
format:  same as velocity
example:
   velocity_min 50
   velocity_max 100
default: both 1
```

## time\_to\_live {#ParticleEmitterAttributes_time_to_live}

Sets the number of seconds each particle will ’live’ for before being destroyed. NB it is possible for particle affectors to alter this in flight, but this is the value given to particles on emission. See also the time\_to\_live\_min and time\_to\_live\_max attributes which let you set a lifetime range instead of a fixed one.

```cpp
format:  time_to_live <seconds>
example: time_to_live 10
default: 5
```

## time\_to\_live\_min & time\_to\_live\_max {#ParticleEmitterAttributes_time_to_live_min_max}

As time\_to\_live, except this sets a range of lifetimes and each particle gets a random value in-between on emission.

```cpp
format:  same as time_to_live
example:
   time_to_live_min 2
   time_to_live_max 5
default: both 5
```

## duration {#ParticleEmitterAttributes_duration}

Sets the number of seconds the emitter is active. The emitter can be started again, see [repeat_delay](@ref ParticleEmitterAttributes_repeat_delay).
See also the duration\_min and duration\_max attributes which let you set a duration range instead of a fixed one.

```cpp
format:  duration <seconds>
example: duration 2.5
default: 0
```

@note A value of 0 means infinite duration. A value < 0 means "burst" where @c emission_rate of particles are emitted once in the next frame.

## duration\_min & duration\_max {#ParticleEmitterAttributes_duration_min_max}

As duration, except these attributes set a variable time range between the min and max values each time the emitter is started.

```cpp
format:  same as duration
example:
   duration_min 2
   duration_max 5
default: both 0
```

## repeat\_delay {#ParticleEmitterAttributes_repeat_delay}

Sets the number of seconds to wait before the emission is repeated when stopped by a limited [duration](@ref ParticleEmitterAttributes_duration). See also the repeat\_delay\_min and repeat\_delay\_max attributes which allow you to set a range of repeat\_delays instead of a fixed one.

```cpp
format:  repeat_delay <seconds>
example: repeat_delay 2.5
default: 0
```

## repeat\_delay\_min & repeat\_delay\_max {#ParticleEmitterAttributes_repeat_delay_min_max}

As repeat\_delay, except this sets a range of repeat delays and each time the emitter is started it gets a random value in-between.

```cpp
format:  same as repeat_delay
example:
   repeat_delay_min 2
   repeat_delay_max 5
default: both 0
```

# Standard Particle Emitters {#Standard-Particle-Emitters}

Ogre comes preconfigured with a few particle emitters. New ones can be added by creating plugins: see the Plugin\_ParticleFX project as an example of how you would do this (this is where these emitters are implemented).

-   [Point Emitter](#Point-Emitter)
-   [Box Emitter](#Box-Emitter)
-   [Cylinder Emitter](#Cylinder-Emitter)
-   [Ellipsoid Emitter](#Ellipsoid-Emitter)
-   [Hollow Ellipsoid Emitter](#Hollow-Ellipsoid-Emitter)
-   [Ring Emitter](#Ring-Emitter)

## Point Emitter {#Point-Emitter}

@copybrief Ogre::PointEmitter

This emitter has no additional attributes over an above the standard emitter attributes.

To create a point emitter, include a section like this within your particle system script:

```cpp

emitter Point
{
    // Settings go here
}
```

## Box Emitter {#Box-Emitter}

@copybrief Ogre::BoxEmitter

It’s extra attributes are:

<dl compact="compact">
<dt>width</dt> <dd>

Sets the width of the box (this is the size of the box along it’s local X axis, which is dependent on the ’direction’ attribute which forms the box’s local Z).

```cpp
format:  width <units>
example: width 250
default: 100
```

</dd> <dt>height</dt> <dd>

Sets the height of the box (this is the size of the box along it’s local Y axis, which is dependent on the ’direction’ attribute which forms the box’s local Z).

```cpp
format:  height <units>
example: height 250
default: 100
```

</dd> <dt>depth</dt> <dd>

Sets the depth of the box (this is the size of the box along it’s local Z axis, which is the same as the ’direction’ attribute).

```cpp
format:  depth <units>
example: depth 250
default: 100
```

</dd> </dl> <br>

To create a box emitter, include a section like this within your particle system script:

```cpp
emitter Box
{
    // Settings go here
}
```

## Cylinder Emitter {#Cylinder-Emitter}

@copybrief Ogre::CylinderEmitter

This emitter has exactly the same parameters as the [Box Emitter](#Box-Emitter) so there are no additional parameters to consider here - the width and height determine the shape of the cylinder along it’s axis (if they are different it is an ellipsoid cylinder), the depth determines the length of the cylinder.

## Ellipsoid Emitter {#Ellipsoid-Emitter}

This emitter emits particles from within an ellipsoid shaped area, i.e. a sphere or squashed-sphere area. The parameters are again identical to the [Box Emitter](#Box-Emitter), except that the dimensions describe the widest points along each of the axes.

## Hollow Ellipsoid Emitter {#Hollow-Ellipsoid-Emitter}

This emitter is just like [Ellipsoid Emitter](#Ellipsoid-Emitter) except that there is a hollow area in the center of the ellipsoid from which no particles are emitted.

Therefore it has 3 extra parameters in order to define this area:

<dl compact="compact">
<dt>inner\_width</dt> <dd>

The width of the inner area which does not emit any particles.

</dd> <dt>inner\_height</dt> <dd>

The height of the inner area which does not emit any particles.

</dd> <dt>inner\_depth</dt> <dd>

The depth of the inner area which does not emit any particles.

</dd> </dl>

## Ring Emitter {#Ring-Emitter}

This emitter emits particles from a ring-shaped area, i.e. a little like [Hollow Ellipsoid Emitter](#Hollow-Ellipsoid-Emitter) except only in 2 dimensions.

<dl compact="compact">
<dt>inner\_width</dt> <dd>

The width of the inner area which does not emit any particles.

</dd> <dt>inner\_height</dt> <dd>

The height of the inner area which does not emit any particles.

</dd> </dl>

# Particle Affectors {#Particle-Affectors}

Particle affectors modify particles over their lifetime. They are classified by ’type’ e.g. ’LinearForce’ affectors apply a force to all particles, whilst ’ColourFader’ affectors alter the colour of particles in flight. New affectors can be added to Ogre by creating plugins. You add an affector to a system by nesting another section within it, headed with the keyword ’affector’ followed by the name of the type of affector (case sensitive).

Particle affectors actually have no universal attributes; they are all specific to the type of affector.

@see @ref Particle-Emitters

# Standard Particle Affectors {#Standard-Particle-Affectors}

Ogre comes preconfigured with a few particle affectors. New ones can be added by creating plugins: see the Plugin\_ParticleFX project as an example of how you would do this (this is where these affectors are implemented).

-   [Linear Force Affector](#Linear-Force-Affector)
-   [ColourFader Affector](#ColourFader-Affector)
-   [ColourFader2 Affector](#ColourFader2-Affector)
-   [Scaler Affector](#Scaler-Affector)
-   [Rotator Affector](#Rotator-Affector)
-   [ColourInterpolator Affector](#ColourInterpolator-Affector)
-   [ColourImage Affector](#ColourImage-Affector)
-   [DeflectorPlane Affector](#DeflectorPlane-Affector)
-   [DirectionRandomiser Affector](#DirectionRandomiser-Affector)
-   [TextureAnimator Affector](#TextureAnimator-Affector)

## Linear Force Affector {#Linear-Force-Affector}

@copybrief Ogre::LinearForceAffector

It’s extra attributes are:

<dl compact="compact">
<dt>force\_vector</dt> <dd>

Sets the vector for the force to be applied to every particle. The magnitude of this vector determines how strong the force is.

```cpp
format:  force_vector <x> <y> <z>
example: force_vector 50 0 -50
default: 0 -100 0 (a fair gravity effect)
```

</dd> <dt>force\_application</dt> <dd>

Sets the way in which the force vector is applied to particle momentum.

```cpp
format:  force_application <add|average>
example: force_application average
default: add
```

The options are:

<dl compact="compact">
<dt>average</dt> <dd>

The resulting momentum is the average of the force vector and the particle’s current motion. Is self-stabilising but the speed at which the particle changes direction is non-linear.

</dd> <dt>add</dt> <dd>

The resulting momentum is the particle’s current motion plus the force vector. This is traditional force acceleration but can potentially result in unlimited velocity.

</dd> </dl> </dd> </dl> <br>

To create a linear force affector, include a section like this within your particle system script:

```cpp
affector LinearForce
{
    // Settings go here
}
```

## ColourFader Affector {#ColourFader-Affector}

@copybrief Ogre::ColourFaderAffector

It’s extra attributes are:

<dl compact="compact">
<dt>red</dt> <dd>

Sets the adjustment to be made to the red component of the particle colour per second.

```cpp
format:  red <delta_value>
example: red -0.1
default: 0
```

</dd> <dt>green</dt> <dd>

Sets the adjustment to be made to the green component of the particle colour per second.

```cpp
format:  green <delta_value>
example: green -0.1
default: 0
```

</dd> <dt>blue</dt> <dd>

Sets the adjustment to be made to the blue component of the particle colour per second.

```cpp
format:  blue <delta_value>
example: blue -0.1
default: 0
```

</dd> <dt>alpha</dt> <dd>

Sets the adjustment to be made to the alpha component of the particle colour per second.

```cpp
format:  alpha <delta_value>
example: alpha -0.1
default: 0
```

</dd> </dl>

To create a colour fader affector, include a section like this within your particle system script:

```cpp
affector ColourFader
{
    // Settings go here
}
```

## ColourFader2 Affector {#ColourFader2-Affector}

This affector is similar to the [ColourFader Affector](#ColourFader-Affector), except it introduces two states of colour changes as opposed to just one. The second colour change state is activated once a specified amount of time remains in the particles life.

<dl compact="compact">
<dt>red1</dt> <dd>

Sets the adjustment to be made to the red component of the particle colour per second for the first state.

```cpp
format:  red1 <delta_value>
example: red1 -0.1
default: 0
```

</dd> <dt>green1</dt> <dd>

Sets the adjustment to be made to the green component of the particle colour per second for the first state.

```cpp
format:  green1 <delta_value>
example: green1 -0.1
default: 0
```

</dd> <dt>blue1</dt> <dd>

Sets the adjustment to be made to the blue component of the particle colour per second for the first state.

```cpp
format:  blue1 <delta_value>
example: blue1 -0.1
default: 0
```

</dd> <dt>alpha1</dt> <dd>

Sets the adjustment to be made to the alpha component of the particle colour per second for the first state.

```cpp
format:  alpha1 <delta_value>
example: alpha1 -0.1
default: 0
```

</dd> <dt>red2</dt> <dd>

Sets the adjustment to be made to the red component of the particle colour per second for the second state.

```cpp
format:  red2 <delta_value>
example: red2 -0.1
default: 0
```

</dd> <dt>green2</dt> <dd>

Sets the adjustment to be made to the green component of the particle colour per second for the second state.

```cpp
format:  green2 <delta_value>
example: green2 -0.1
default: 0
```

</dd> <dt>blue2</dt> <dd>

Sets the adjustment to be made to the blue component of the particle colour per second for the second state.

```cpp
format:  blue2 <delta_value>
example: blue2 -0.1
default: 0
```

</dd> <dt>alpha2</dt> <dd>

Sets the adjustment to be made to the alpha component of the particle colour per second for the second state.

```cpp
format:  alpha2 <delta_value>
example: alpha2 -0.1
default: 0
```

</dd> <dt>state\_change</dt> <dd>

When a particle has this much time left to live, it will switch to state 2.

```cpp
format:  state_change <seconds>
example: state_change 2
default: 1
```

</dd> </dl>

To create a ColourFader2 affector, include a section like this within your particle system script:

```cpp
affector ColourFader2
{
    // Settings go here
}
```

## Scaler Affector {#Scaler-Affector}

@copybrief Ogre::ScaleAffector

It’s extra attributes are:


@par rate
The amount by which to scale the particles in both the x and y direction per second.

```cpp
format:  rate <xy_rate>
example: rate 3.5
default: 0
```

@par multiply_mode
@parblock
How we should scale. When false, every tick we do:
```cpp
particle_scale += rate * timeSinceLast;
```

When true, every tick we do:
```cpp
particle_scale *=  pow( rate, timeSinceLast );
```

Multiply mode is useful for particles that always grow at a constant rate.
Non-multiply mode is useful if you want the growth rate to slow down over time (e.g. 10 + 100 makes the particle grow by 10x, but on the next tick 100 + 100 means the particle grows by 2x).

```cpp
format:  multiply_mode <true|false>
example: multiply_mode true
default: false
```
@endparblock

To create a scale affector, include a section like this within your particle system script:

```cpp
affector Scaler
{
    // Settings go here
}
```

## Rotator Affector {#Rotator-Affector}

@copybrief Ogre::RotationAffector

It’s extra attributes are:

<dl compact="compact">
<dt>rotation\_speed\_range\_start</dt> <dd>

The start of a range of rotation speeds to be assigned to emitted particles.

```cpp
format:  rotation_speed_range_start <degrees_per_second>
example: rotation_speed_range_start 90
default: 0
```

</dd> <dt>rotation\_speed\_range\_end</dt> <dd>

The end of a range of rotation speeds to be assigned to emitted particles.

```cpp
format:  rotation_speed_range_end <degrees_per_second>
example: rotation_speed_range_end 180
default: 0
```

</dd> <dt>rotation\_range\_start</dt> <dd>

The start of a range of rotation angles to be assigned to emitted particles.

```cpp
format:  rotation_range_start <degrees_per_second>
example: rotation_range_start 0
default: 0
```

</dd> <dt>rotation\_range\_end</dt> <dd>

The end of a range of rotation angles to be assigned to emitted particles.

```cpp
format:  rotation_range_end <degrees_per_second>
example: rotation_range_end 360
default: 0
```

</dd> </dl>

To create a rotate affector, include a section like this within your particle system script:

```cpp
affector Rotator
{
    // Settings go here
}
```

## ColourInterpolator Affector {#ColourInterpolator-Affector}

Similar to the ColourFader and ColourFader2 Affectors, this affector modifies the colour of particles in flight, except it has a variable number of defined stages. It swaps the particle colour for several stages in the life of a particle and interpolates between them.

It’s extra attributes are:

<dl compact="compact">
<dt>time0</dt> <dd>

The point in time of stage 0. Must be in range \[0; 1\].

```cpp
format:  time0 <0-1>
example: time0 0
default: 1
```

</dd> <dt>colour0</dt> <dd>

The colour at stage 0.

```cpp
format:  colour0 <r> <g> <b> <a>
example: colour0 1 0 0 1
default: 0.5 0.5 0.5 0.0
```

</dd> <dt>time1</dt> <dd>

The point in time of stage 1. Must be in range \[0; 1\].

```cpp
format:  time1 <0-1>
example: time1 0.5
default: 1
```

</dd> <dt>colour1</dt> <dd>

The colour at stage 1

```cpp
format:  colour1 <r> <g> <b> <a>
example: colour1 0 1 0 1
default: 0.5 0.5 0.5 0.0
```

</dd> <dt>time2</dt> <dd>

The point in time of stage 2. Must be in range \[0; 1\].

```cpp
format:  time1 <0-1>
example: time2 1
default: 1
```

</dd> <dt>colour2</dt> <dd>

The colour at stage 2.

```cpp
format:  colour2 <r> <g> <b> <a>
example: colour2 0 0 1 1
default: 0.5 0.5 0.5 0.0
```

</dd> </dl>

@note The number of stages is variable. The maximal number of stages is 6; where time5 and colour5 are the last possible parameters.

To create a colour interpolation affector, include a section like this within your particle system script:

```cpp
affector ColourInterpolator
{
    // Settings go here
}
```

## ColourImage Affector {#ColourImage-Affector}

This is another affector that modifies the colour of particles in flight, but instead of programmatically defining colours, the colours are taken from a specified image file. The range of colour values begins from the left side of the image and move to the right over the lifetime of the particle, therefore only the horizontal dimension of the image is used.

Its extra attributes are:

<dl compact="compact">
<dt>image</dt> <dd>

The start of a range of rotation speed to be assigned to emitted particles.

```cpp
format:  image image_name
example: image rainbow.png
default: none
```

</dd> </dl>

To create a ColourImage affector, include a section like this within your particle system script:

```cpp
affector ColourImage
{
    // Settings go here
}
```

## DeflectorPlane Affector {#DeflectorPlane-Affector}

@copybrief Ogre::DeflectorPlaneAffector

The attributes are:

<dl compact="compact">
<dt>plane\_point</dt> <dd>

A point on the deflector plane. Together with the normal vector it defines the plane.

```cpp
format:  plane_point <x> <y> <z>
example: plane_point 0 0 0
default: 0 0 0
```

</dd> <dt>plane\_normal</dt> <dd>

The normal vector of the deflector plane. Together with the point it defines the plane.

```cpp
format:  plane_normal <x> <y> <z>
example: plane_normal 0 1 0
default: 0 1 0
```

</dd> <dt>bounce</dt> <dd>

The amount of bouncing when a particle is deflected. 0 means no deflection and 1 stands for 100 percent reflection.

```cpp
format:  bounce <value>
example: bounce 0.5
default: 1.0
```

</dd> </dl>

## DirectionRandomiser Affector {#DirectionRandomiser-Affector}

@copybrief Ogre::DirectionRandomiserAffector

Its extra attributes are:

@par randomness
@copybrief Ogre::DirectionRandomiserAffector::setRandomness
```cpp
format:  randomness <value>
example: randomness 0.5
default: 1.0
```

@par scope
@copybrief Ogre::DirectionRandomiserAffector::setScope
```cpp
format:  scope <value>
example: scope 0.5
default: 1.0
```

@par keep_velocity
@copybrief Ogre::DirectionRandomiserAffector::setKeepVelocity
Default: keep_velocity false
```cpp
format:  keep_velocity <true|false>
example: keep_velocity true
default: false
```
