
from subprocess import call

call( ["python2", \
        "clone_datablock.py", \
        "../../Components/Hlms/Pbs/include/OgreHlmsPbsDatablock.h", \
        "-I", "../../Components/Hlms/Common/include", \
        "-I", "../../Components/Hlms/Pbs/include", \
        "-I", "../../OgreMain/include/", \
        "-I", "../../build/include", \
        "-I", "../../build/Debug/include", \
        "-I", "../../build/Release/include", \
        ] )
call( ["python2", \
        "clone_datablock.py", \
        "../../Components/Hlms/Unlit/include/OgreHlmsUnlitDatablock.h", \
        "-I", "../../Components/Hlms/Common/include", \
        "-I", "../../Components/Hlms/Unlit/include", \
        "-I", "../../OgreMain/include/", \
        "-I", "../../build/include", \
        "-I", "../../build/Debug/include", \
        "-I", "../../build/Release/include", \
        ] )
call( ["python2", \
        "clone_datablock.py", \
        "../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Hlms/OgreHlmsTerraDatablock.h", \
        "-I", "../../Components/Hlms/Common/include", \
        "-I", "../../Samples/2.0/Tutorials/Tutorial_Terrain/include", \
        "-I", "../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Hlms", \
        "-I", "../../OgreMain/include/", \
        "-I", "../../build/include", \
        "-I", "../../build/Debug/include", \
        "-I", "../../build/Release/include", \
        ] )

call(["python3",
      "clone_particle_systems.py",
      "../../OgreMain/include/ParticleSystem/OgreEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgreAreaEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgreBoxEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgreCylinderEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgreEllipsoidEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgreHollowEllipsoidEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgrePointEmitter2.h",
      "../../PlugIns/ParticleFX2/include/OgreRingEmitter2.h",
      "-I", "../../OgreMain/include/",
      "-I", "../../PlugIns/ParticleFX2/include/",
      "-I", "../../build/include",
      "-I", "../../build/Debug/include",
      "-I", "../../build/Release/include"
      ])
