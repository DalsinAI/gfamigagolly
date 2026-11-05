/*
 * GLSoft: the part of OpenGL 1.1 Golly's pattern view draws with, done in
 * software into a 32-bit A R G B picture: 2D only (an orthographic
 * projection and a modelview of moves and scales), rectangles made of
 * triangles, axis-aligned lines, RGBA textures sampled nearest, and the
 * one blend Golly turns on (source alpha, one minus source alpha).
 * Copyright (c) 2026 AmiCradle Team
 */
#ifndef GLSOFT_GL_H
#define GLSOFT_GL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;

#define GL_FALSE 0
#define GL_TRUE 1

/* primitives */
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007

/* types */
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_DOUBLE 0x140A

/* capabilities */
#define GL_FOG 0x0B60
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_DITHER 0x0BD0
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_TEXTURE_2D 0x0DE1
#define GL_LINE_SMOOTH 0x0B20
#define GL_MULTISAMPLE 0x809D
#define GL_VERTEX_ARRAY 0x8074
#define GL_COLOR_ARRAY 0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078

/* blending */
#define GL_ZERO 0
#define GL_ONE 1
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303

/* matrices */
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702

/* clearing */
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000

/* textures */
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_CLAMP 0x2900
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05

/* queries */
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_VIEWPORT 0x0BA2
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03

/* errors */
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_OUT_OF_MEMORY 0x0505

void glEnable(GLenum cap);
void glDisable(GLenum cap);
GLboolean glIsEnabled(GLenum cap);
void glEnableClientState(GLenum array);
void glDisableClientState(GLenum array);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glClear(GLbitfield mask);
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glLineWidth(GLfloat width);
void glPixelStorei(GLenum pname, GLint param);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);

void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glPushMatrix(void);
void glPopMatrix(void);

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);

void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                     GLenum format, GLenum type, const GLvoid *pixels);

const GLubyte *glGetString(GLenum name);
void glGetIntegerv(GLenum pname, GLint *params);
GLenum glGetError(void);
void glFlush(void);
void glFinish(void);

/* GLSoft's own: the picture drawn into (width * height pixels, each a
   32-bit 0xAARRGGBB, rows of pitch pixels). NULL draws nowhere. */
void glsoftSetTarget(unsigned int *pixels, int width, int height, int pitch);

#ifdef __cplusplus
}
#endif

#endif
