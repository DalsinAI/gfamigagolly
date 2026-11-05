/*
 * GLSoft's contexts, for the wxGLCanvas and wxGLContext of glcanvas.cpp.
 * Copyright (c) 2026 AmiCradle Team
 */
#ifndef GLSOFT_PRIVATE_H
#define GLSOFT_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct glsoft_ctx;
struct glsoft_ctx *glsoftCreateContext(void);
void glsoftDestroyContext(struct glsoft_ctx *c);
void glsoftMakeCurrent(struct glsoft_ctx *c);

#ifdef __cplusplus
}
#endif

#endif
