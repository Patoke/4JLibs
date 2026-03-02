# 4JLibs

A project that aims at rebuilding the 4J libraries source code via decompilation for the Minecraft: Legacy Console Edition

## Why?

This would allow compiling the Minecraft: Legacy Console Edition source code from newer versions of Visual Studio, expand the Renderer code, add new Input support, etc...

This would also help document the structure of their projects for decompilation projects of newer versions of this version of the game

## Does this use leaked code?

No, this does not use any code from the Minecraft: Legacy Console Edition source leak, this is all clean decompilation from binaries and debug binaries

## How can I build this?

You will need to get your hands with files from the source leak that are not included here or rebuild them and push to the repository

The files needed are the following:
* 4J_Input.h
* 4J_Storage.h
* 4J_Render.h
* 4J_Profile.h
* extraX64.h

You will need to modify ``extraX64.h`` as it has a couple errors

You will need to add every file into their respective project and add the ``extraX64.h`` header inside ``Profile`` and ``Storage``

## What is implemented?

All projects can be linked against the main game code, whilst there's some unnamed stuff in the Renderer, this works just fine and the game can be played