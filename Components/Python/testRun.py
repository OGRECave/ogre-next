import Ogre

# Ogre & rendering window init

root = Ogre.Root("../../bin/plugins.cfg", "../../bin/ogre.cfg", "/tmp/ogre.log")
root.restoreConfig() or root.showConfigDialog()
window = root.initialise(True, "Ogre Python")

# resources init

Ogre.initHLMS("/usr/local/share/OGRE/Media/")

# scene manager init

scnMgr = root.createSceneManager("DefaultSceneManager", 1, "testSceneMenager")

# scene setup

if "mode" in globals() and mode == "loading screen":
	for path in ["resources/LoadingScreen", "resources/LoadingScreen/scripts", "resources/LoadingScreen/shaders"]:
		path = "/01-Game/SRC/" + path
		Ogre.ResourceGroupManager.getSingleton().addResourceLocation( path, "FileSystem", "LoadingScreen", False )
	Ogre.ResourceGroupManager.getSingleton().initialiseResourceGroup("LoadingScreen", True);
	cameraName = "LoadingScreen"
else:
	Ogre.ResourceGroupManager.getSingleton().addResourceLocation( "/01-Game/resources/SampleMod/Media/Models", "FileSystem", "SceneAssets", True )
	Ogre.ResourceGroupManager.getSingleton().initialiseResourceGroup("SceneAssets", True);
	
	node = scnMgr.getRootSceneNode().createChildSceneNode()
	item = scnMgr.createItem("Dragon/dragon.mesh");
	node.attachObject(item)
	node.setPosition(20, 20, 20)
	
	#node = scnMgr.getRootSceneNode().createChildSceneNode()
	#light = scnMgr.createLight()
	#node.attachObject(light)
	#node.setPosition(10, 10, 10)
	#light.setType(Ogre.Light.LT_POINT)
	#light.setDiffuseColour(Ogre.ColourValue(0, 0, 1))
	#light.setSpecularColour(Ogre.ColourValue(1, 0, 0))
	#light.setAttenuationBasedOnRadius(20, 0)
	
	scnMgr.setForwardClustered( True, 16, 16, 8, 8, 4, 4, 1, 32 );
	scnMgr.setAmbientLight(Ogre.ColourValue(.5, .5, .5, 1), Ogre.ColourValue(.5, .5, .5, 1), Ogre.Vector3(0, 1, 0), 1)
	
	cameraName = "LoadingScreen"

# camera init

camera = scnMgr.createCamera(cameraName)
camera.lookAt(Ogre.Vector3(10,10,10))
camera.setNearClipDistance(1)

# compositor init

compMgr = root.getCompositorManager2()
if not compMgr.hasWorkspaceDefinition(Ogre.IdString("Workspace"+cameraName)):
	compMgr.createBasicWorkspaceDef("Workspace"+cameraName, Ogre.ColourValue( 0.0, 0.4, 0.0 ));

workspace = compMgr.addWorkspace( scnMgr, window.getTexture(), camera, Ogre.IdString("Workspace"+cameraName), True, -1 )

# wait for textures and render

root.getRenderSystem().getTextureGpuManager().waitForStreamingCompletion()
root.renderOneFrame()
