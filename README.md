# 4JLibs

A project that aims at rebuilding the 4J libraries source code via decompilation for the Minecraft: Legacy Console Edition

## Known Bugs

There's a bug in the main game code where the depth stencil view descriptor isn't zero initialized, this happens in the file ``Windows64_Minecraft.cpp`` at the 
```D3D11_DEPTH_STENCIL_VIEW_DESC descDSView;``` line


This causes the depth stencil view creation to fail and visually breaks the game, to fix this you will need to add the following line below the ``descDSView`` definition in ``Windows64_Minecraft.cpp``:

```ZeroMemory(&descDSView, sizeof(descDSView));```

This issue only happens when building with newer versions of the Visual Studio compiler, Visual Studio 2012 isn't affected by this, so if you're working on a fork of the main source
remember to add this line to avoid breaking the game for other people

## Branches

Currently there are two branches for the project: [master](https://github.com/Patoke/4JLibs/tree/master) and [legacy-4jlibs](https://github.com/Patoke/4JLibs/tree/legacy-4jlibs)

The `master` branch is where we extend the `legacy-4jlibs` branch and implement new features or fixes that the original libraries may not have had.

The `legacy-4jlibs` branch is where we aim to achieve a 1:1 decompilation using [objdiff](https://github.com/encounter/objdiff) for matching. Once that is achieved, the branch will be left as is.

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