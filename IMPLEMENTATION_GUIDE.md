# 4JLibs implementation guide

Whilst the library can be a direct replacement to the standard libraries shipped by the source leak, we can apply some enhancements to the original code

Some functions were misimplemented due to the Windows library being a direct port off the Xbox 360 libraries, and because this platform was never supposed to see the light of day

This guide will explain every change you can do in the source leak to improve it, like displaying save thumbnails, displaying save titles on the load menu and more

## Save thumbnails

#### The original code for displaying and saving thumbnails was broken for the Windows platform, either by poor ports or lack of care, in this section it will be explained how to enable this feature for the Windows platform using 4JLibs

In the files ``ConsoleSaveFileOriginal.cpp`` and ``ConsoleSaveFileSplit.cpp`` (the latter might not exist if you are using the December build) there was a mistake when calling ``app.GetSaveThumbnail``

To fix this issue, you need to replace the line of code:

```cpp
#if ( defined _XBOX || defined _DURANGO )
```

For:
```cpp
#if ( defined _XBOX || defined _DURANGO || defined _WINDOWS64 )
```

In the files ``Windows64_App.h`` and ``Windows64_App.cpp``, it is assumed that thumbnail capturing is broken, therefore the functions responsible for doing the capturing are empty, we need to fill them back up as follows:

Below the implementation of:
```cpp
class CConsoleMinecraftApp : public CMinecraftApp
{
public:
```

You need to add the line:
```cpp
ImageFileBuffer m_ThumbnailBuffer;
```

Then, in ``Windows64_App.cpp``, you need to replace the empty implementations of ``CConsoleMinecraftApp::CaptureSaveThumbnail()`` and ``CConsoleMinecraftApp::GetSaveThumbnail`` with the following:
```cpp
void CConsoleMinecraftApp::CaptureSaveThumbnail()
{
	RenderManager.CaptureThumbnail(&m_ThumbnailBuffer);
}
void CConsoleMinecraftApp::GetSaveThumbnail(PBYTE *pbData,DWORD *pdwSize)
{
	if (m_ThumbnailBuffer.Allocated())
	{
		if (pbData)
		{
			*pbData = new BYTE[m_ThumbnailBuffer.GetBufferSize()];
			*pdwSize = m_ThumbnailBuffer.GetBufferSize();
			memcpy(*pbData, m_ThumbnailBuffer.GetBufferPointer(), *pdwSize);
		}
		m_ThumbnailBuffer.Release();
	}
}
```

These changes are enough to display the save thumbnails in the save selector, but we also need to fix the save data displayed in the actual load menu, which we will do as follows

In the file ``UIScene_LoadMenu.cpp``, you need to replace the line:
```cpp
#if defined(__PS3__) || defined(__ORBIS__)|| defined(_DURANGO) || defined (__PSVITA__)
```

With:
```cpp
#if defined(__PS3__) || defined(__ORBIS__)|| defined(_DURANGO) || defined (__PSVITA__) || defined (_WINDOWS64)
```

Then, below that, there's the following chunk of code:
```cpp
#else // __ORBIS__
		{
			SceCesUcsContext Context;
			sceCesUcsContextInit( &Context );
			uint32_t utf8Len, utf16Len;
			sceCesUtf8StrToUtf16Str(&Context, (uint8_t *)params->saveDetails->UTF8SaveFilename, srclen, &utf8Len, u16Message, dstlen, &utf16Len);
		}
#endif
```

You want to modify the ``#else`` conditional to be:
```cpp
#elif defined(__ORBIS__) || defined(__PSVITA__)
```

This way the Windows platform doesn't try accessing PS4/PSVita functions

Then you want to comment/remove the following chunk of code:
```cpp
		if(params->saveDetails->pbThumbnailData)
		{
			m_pbThumbnailData = params->saveDetails->pbThumbnailData;
			m_uiThumbnailSize = params->saveDetails->dwThumbnailSize;
			m_bSaveThumbnailReady = true;
		}
		else
```

You can replace it for
```cpp
		//if(params->saveDetails->pbThumbnailData)
		//{
		//	m_pbThumbnailData = params->saveDetails->pbThumbnailData;
		//	m_uiThumbnailSize = params->saveDetails->dwThumbnailSize;
		//	m_bSaveThumbnailReady = true;
		//}
		//else
```

This is because, for a ``SAVE_DETAILS`` object, the thumbnail data is always pointing to valid memory (aka, it's always allocated)


After all these changes, save titles and thumbnails should be properly displayed across the game

## Renderer bugs/artifacts on newer Visual Studio versions

In the file ``Windows64_Minecraft.cpp``, the depth stencil view descriptor is not null terminated which causes undefined behaviour

In the case of newer compilers like Visual Studio 2022/2026, this causes the ``descDSView.Flags`` field to be filled with garbage data, and therefore causing visual glitches in game because the depth stencil fails to be created

To fix this issue, below the line:
```cpp
D3D11_DEPTH_STENCIL_VIEW_DESC descDSView;
```

You need to fill the structure with zeros, you can do it like this:
```cpp
ZeroMemory(&descDSView, sizeof(descDSView));
```