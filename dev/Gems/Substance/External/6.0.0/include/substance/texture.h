/** @file texture.h
	@brief Platform specific texture (results) structure
	@author Christophe Soum - Allegorithmic (christophe.soum@allegorithmic.com)
	@note This file is part of the Substance engine headers
	@date 20080108
	@copyright Allegorithmic. All rights reserved.

	Defines the SubstanceTexture structure. Platform dependent definition. Used
	to return resulting output textures. */

#ifndef _SUBSTANCE_TEXTURE_H
#define _SUBSTANCE_TEXTURE_H


/** Platform dependent definitions */
#include "platformdep.h"



/* Platform specific organization */
#ifdef SUBSTANCE_PLATFORM_D3D9PC
	#define SUBSTANCE_TEXTURE_D3D9 1
#endif /* ifdef SUBSTANCE_PLATFORM_D3D9PC */

#if defined(SUBSTANCE_PLATFORM_OGLES2) | \
    defined(SUBSTANCE_PLATFORM_OGL2)   | \
    defined(SUBSTANCE_PLATFORM_OGL3)
	#define SUBSTANCE_TEXTURE_OGL 1
#endif


#if defined(SUBSTANCE_TEXTURE_D3D9)

	/** @brief Substance engine texture (results) structure

	    Direct3D9 PC version. */
	typedef struct SubstanceTexture_
	{
		/** @brief Direct3D Texture handle */
		LPDIRECT3DTEXTURE9 handle;

	} SubstanceTexture;

#elif defined(SUBSTANCE_TEXTURE_OGL)

	/** @brief Substance engine texture (results) structure

	    OpenGL version. */
	typedef struct SubstanceTexture_
	{
		/** @brief OpenGL texture name */
		unsigned int textureName;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_D3D10PC)

	/** @brief Substance engine texture (results) structure

	    Direct3D10 PC version. */
	typedef struct SubstanceTexture_
	{
		/** @brief Direct3D Texture handle */
		ID3D10Texture2D* handle;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_D3D11PC)

	/** @brief Substance engine texture (results) structure

	    Direct3D11 PC/XboxOne versions. */
	typedef struct SubstanceTexture_
	{
		/** @brief Direct3D Texture handle */
		ID3D11Texture2D* handle;

	} SubstanceTexture;

#elif defined(SUBSTANCE_PLATFORM_BLEND)

	/** @brief Substance engine texture (results) structure

	    Blend platform. Result in system memory. */
	typedef struct SubstanceTexture_
	{
		/** @brief Pointer to the result array

			This value is NULL if the buffer of this output has been allocated
			by the SubstanceCallbackInPlaceOutput callback. See callbacks.h for
			further information.

			If present, the buffer contains all the mipmap levels concatenated
			without any padding. If a different memory layout is required, use
			the SubstanceCallbackInPlaceOutput callback. */
		void *buffer;

		/** @brief Width of the texture base level */
		unsigned short level0Width;

		/** @brief Height of the texture base level */
		unsigned short level0Height;

		/** @brief Pixel format of the texture
			@see pixelformat.h */
		unsigned char pixelFormat;

		/** @brief Channels order in memory

			One of the SubstanceChannelsOrder enum (defined in pixelformat.h).

			The default channel order  depends on the format and on the
			Substance engine concrete implementation (CPU/GPU, GPU API, etc.).
			See the platform specific documentation. */
		unsigned char channelsOrder;

		/** @brief Depth of the mipmap pyramid: number of levels */
		unsigned char mipmapCount;

	} SubstanceTexture;

#else

	/** @todo Add other APIs texture definition */
	#error NYI

#endif



#endif /* ifndef _SUBSTANCE_TEXTURE_H */
