#!/usr/bin/env python

# Get the branch name from Mercurial
print( 'Retrieving Mercurial branch name' )
from subprocess import Popen, PIPE

process = Popen( ['hg', 'identify', '-b'], stdout=PIPE )
(output, err) = process.communicate()
exitCode = process.wait()

branchName = output.replace( '\n', '' )

print( 'Branch name is: ' + output )

import os
import stat
import errno

try:
	os.makedirs( './output' )
except OSError as exc:
	if exc.errno == errno.EEXIST and os.path.isdir( './output' ):
		pass
	else:
		raise

# Generate Windows scripts

print( 'Generating scripts for Windows' )

generators = \
[
	'Visual Studio 16 2019',
	'Visual Studio 15 2017',
	'Visual Studio 14 2015',
	'Visual Studio 12 2013',
	'Visual Studio 11 2012',
	'Visual Studio 10 2010',
	'Visual Studio 9 2008'
]

platforms = \
[
	'Win32',
	'x64'
]

file = open( 'build_ogre.bat', 'rt' )
templateStr = file.read()

for generator in generators:
	for platform in platforms:
		batchScript = templateStr.format( branchName, generator, platform )
		generatorUnderscores = generator.replace( ' ', '_' )
		filename = 'build_ogre_{0}_{1}.bat'.format( generatorUnderscores, platform )
		file = open( './output/' + filename, 'wt' )
		file.write( batchScript )

print( 'Done' )
print( 'Generating scripts for Linux' )

cppVersions = \
[
	98,
	11,
	0
]

file = open( 'build_ogre_linux.sh', 'rt' )
templateStr = file.read()

for cppVersion in cppVersions:
	if cppVersion == 0:
		cppVersionParam = ''
		filename = 'build_ogre_linux_c++latest.sh'.format( cppVersionParam )
	else:
		cppVersionParam = '-D CMAKE_CXX_STANDARD=' + str( cppVersion )
		filename = 'build_ogre_linux_c++{0}.sh'.format( cppVersion )
	batchScript = templateStr.format( branchName, cppVersionParam )
	path = './output/' + filename
	file = open( path, 'wt' )
	file.write( batchScript )
	os.chmod( path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH | stat.S_IWUSR | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH )

print( 'Done' )
