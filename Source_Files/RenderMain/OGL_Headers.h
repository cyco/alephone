#ifndef _OGL_HEADERS_
#define _OGL_HEADERS_

/*

	Copyright (C) 2009 by Gregory Smith
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

	Uniform header for all Aleph One OpenGL users
*/

#ifdef __WIN32__

#define GLEW_STATIC 1
#include <GL/glew.h>

#else

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <SDL_opengl.h>

#if defined(TARGET_OS_TV)
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#include "glu.h"

#define GL_POLYGON GL_TRIANGLE_FAN
#define GL_DOUBLE GL_FLOAT
#define GLdouble GLfloat
#define glTranslated glTranslatef
#define glRotated glRotatef
#define glScaled glScalef
#define glOrtho glOrthof
#define glClipPlane glClipPlanef
#define glColor3ub(a,b,c) glColor4ub(a, b, c, 0xFF)
#define glColor4fv(_V_) glColor4f((_V_)[0], (_V_)[1], (_V_)[2], (_V_)[3])
#define glColor3fv(_V_) glColor4f((_V_)[0], (_V_)[1], (_V_)[2], 1.0)
#define glColor3us(a,b,c) glColor4f((a)/(float)0xFFFF, (b)/(float)0xFFFF, (c)/(float)0xFFFF, 0xFFFF)
#define glColor4us(a,b,c,d) glColor4f((a)/(float)0xFFFF, (b)/(float)0xFFFF, (c)/(float)0xFFFF, (d)/(float)0xFFFF)
#define glColor3usv(_V_) glColor3us((_V_)[0], (_V_)[1], (_V_)[2])
#define glColor3f(a,b,c) glColor4f(a, b,c, 1.0)
#define glColor4usv(_V_) glColor4us((_V_)[0], (_V_)[1], (_V_)[2], (_V_)[3])
#define glColor3usv(_V_) glColor3us((_V_)[0], (_V_)[1], (_V_)[2])
#define GL_DOUBLE GL_FLOAT
#define glLoadMatrixd glLoadMatrixf
#define GLdouble GLfloat
#define AGL_DOUBLEBUFFER AGL_FLOATBUFFER
#define glDepthRange glDepthRangef
#define glOrtho glOrthof
#define glGetDoublev glGetFloatv
#define glMultMatrixd glMultMatrixf
#define glTranslated glTranslatef
#define glClipPlane glClipPlanef
#define glRotated glRotatef
#define glScaled glScalef
#define GLcharARB char
#define GL_CLAMP GL_CLAMP_TO_EDGE
#define glClientActiveTextureARB glClientActiveTexture
#define glActiveTextureARB glActiveTexture
#define GL_TEXTURE0_ARB GL_TEXTURE0
#define GL_TEXTURE1_ARB GL_TEXTURE1
#define glMultiTexCoord4fARB glMultiTexCoord4f
#define GL_TEXTURE_RECTANGLE_ARB GL_TEXTURE_2D
#define GL_QUADS GL_TRIANGLE_FAN
#define glUniform1iARB glUniform1i
#define glUniform1fARB glUniform1f
#define glUniformMatrix4fvARB glUniformMatrix4fv
#define GLhandleARB GLuint
#define GLcharARB char
#define GL_VERTEX_SHADER_ARB GL_VERTEX_SHADER
#define GL_FRAGMENT_SHADER_ARB GL_FRAGMENT_SHADER
#define glGenFramebuffersEXT glGenFramebuffers
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
#define GL_RENDERBUFFER_EXT GL_RENDERBUFFER
#define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
#define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0
#define glBindRenderbufferEXT glBindRenderbuffer
#define glBindFramebufferEXT glBindFramebuffer
#define glDeleteRenderbuffersEXT glDeleteRenderbuffers
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#define glDeleteFramebuffersExt glDeleteFramebuffers
#define GL_DEPTH_ATTACHMENT_EXT GL_DEPTH_ATTACHMENT
#define glRenderbufferStorageEXT glRenderbufferStorage
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glCheckFramebufferStatusEXT glCheckFramebufferStatus
#define glGenRenderbuffersEXT glGenRenderbuffers
#define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
#define glFramebufferTexture2DEXT glFramebufferTexture2D
#define glPopAttrib() do {} while(0)
#define glPushAttrib(A) do {} while(0)
#define glGetUniformLocationARB glGetUniformLocation
#define glCreateShaderObjectARB glCreateShader
#define glShaderSourceARB glShaderSource
#define glCompileShaderARB glCompileShader
#define glGetObjectParameterivARB glGetShaderiv
#define glDeleteObjectARB glDeleteShader
#define GL_OBJECT_COMPILE_STATUS_ARB GL_COMPILE_STATUS
#define glLinkProgramARB glLinkProgram
#define glUseProgramObjectARB glUseProgram
#define glAttachObjectARB glAttachShader
#define glCreateProgramObjectARB glCreateProgram
#define glFrustum glFrustumf

#elif defined (__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#endif

#endif
