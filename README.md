# 4JLibs (legacy-4jlibs branch)

This version of 4JLibs is focused on keeping functionality intact from the original 4JLibs, as in, no code modifications or optimizations specific to 4JLibs (apart from the profiler)

This is will be kept as an archive

## NOTICE

There's a bug in the main game code where the depth stencil view descriptor isn't zero initialized, this happens in the file ``Windows64_Minecraft.cpp`` at the 
```D3D11_DEPTH_STENCIL_VIEW_DESC descDSView;``` line


This causes the depth stencil view creation to fail and consequently breaks the game, to fix this you will need to add the following line after the definition described:

```ZeroMemory(&descDSView, sizeof(descDSView));```

This issue only happens when building with newer versions of the Visual Studio compiler, Visual Studio 2012 isn't affected by this, so if you're working on a fork of the main source
remember to add this line to avoid breaking the game for other people

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