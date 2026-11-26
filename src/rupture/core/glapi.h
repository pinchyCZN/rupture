/*
 * The GL entry points the core uses, the same names on both hosts.
 * The browser has them from <GLES3/gl3.h>. On Windows opengl32.dll
 * exports only GL 1.1, so everything newer is a function pointer of
 * the GL name, filled by GlApi_Load once a 3.3 core context is
 * current. Constants GL 1.1's header lacks are defined here.
 */
#ifndef RUPTURE_GLAPI_H
#define RUPTURE_GLAPI_H

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <windows.h>
#include <GL/gl.h>
#include <stddef.h>

typedef char      GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef unsigned __int64 GLuint64;

#define GL_PROGRAM_POINT_SIZE        0x8642
#define GL_ARRAY_BUFFER              0x8892
#define GL_ELEMENT_ARRAY_BUFFER      0x8893
#define GL_STATIC_DRAW               0x88e4
#define GL_DYNAMIC_DRAW              0x88e8
#define GL_STREAM_DRAW               0x88e0
#define GL_MAP_WRITE_BIT             0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT  0x0004
#define GL_MAP_UNSYNCHRONIZED_BIT    0x0020
#define GL_TIME_ELAPSED              0x88bf
#define GL_QUERY_RESULT              0x8866
#define GL_FRAGMENT_SHADER           0x8b30
#define GL_VERTEX_SHADER             0x8b31
#define GL_COMPILE_STATUS            0x8b81
#define GL_LINK_STATUS               0x8b82
#define GL_INFO_LOG_LENGTH           0x8b84
#define GL_TEXTURE0                  0x84c0
#define GL_CLAMP_TO_EDGE             0x812f
#define GL_FRAMEBUFFER               0x8d40
#define GL_READ_FRAMEBUFFER          0x8ca8
#define GL_DRAW_FRAMEBUFFER          0x8ca9
#define GL_RENDERBUFFER              0x8d41
#define GL_DEPTH_COMPONENT24         0x81a6
#define GL_COLOR_ATTACHMENT0         0x8ce0
#define GL_DEPTH_ATTACHMENT          0x8d00
#define GL_FRAMEBUFFER_COMPLETE      0x8cd5
#define GL_DEPTH_COMPONENT32         0x81a7
#define GL_R8                        0x8229
#define GL_MAX_SAMPLES               0x8d57
#define GL_TEXTURE_CUBE_MAP          0x8513
#define GL_TEXTURE_SWIZZLE_G         0x8e43
#define GL_TEXTURE_SWIZZLE_B         0x8e44

#define GLAPI_FUNCS(X) \
    X(void,   glGenBuffers, (GLsizei n, GLuint *buffers)) \
    X(void,   glDeleteBuffers, (GLsizei n, const GLuint *buffers)) \
    X(void,   glBindBuffer, (GLenum target, GLuint buffer)) \
    X(void,   glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage)) \
    X(void,   glBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const void *data)) \
    X(void *, glMapBufferRange, (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)) \
    X(GLboolean, glUnmapBuffer, (GLenum target)) \
    X(void,   glGenVertexArrays, (GLsizei n, GLuint *arrays)) \
    X(void,   glBindVertexArray, (GLuint array)) \
    X(void,   glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, \
                                      GLsizei stride, const void *pointer)) \
    X(void,   glEnableVertexAttribArray, (GLuint index)) \
    X(void,   glDisableVertexAttribArray, (GLuint index)) \
    X(void,   glVertexAttrib4f, (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
    X(GLuint, glCreateShader, (GLenum type)) \
    X(void,   glDeleteShader, (GLuint shader)) \
    X(void,   glShaderSource, (GLuint shader, GLsizei count, const GLchar *const *string, \
                               const GLint *length)) \
    X(void,   glCompileShader, (GLuint shader)) \
    X(void,   glGetShaderiv, (GLuint shader, GLenum pname, GLint *params)) \
    X(void,   glGetShaderInfoLog, (GLuint shader, GLsizei size, GLsizei *length, GLchar *log)) \
    X(GLuint, glCreateProgram, (void)) \
    X(void,   glAttachShader, (GLuint program, GLuint shader)) \
    X(void,   glBindAttribLocation, (GLuint program, GLuint index, const GLchar *name)) \
    X(void,   glLinkProgram, (GLuint program)) \
    X(void,   glGetProgramiv, (GLuint program, GLenum pname, GLint *params)) \
    X(void,   glGetProgramInfoLog, (GLuint program, GLsizei size, GLsizei *length, GLchar *log)) \
    X(void,   glUseProgram, (GLuint program)) \
    X(GLint,  glGetUniformLocation, (GLuint program, const GLchar *name)) \
    X(void,   glUniform1i, (GLint location, GLint v0)) \
    X(void,   glUniform1f, (GLint location, GLfloat v0)) \
    X(void,   glUniform3f, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2)) \
    X(void,   glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, \
                                   const GLfloat *value)) \
    X(void,   glActiveTexture, (GLenum texture)) \
    X(void,   glGenFramebuffers, (GLsizei n, GLuint *framebuffers)) \
    X(void,   glBindFramebuffer, (GLenum target, GLuint framebuffer)) \
    X(void,   glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, \
                                       GLuint texture, GLint level)) \
    X(void,   glFramebufferRenderbuffer, (GLenum target, GLenum attachment, \
                                          GLenum renderbuffertarget, GLuint renderbuffer)) \
    X(GLenum, glCheckFramebufferStatus, (GLenum target)) \
    X(void,   glGenRenderbuffers, (GLsizei n, GLuint *renderbuffers)) \
    X(void,   glBindRenderbuffer, (GLenum target, GLuint renderbuffer)) \
    X(void,   glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, \
                                      GLsizei height)) \
    X(void,   glRenderbufferStorageMultisample, (GLenum target, GLsizei samples, \
                                                 GLenum internalformat, GLsizei width, \
                                                 GLsizei height)) \
    X(void,   glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, \
                                  GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, \
                                  GLenum filter)) \
    X(void,   glDrawBuffers, (GLsizei n, const GLenum *bufs)) \
    X(void,   glGenQueries, (GLsizei n, GLuint *ids)) \
    X(void,   glBeginQuery, (GLenum target, GLuint id)) \
    X(void,   glEndQuery, (GLenum target)) \
    X(void,   glGetQueryObjectui64v, (GLuint id, GLenum pname, GLuint64 *params))

#define GLAPI_DECLARE(ret, name, params) \
    typedef ret (APIENTRY *PFN_##name) params; \
    extern PFN_##name name;
GLAPI_FUNCS(GLAPI_DECLARE)
#undef GLAPI_DECLARE

#endif

/* Fills the pointers on Windows; true when every one was found. */
bool GlApi_Load(void);

#endif
