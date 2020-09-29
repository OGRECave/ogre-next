#!/usr/bin/env python

import os
import shutil

keep_files = [
	'api',
	'images',
	'javascripts',
	'stylesheets',
	'vtests',
	'.git',
	'_config.yml',
	'index.md',
	'ogre-logo.png',
	'.nojekyll',
]

for root, dirs, filenames in os.walk( '../../' ):
	for fileName in filenames:
		if fileName not in keep_files:
			toRemove = os.path.join( root, fileName )
			print( 'Removing ' + toRemove )
			os.unlink( toRemove )
	for dir in dirs:
		if dir not in keep_files:
			toRemove = os.path.join( root, dir )
			print( 'Removing ' + toRemove )
			shutil.rmtree( toRemove )
	break