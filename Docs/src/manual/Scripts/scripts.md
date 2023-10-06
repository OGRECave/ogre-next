# Scripts {#Scripts}

OGRE-Next drives many of its features through scripts in order to make it easier to set up. The scripts are simply plain text files which can be edited in any standard text editor, and modifying them immediately takes effect on your OGRE-Next-based applications, without any need to recompile. This makes prototyping a lot faster. Here are the items that OGRE-Next lets you script:

- @subpage JSON-Materials
- @subpage Material-Scripts
- @subpage High-level-Programs
- @subpage compositor [Scripts](@ref compositor)
- @subpage Particle-Scripts
- @subpage v1_Overlays

@tableofcontents

# Loading scripts

Scripts are loaded when resource groups are initialised: OGRE-Next looks in all resource locations associated with the group (see Ogre::ResourceGroupManager::addResourceLocation) for files with the respective extension (e.g. ’.material’, ’.material.json’, ’.compositor’, ..) and parses them. If you want to parse files manually, use Ogre::ScriptCompilerManager::parseScript.

The file extension does not actually restrict the items that can be specified inside the file; e.g. %Ogre is perfectly fine with loading a particle-system from a ’.compositor’ file - but it will lead you straight to maintenance-hell if you do that.
The extensions, however, do specify the order in which the scripts are parsed, which is as follows:

1. *.material.json
2. *.program
3. *.material
4. *.particle
5. *.particle2
6. *.compositor
7. *.os
8. *.overlay

# Format {#Format}

Several script objects may be defined in a single file. The script format is pseudo-C++, with sections delimited by curly braces ({}), and comments indicated by starting a line with ’//’. The general format is shown below:

```cpp
// This is a comment
object_keyword Example/ObjectName
{
    attribute_name "some value"

    object_keyword2 "Nested Object"
    {
        other_attribute 1 2 3
        // and so on..
    }
}
```

Every script object must be given a name, which is the line ’object_keyword &lt;name&gt;’ before the first opening ’{’. This name must be globally unique. It can include path characters (as in the example) to logically divide up the objects, and also to avoid duplicate names, but the engine does not treat the name as hierarchical, just as a string. Names can include spaces but must be surrounded by double quotes i.e. `compositor "My Name"`.

@note ’:’ is the delimiter for specifying inheritance in the script so it can’t be used as part of the name.

An script object can inherit from a previously defined object by using a *colon* ’:’ after the name followed by the name of the reference object to inherit from. You can in fact even inherit just *parts* of a script object from others; all this is covered in @ref Script-Inheritance). You can also use variables in your script which can be replaced in inheriting versions, see @ref Script-Variables.

## Script Inheritance {#Script-Inheritance}

When creating new script objects that are only slight variations of another object, it’s good to avoid copying and pasting between scripts. Script inheritance lets you do this; in this section we’ll use material scripts as an example, but this applies to all scripts parsed with the script compilers in %Ogre 1.6 onwards.

For example, to make a new material that is based on one previously defined, add a *colon* ’:’ after the new material name followed by the name of the material that is to be copied.

Example
```cpp
material <NewUniqueChildName> : <ReferenceParentMaterial>
```

The only caveat is that a parent material must have been defined/parsed prior to the child material script being parsed. The easiest way to achieve this is to either place parents at the beginning of the material script file, or to use the @ref Script-Import-Directive. Note that inheritance is actually a copy - after scripts are loaded into Ogre, objects no longer maintain their copy inheritance structure. If a parent material is modified through code at runtime, the changes have no effect on child materials that were copied from it in the script.

Material copying within the script alleviates some drudgery from copy/paste but having the ability to identify specific techniques, passes, and texture units to modify makes material copying easier. Techniques, passes, texture units can be identified directly in the child material without having to layout previous techniques, passes, texture units by associating a name with them, Techniques and passes can take a name and texture units can be numbered within the material script. You can also use variables, See @ref Script-Variables.

Names become very useful in materials that copy from other materials. In order to override values they must be in the correct technique, pass, texture unit etc. The script could be lain out using the sequence of techniques, passes, texture units in the child material but if only one parameter needs to change in say the 5th pass then the first four passes prior to the fifth would have to be placed in the script:

Here is an example:

```cpp
material test2 : test1
{
  technique
  {
    pass
    {
    }

    pass
    {
    }

    pass
    {
    }

    pass
    {
    }

    pass
    {
      ambient 0.5 0.7 0.3 1.0
    }
  }
}
```

This method is tedious for materials that only have slight variations to their parent. An easier way is to name the pass directly without listing the previous passes:<br>

```cpp
material test2 : test1
{
  technique
  {
    pass "Fifth Pass"
    {
      ambient 0.5 0.7 0.3 1.0
    }
  }
}
```

The parent pass name must be known and the pass must be in the correct technique in order for this to work correctly. Specifying the technique name and the pass name is the best method.

### Advanced Script Inheritance {#Advanced-Script-Inheritance}

Script objects can also inherit from each other more generally. The previous concept of inheritance, material copying, was restricted only to the top-level material objects. Now, any level of object can take advantage of inheritance (for instance, techniques, passes, and compositor targets).

```cpp
material Test
{
    technique
    {
        pass : ParentPass
        {
        }
    }
}
```

Notice that the pass inherits from ParentPass. This allows for the creation of more fine-grained inheritance hierarchies.

Along with the more generalized inheritance system comes an important new keyword: "abstract." This keyword is used at a top-level object declaration (not inside any other object) to denote that it is not something that the compiler should actually attempt to compile, but rather that it is only for the purpose of inheritance. For example, a material declared with the abstract keyword will never be turned into an actual usable material in the material framework. Objects which cannot be at a top-level in the document (like a pass) but that you would like to declare as such for inheriting purpose must be declared with the abstract keyword.

```cpp
abstract pass ParentPass
{
    diffuse 1 0 0 1
}
```

That declares the ParentPass object which was inherited from in the above example. Notice the abstract keyword which informs the compiler that it should not attempt to actually turn this object into any sort of Ogre resource. If it did attempt to do so, then it would obviously fail, since a pass all on its own like that is not valid.

The final matching option is based on wildcards. Using the ’\*’ character, you can make a powerful matching scheme and override multiple objects at once, even if you don’t know exact names or positions of those objects in the inherited object.

```cpp
abstract technique Overrider
{
   pass *colour*
   {
      diffuse 0 0 0 0
   }
}
```

This technique, when included in a material, will override all passes matching the wildcard "\*color\*" (color has to appear in the name somewhere) and turn their diffuse properties black. It does not matter their position or exact name in the inherited technique, this will match them.

## Script Variables {#Script-Variables}

A very powerful new feature in Ogre 1.6 is variables. Variables allow you to parameterize data in materials so that they can become more generalized. This enables greater reuse of scripts by targeting specific customization points. Using variables along with inheritance allows for huge amounts of overrides and easy object reuse.

```cpp
abstract pass ParentPass
{
   diffuse $diffuse_colour
}

material Test
{
   technique
   {
       pass : ParentPass
       {
           set $diffuse_colour "1 0 0 1"
       }
   }
}
```

The ParentPass object declares a variable called "diffuse\_colour" which is then overridden in the Test material’s pass. The "set" keyword is used to set the value of that variable. The variable assignment follows lexical scoping rules, which means that the value of "1 0 0 1" is only valid inside that pass definition. Variable assignment in outer scopes carry over into inner scopes.

```cpp
material Test
{
    set $diffuse_colour "1 0 0 1"
    technique
    {
        pass : ParentPass
        {
        }
    }
}
```

The $diffuse\_colour assignment carries down through the technique and into the pass. 

## Script Import Directive {#Script-Import-Directive}

Imports are a feature introduced to remove ambiguity from script dependencies. When using scripts that inherit from each other but which are defined in separate files sometimes errors occur because the scripts are loaded in incorrect order. Using imports removes this issue. The script which is inheriting another can explicitly import its parent’s definition which will ensure that no errors occur because the parent’s definition was not found.

```cpp
import * from "parent.material"
material Child : Parent
{
}
```

The material "Parent" is defined in parent.material and the import ensures that those definitions are found properly. You can also import specific targets from within a file.

```cpp
import Parent from "parent.material"
```

If there were other definitions in the parent.material file, they would not be imported.

@note Importing does not actually cause objects in the imported script to be fully parsed & created, it just makes the definitions available for inheritance. This has a specific ramification for vertex / fragment program definitions, which must be loaded before any parameters can be specified. You should continue to put common program definitions in .program files to ensure they are fully parsed before being referenced in multiple .material files. The ’import’ command just makes sure you can resolve dependencies between equivalent script definitions (e.g. material to material).

# Custom Translators {#custom-translators}
Writing a custom translators allows you to extend Ogre's standard compilers with completely new functionality. The same scripting interfaces can be used to define application-specific functionality. Here's how you do it.

The first step is creating a custom translator class which extends Ogre::ScriptTranslator.

@snippet Components/Overlay/src/OgreOverlayTranslator.h font_translator

This class defines the important function to override: translate. This is called when the TestTranslator needs to process a sub-set of the parsed script. The definition of this function might look something like this:

@snippet Components/Overlay/src/OgreOverlayTranslator.cpp font_translate

The translate function here expects all children to be atomic properties. Sub-objects can also be processed by checking if the child node type is Ogre::ANT_OBJECT.

From here you need to register the translator to be invoked when the proper object is found in the scripts. To do this we need to create a Ogre::ScriptTranslatorManager object to create your custom translator. The relevant parts look like this:

@snippet Components/Overlay/src/OgreOverlayTranslator.cpp font_register

Note how we use Ogre::ScriptCompilerManager::registerCustomWordId to avoid string comparisons in our code.

@snippet Components/Overlay/src/OgreOverlayTranslator.cpp font_get_translator

No new translators are created here, just returned when requested. This is because our translator does not require separate instances to properly parse scripts, and so it is easier to only create one instance and reuse it. Often this strategy will work.

The order that custom translator managers are registered will make a difference. When the system is attempting to find translators to handle pieces of a script, it will query the translator managers one-by-one until it finds one that handles that script object. It is a first-come-first-served basis.

An important note is that this will recognize the above pattern no matter where in the script it is. That means that this may appear at the top-level of a script or inside several sub-objects. If this is not what you want then you can change the translator manager to do more advanced processing in the getTranslator function.
