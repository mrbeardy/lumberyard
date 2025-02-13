/** @file engineid.h
	@brief Substance engine identifier enumeration definition.
	@author Christophe Soum - Allegorithmic (christophe.soum@allegorithmic.com)
	@note This file is part of the Substance headers
	@date 20100414
	@copyright Allegorithmic. All rights reserved.

	Used by Substance Linker to generate dedicated binaries and by engines
	version structure (see version.h). */

#ifndef _SUBSTANCE_ENGINEID_H
#define _SUBSTANCE_ENGINEID_H


/** Basic type defines */
#include <stddef.h>


/** @brief Substance engine ID enumeration */
typedef enum
{
	Substance_EngineID_INVALID      = 0x0,    /**< Unknown engine, do not use */
	Substance_EngineID_BLEND        = 0x0,    /**< BLEND platform API */

	Substance_EngineID_ogl2         = 0x1,    /**< OpenGL2 */
	Substance_EngineID_ogl3         = 0xC,    /**< OpenGL3 */
	Substance_EngineID_d3d9pc       = 0x3,    /**< Direct3D 9 Windows */
	Substance_EngineID_d3d10pc      = 0x7,    /**< Direct3D 10 Windows */
	Substance_EngineID_d3d11pc      = 0xB,    /**< Direct3D 11 Windows */

	Substance_EngineID_ogles2       = 0xA,    /**< Open GL ES 2.0 engine */
	Substance_EngineID_webgl        = 0xF,    /**< WebGL engine */

	Substance_EngineID_stdc         = 0x10,   /**< Pure Software (at least) engine */
	Substance_EngineID_sse2         = 0x13,   /**< SSE2 (minimum) CPU engine */
	Substance_EngineID_neon         = 0x1B,   /**< NEON (minimum) CPU engine */

	/* etc. */

	Substance_EngineID_FORCEDWORD   = 0xFFFFFFFF /**< Force DWORD enumeration */

} SubstanceEngineIDEnum;


#endif /* ifndef _SUBSTANCE_ENGINEID_H */
