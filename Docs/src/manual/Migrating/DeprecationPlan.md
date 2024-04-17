# Deprecation Plan {#DeprecationPlan}

The following is a tracker of all deprecated settings with a plan to phase out certain settings or features in an ordered fashion.

The plan is not strict, not set in stone and may change in the future, specially if user needs demand it. If we plan to remove something in e.g. version 5.0, the idea is that it will be removed in version 5.0 or later, but not earlier.

## CMake Option

| CMake Option                           | Introduced                 | Deprecated                         | Removed |
|----------------------------------------|----------------------------|------------------------------------|---------|
| [OGRE_USE_NEW_PROJECT_NAME](@ref Ogre30Changes)              | 2.3.3<br/>Defaults to FALSE. | 4.0<br/>Defaults to TRUE | 5.0     |
| [OGRE_SHADER_COMPILATION_THREADING_MODE](@ref HlmsThreading_CMakeOptions) | 4.0<br/>Defaults to 1      | 5.0<br/>Defaults to 2.             | 6.0     |

## Functions marked with `OGRE_DEPRECATED_VER`

When you see a function marked like this:

```cpp
OGRE_DEPRECATED_VER( 3 ) void setPerceptualRoughness( bool bPerceptualRoughness );
```

The number indicates the function has been marked as deprecated since 3.0, and is not guaranteed to exist in versions >= 4.0.