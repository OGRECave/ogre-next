#!/usr/bin/env python3

from multiprocessing.pool import ThreadPool
from subprocess import call

arguments = [
    ["python3",
        "clone_datablock.py",
        "../../Components/Hlms/Pbs/include/OgreHlmsPbsDatablock.h",
        "-I", "../../Components/Hlms/Common/include",
        "-I", "../../Components/Hlms/Pbs/include",
        "-I", "../../OgreMain/include/",
        "-I", "../../build/include",
        "-I", "../../build/Debug/include",
        "-I", "../../build/Release/include",
     ],
    ["python3",
     "clone_datablock.py",
     "../../Components/Hlms/Unlit/include/OgreHlmsUnlitDatablock.h",
     "-I", "../../Components/Hlms/Common/include",
     "-I", "../../Components/Hlms/Unlit/include",
     "-I", "../../OgreMain/include/",
     "-I", "../../build/include",
     "-I", "../../build/Debug/include",
     "-I", "../../build/Release/include",
     ],
    ["python3",
     "clone_datablock.py",
     "../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Hlms/OgreHlmsTerraDatablock.h",
     "-I", "../../Components/Hlms/Common/include",
     "-I", "../../Samples/2.0/Tutorials/Tutorial_Terrain/include",
     "-I", "../../Samples/2.0/Tutorials/Tutorial_Terrain/include/Terra/Hlms",
     "-I", "../../OgreMain/include/",
     "-I", "../../build/include",
     "-I", "../../build/Debug/include",
     "-I", "../../build/Release/include",
     ],
    ["python3",
     "clone_particle_systems2.py",
     "-I", "../../OgreMain/include/",
     "-I", "../../PlugIns/ParticleFX2/include/",
     "-I", "../../build/include",
     "-I", "../../build/Debug/include",
     "-I", "../../build/Release/include"
     ],
    ["python3",
     "clone_particle_emitters.py",
     "-I", "../../OgreMain/include/",
     "-I", "../../PlugIns/ParticleFX2/include/",
     "-I", "../../build/include",
     "-I", "../../build/Debug/include",
     "-I", "../../build/Release/include"
     ],
    ["python3",
     "clone_particle_affectors.py",
     "-I", "../../OgreMain/include/",
     "-I", "../../PlugIns/ParticleFX2/include/",
     "-I", "../../build/include",
     "-I", "../../build/Debug/include",
     "-I", "../../build/Release/include"
     ]
]


def runInThread(args):
    call(args)


pool = ThreadPool()
result = pool.map(runInThread,  arguments)
