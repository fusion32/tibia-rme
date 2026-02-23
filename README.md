# Tibia Remere's Map Editor 7.7.0
This is a map editor, based on the original Remere's Map Editor, modified to support [Tibia Game Server](https://github.com/fusion32/tibia-game). It should have the same features as the original RME, but I do expect problems so any issues should be submitted to the issue tracker with the appropriate description.

# Usage
Although most of the original features are preserved, the notion of what is a map has drastically changed. The editor will now load a "project" directory, rather than a single map file, which should contain all files related to the project. This includes an `editor` directory with files related to editor menu, materials, and sprites. The sample `editor` in this repository was converted from the former `data/760`, and can be used as a starting point, although it is missing sprite files for obvious reasons.

A minimal layout would look like this:
```
project/
├── dat/
│   ├── houseareas.dat
│   ├── houses.dat
│   ├── map.dat
│   ├── mem.dat
│   ├── monster.db
│   └── objects.srv
├── editor/
│   ├── menubar.xml
│   ├── materials.xml
│   ├── Tibia.dat
│   └── Tibia.spr
├── mon/
│   └── *.mon
├── origmap/
│   └── *.sec
└── save/
```

The workflow has also drastically changed. The baseline map is kept in `project/origmap` and changes to it are kept in `project/save` as patches. The editor will load these patches along with the baseline map to reconstruct the current saved version. They should be considered as integral part of the map. Spawns (`project/dat/monster.db`), houses (`project/dat/houseareas.dat` and `project/dat/houses.dat`), and marks (`project/dat/map.dat`) are stored whole in their own files. Backups are always made before saving to make sure there is always a fallback in case things go wrong.

When you're ready to export a patch, the `Export Patch...` tool can be used to generate a final ZIP containing all the relevant files that could have been modified. There is also an option to commit patches to the local baseline map (`project/origmap`) and is somewhat discussed in the next two paragraphs.

The game server should be able to read the modified spawns, houses, and marks files as normal. The big difference, and the reason why the map is stored the way it is, is that the any patches in `game/save` directory are consumed **at startup** and applied to both persistent (`game/map`) and baseline (`game/origmap`) maps. Once it's done, it's done, and unless you made backup of both, there is no coming back. Next time you edit the project's map, you should have the project's baseline map synced with the server's, which is why you should commit patches locally when exporting to the server.

But... If you're familiar with the server's layout, you'll notice that it's pretty much the same as the project's layout. If you add the `editor` directory, it should load as a project without too much hassle. One thing to be aware tho, is that patches in `game/save` will be consumed everytime you startup the server, so you should make sure you're not editing the map and running the server at the same time, or at least that you reload the project after starting the server. This is one example where you don't want to commit patches locally when exporting a patch, nor exporting patches would make too much sense.

For a robust setup, you should have the project's directory separate from the server's.

# Problems and Missing Features
- Houses and Marks are still not supported, even though their files were mentioned above.
- NPCs are not supported because they're unique, and specify their spawn position in their own files. It would require us to parse, modify, and save their files to fully support moving them around.
- Adding, removing, and moving spawns will mark tiles as dirty. This will cause a patch to be generated for the involved tiles, even though no items have been modified. This has to do with how the action queue works currently and how spawns and items are tied together inside a tile. I have some thoughts on how to fix this but I still need to refine the idea.

# Compiling (Windows)
There are probably multiple ways to compile on Windows but the CMake+VCPKG combo is probably the simplest so it's the one I'm using here. The only setup is to have GIT, MSVC, and VCPKG installed. Note that the MSVC installer will have options to get CMake and VCPKG, but for VCPKG specifically you'll probably want to get the standalone version, where you clone the repo and follow some instructions to bootstrap it. You'll also need to make sure the `VCPKG_ROOT` environment variable is propertly set to the root directory of the VCPKG installation, so CMake can find it.

If your setup is correct, you should be able to open the shell, traverse to a sensible directory, and run the commands below. If it complains about not finding `cl.exe` or any other command, then you might need to run on top of the MSVC shell (`x64 Native Tools Command Prompt for VS20XX`).

```
git clone https://github.com/fusion32/tibia-rme.git
cd tibia-rme
cmake -B build --preset vcpkg-windows-static
cmake --build build --config Release -j 4
```

# Compiling (Linux)
The simplest way is to install FreeGLUT, wxWidgets, and ZLib from the package manager, then use CMake to configure and build.

```
# PACKAGES
pacman -S freeglut wxwidgets-gtk3 zlib               # ARCH
apt install freeglut3-dev libwxgtk3.2-dev zlib1g-dev # DEBIAN STABLE

# BUILD
git clone https://github.com/fusion32/tibia-rme.git
cd tibia-rme
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
```

