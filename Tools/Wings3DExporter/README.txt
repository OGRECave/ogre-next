
WHAT

This is wings-ogre, my Wings3D to OGRE converter written in Python.

Please send feedback, suggestions and bug reports to:

Attila Tajti <attis@spacehawks.hu>

I'm also used to be around #Ogre3D on irc.freenode.net.


FEATURES

It's basically a couple of Python modules to load and process the
structured Erlang binary file produced by Wings and save it in 
Ogre XML. It does the following:

- loads the polygon data
- handles UV coordinates, material
diffuse/ambient/specular/shininess values
- handle copying vertices at hard edges (like smoothing groups in Max)
- triangulates polygons using the ear-clipping method
- writes internal images in the .wings file into PNG format
- uses an own primitiev XML module, so no libxml2 dependency 


USAGE

Copy the .wings file to the directory containing the python files. Then
execute ./w2o.py <files> from the command line, eg:

	% tar -xzf wings-ogre-0.9.tar.gz
	% cd wings-ogre-0.9
	% cp ~/models/test.wings
	% ./w2o.py test.wings

This will result in a .mesh.xml file. It has to be converted to a .mesh 
via the the XML converter:

	% OgreXMLConverter test.mesh.xml

The w2o.py program will also export internal Wings textures, using the name
of the material (not the name of the image) as filename.


REQUIREMENTS

(These should be available on a recent Linux system)

Python 2.2
Python Imaging Library (for Image export)


TODO

Speed things up. The exporter is terribly slow. This is mainly because
of the soft/hard edge detection stuff.


NOTES

The xmlout module may be used using Temas's Blender exporter to remove
the libxml2 dependency. Don't use xmlout for anything more serious, though.


ACKNOWLEDGEMENTS

The work of these people were particularily helpful for me:

Thomas 'temas' Muldowney - the OGRE Blender exporter
Anthony D'Agostino - IO Suide for Blender


LINKS

http://ogre.sf.net - OGRE homepage

http://www.wings3d.com - Wings 3D homepage

http://www.python.org - Python homepage

http://www.pythonware.com/products/pil/ - Python Imaging Library homepage


