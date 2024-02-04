#!/usr/bin/env python

import subprocess

def compressTo7z( branchName, generatedFilenames ):
	print( "Compressing to 7z..." )
	cmds = ["7z", "a", "../output/build_ogre_scripts-" + branchName + ".7z"]
	cmds.extend( generatedFilenames )
	retCode = subprocess.call( cmds, cwd='./output' )
	if retCode != 0:
		print( "Warning: 7z " + cmds + "failed" )
		exit()
	else:
		print( "7z finished" )

def getMercurialBranchName():
	print( 'Retrieving Mercurial bookmark name' )
	process = subprocess.Popen( ['hg', 'log', '--template', '{bookmarks}\n', '-r', 'bookmark() & .'], stdout=subprocess.PIPE )
	(output, err) = process.communicate()
	exitCode = process.wait()

	if exitCode == 0:
		output = output.decode()
		branchName = output.replace( '\n', '' )
		return branchName
	else:
		return None

def getGitBranchName():
	print( 'Retrieving git branch name' )
	process = subprocess.Popen( ['git', 'rev-parse', '--abbrev-ref', 'HEAD'], stdout=subprocess.PIPE )
	(output, err) = process.communicate()
	exitCode = process.wait()

	if exitCode == 0:
		output = output.decode()
		branchName = output.replace( '\n', '' )
		return branchName
	else:
		return None

branchName = getMercurialBranchName()

if branchName == None:
	print( 'Mercurial failed. This is likely not a Mercurial repo' )
	branchName = getGitBranchName()

if branchName == None:
	print( 'Failed to retrieve branch name. Cannot continue.' )
	exit( 1 )

generatedFilenames = []

print( 'Branch name is: ' + branchName )

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
	'Visual Studio 17 2022',
	'Visual Studio 16 2019',
	'Visual Studio 15 2017',
	'Visual Studio 14 2015',
	'Visual Studio 12 2013'
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
		generatedFilenames.append( filename )
		file = open( './output/' + filename, 'wt' )
		file.write( batchScript )
		file.close()

print( 'Generating scripts for Linux' )

cppVersions = \
[
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
	generatedFilenames.append( filename )
	batchScript = templateStr.format( branchName, cppVersionParam )
	path = './output/' + filename
	file = open( path, 'wt' )
	file.write( batchScript )
	os.chmod( path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH | stat.S_IWUSR | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH )
	file.close()

applePlatforms = \
[
	'macOS',
	'iOS'
]

for applePlatform in applePlatforms:
	print( 'Generating scripts for ' + applePlatform )

	filename = 'build_ogre_{0}.sh'.format( applePlatform )

	file = open( filename, 'rt' )
	templateStr = file.read()

	generatedFilenames.append( filename )
	batchScript = templateStr.format( branchName )
	path = './output/' + filename
	file = open( path, 'wt' )
	file.write( batchScript )
	os.chmod( path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH | stat.S_IWUSR | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH )
	file.close()

compressTo7z( branchName, generatedFilenames )

print( 'Done' )
