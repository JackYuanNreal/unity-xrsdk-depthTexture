#include "RenderAPI.h"
#include "PlatformBase.h"
#include <string>
#include "XR/IUnityXRTrace.h"
#include "ProviderContext.h"

// OpenGL Core profile (desktop) or OpenGL ES (mobile) implementation of RenderAPI.
// Supports several flavors: Core, ES2, ES3


#if SUPPORT_OPENGL_UNIFIED


#include <assert.h>
#if UNITY_IOS || UNITY_TVOS
#	include <OpenGLES/ES2/gl.h>
#elif UNITY_ANDROID || UNITY_WEBGL
#	include <GLES2/gl2.h>
#elif UNITY_OSX
#	include <OpenGL/gl3.h>
#elif UNITY_WIN
// On Windows, use gl3w to initialize and load OpenGL Core functions. In principle any other
// library (like GLEW, GLFW etc.) can be used; here we use gl3w since it's simple and
// straightforward.
#	include "gl3w/gl3w.h"
#elif UNITY_LINUX
#	define GL_GLEXT_PROTOTYPES
#	include <GL/gl.h>
#elif UNITY_EMBEDDED_LINUX
#	include <GLES2/gl2.h>
#if SUPPORT_OPENGL_CORE
#	define GL_GLEXT_PROTOTYPES
#	include <GL/gl.h>
#endif
#else
#	error Unknown platform
#endif

extern ProviderContext *s_Context;
extern void* g_TextureHandle;

class RenderAPI_OpenGLCoreES : public RenderAPI
{
public:
	RenderAPI_OpenGLCoreES(UnityGfxRenderer apiType);
	virtual ~RenderAPI_OpenGLCoreES() { }

	virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);

	virtual bool GetUsesReverseZ() { return false; }

	virtual void DrawSimpleTriangles(const float worldMatrix[16], const float projectionMatrix[16], int triangleCount, const void* verticesFloat3Byte4, unsigned int texId, bool doDepthTestWrite);

	virtual void* BeginModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int* outRowPitch);
	virtual void EndModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, void* dataPtr);

	virtual void* BeginModifyVertexBuffer(void* bufferHandle, size_t* outBufferSize);
	virtual void EndModifyVertexBuffer(void* bufferHandle);

private:
	void CreateResources();
	bool CheckForGLError(const std::string &message);
	GLuint CreateShader(GLenum type, const char* sourceText);

private:
	UnityGfxRenderer m_APIType;
	GLuint m_VertexShader;
	GLuint m_FragmentShader;
	GLuint m_Program;
	GLuint m_VertexArray;
	GLuint m_VertexBuffer;
	int m_UniformWorldMatrix;
	int m_UniformProjMatrix;
	int m_UniformSampler;
};


RenderAPI* CreateRenderAPI_OpenGLCoreES(UnityGfxRenderer apiType)
{
	return new RenderAPI_OpenGLCoreES(apiType);
}


enum VertexInputs
{
	kVertexInputPosition = 0,
	kVertexInputColor = 1,
	kVertexInputUV = 2
};


// Simple vertex shader source
#define VERTEX_SHADER_SRC(ver, attr, varying)						\
	ver																\
	attr " highp vec3 pos;\n"										\
	attr " lowp vec4 color;\n"										\
	attr " lowp vec2 texCoord;\n"										\
	"\n"															\
	varying " lowp vec4 ocolor;\n"									\
	varying " lowp vec2 oTexCoord;\n"									\
	"\n"															\
	"uniform highp mat4 worldMatrix;\n"								\
	"uniform highp mat4 projMatrix;\n"								\
	"\n"															\
	"void main()\n"													\
	"{\n"															\
	"	gl_Position = projMatrix * (worldMatrix * vec4(pos,1));\n"	\
	"	ocolor = color;\n"											\
	"	oTexCoord = texCoord;\n"									\
	"	oTexCoord.x = 1.0f - oTexCoord.x;\n"						\
	"}\n"															\

static const char* kGlesVProgTextGLES2 = VERTEX_SHADER_SRC("\n", "attribute", "varying");
static const char* kGlesVProgTextGLES3 = VERTEX_SHADER_SRC("#version 300 es\n", "in", "out");
#if SUPPORT_OPENGL_CORE
static const char* kGlesVProgTextGLCore = VERTEX_SHADER_SRC("#version 150\n", "in", "out");
#endif

#undef VERTEX_SHADER_SRC

// Simple fragment shader source
#define FRAGMENT_SHADER_SRC(ver, varying, outDecl, outVar)	\
	ver												\
	outDecl											\
	varying " vec4 ocolor;\n"					\
	varying " vec2 oTexCoord;\n"					\
	"\n"											\
    "uniform sampler2D textureSampler;\n"			\
	"void main()\n"									\
	"{\n"											\
	"	vec4 tex = texture(textureSampler, oTexCoord);\n"		\
    "   float depth = tex.r;\n"		\
	"	depth = 1.0f - depth;\n"					\
    "	depth = depth * depth * depth * depth;\n"	\
	"	" outVar " = vec4(depth, depth, depth, 1.0);\n"						\
	"	" outVar " = tex;\n"						\
	"}\n"											\

static const char* kGlesFShaderTextGLES2 = FRAGMENT_SHADER_SRC("\n", "varying", "\n", "gl_FragColor");
static const char* kGlesFShaderTextGLES3 = FRAGMENT_SHADER_SRC("#version 300 es\n", "in", "out lowp vec4 fragColor;\n", "fragColor");
#if SUPPORT_OPENGL_CORE
static const char* kGlesFShaderTextGLCore = FRAGMENT_SHADER_SRC("#version 150\n", "in", "out lowp vec4 fragColor;\n", "fragColor");
#endif

#undef FRAGMENT_SHADER_SRC



extern std::string str_GL_NO_ERROR;
extern std::string str_GL_INVALID_ENUM;
extern std::string str_GL_INVALID_VALUE;
extern std::string str_GL_INVALID_OPERATION;
extern std::string str_GL_INVALID_FRAMEBUFFER_OPERATION;
extern std::string str_GL_OUT_OF_MEMORY;
extern std::string str_GL_UNKNOWN_ERROR;

GLuint RenderAPI_OpenGLCoreES::CreateShader(GLenum type, const char* sourceText)
{
	GLuint ret = glCreateShader(type);
	glShaderSource(ret, 1, &sourceText, NULL);
	if (CheckForGLError("CreateShader glShaderSource"))
	{
        XR_TRACE_LOG(s_Context->trace, "Shader:\n %s", sourceText);
		return ret;
	}
	glCompileShader(ret);
	if (CheckForGLError("CreateShader glCompileShader"))
	{
        XR_TRACE_LOG(s_Context->trace, "Shader:\n %s", sourceText);
		return ret;
	}

	return ret;
}


bool RenderAPI_OpenGLCoreES::CheckForGLError(const std::string &message)
{
    GLenum err;
    bool retcode = false;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::string error_string;
        switch (err)
        {
        case GL_NO_ERROR:
            error_string = str_GL_NO_ERROR;
            break;
        case GL_INVALID_ENUM:
            error_string = str_GL_INVALID_ENUM;
            break;
        case GL_INVALID_VALUE:
            error_string = str_GL_INVALID_VALUE;
            break;
        case GL_INVALID_OPERATION:
            error_string = str_GL_INVALID_OPERATION;
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error_string = str_GL_INVALID_FRAMEBUFFER_OPERATION;
            break;
        case GL_OUT_OF_MEMORY:
            error_string = str_GL_OUT_OF_MEMORY;
            break;
        default:
            error_string = str_GL_UNKNOWN_ERROR;
            break;
        }

        XR_TRACE_LOG(s_Context->trace, "%s OpenGL error: %s(%d)", message.c_str(), error_string.c_str(), (int)err);
        retcode = true;
    }
    return retcode;
}

void RenderAPI_OpenGLCoreES::CreateResources()
{
#	if UNITY_WIN && SUPPORT_OPENGL_CORE
	if (m_APIType == kUnityGfxRendererOpenGLCore)
		gl3wInit();
#	endif
	// Make sure that there are no GL error flags set before creating resources
	while (glGetError() != GL_NO_ERROR) {}

	// Create shaders
	if (m_APIType == kUnityGfxRendererOpenGLES20)
	{
		m_VertexShader = CreateShader(GL_VERTEX_SHADER, kGlesVProgTextGLES2);
        XR_TRACE_LOG(s_Context->trace, "VertexShader 2:\n %s", kGlesVProgTextGLES2);
		m_FragmentShader = CreateShader(GL_FRAGMENT_SHADER, kGlesFShaderTextGLES2);
        XR_TRACE_LOG(s_Context->trace, "FragmentShader 2:\n %s", kGlesFShaderTextGLES2);
	}
	else if (m_APIType == kUnityGfxRendererOpenGLES30)
	{
		m_VertexShader = CreateShader(GL_VERTEX_SHADER, kGlesVProgTextGLES3);
        XR_TRACE_LOG(s_Context->trace, "VertexShader:\n %s", kGlesVProgTextGLES3);
		m_FragmentShader = CreateShader(GL_FRAGMENT_SHADER, kGlesFShaderTextGLES3);
        XR_TRACE_LOG(s_Context->trace, "FragmentShader:\n %s", kGlesFShaderTextGLES3);
	}
#	if SUPPORT_OPENGL_CORE
	else if (m_APIType == kUnityGfxRendererOpenGLCore)
	{
		m_VertexShader = CreateShader(GL_VERTEX_SHADER, kGlesVProgTextGLCore);
		m_FragmentShader = CreateShader(GL_FRAGMENT_SHADER, kGlesFShaderTextGLCore);
	}
#	endif // if SUPPORT_OPENGL_CORE


	// Link shaders into a program and find uniform locations
	m_Program = glCreateProgram();
	glBindAttribLocation(m_Program, kVertexInputPosition, "pos");
	glBindAttribLocation(m_Program, kVertexInputColor, "color");
	glBindAttribLocation(m_Program, kVertexInputUV, "texCoord");
	CheckForGLError("glBindAttribLocation");

	glAttachShader(m_Program, m_VertexShader);
	glAttachShader(m_Program, m_FragmentShader);
	CheckForGLError("glAttachShader");

#	if SUPPORT_OPENGL_CORE
	if (m_APIType == kUnityGfxRendererOpenGLCore)
		glBindFragDataLocation(m_Program, 0, "fragColor");
#	endif // if SUPPORT_OPENGL_CORE
	glLinkProgram(m_Program);

	GLint status = 0;
	glGetProgramiv(m_Program, GL_LINK_STATUS, &status);
	assert(status == GL_TRUE);

	m_UniformWorldMatrix = glGetUniformLocation(m_Program, "worldMatrix");
	m_UniformProjMatrix = glGetUniformLocation(m_Program, "projMatrix");
	m_UniformSampler = glGetUniformLocation(m_Program, "textureSampler");

	// Create vertex buffer
	glGenBuffers(1, &m_VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 1024, NULL, GL_STREAM_DRAW);

	assert(glGetError() == GL_NO_ERROR);
}


RenderAPI_OpenGLCoreES::RenderAPI_OpenGLCoreES(UnityGfxRenderer apiType)
	: m_APIType(apiType)
{
}


void RenderAPI_OpenGLCoreES::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
	if (type == kUnityGfxDeviceEventInitialize)
	{
		CreateResources();
	}
	else if (type == kUnityGfxDeviceEventShutdown)
	{
		//@TODO: release resources
	}
}


void RenderAPI_OpenGLCoreES::DrawSimpleTriangles(const float worldMatrix[16], const float projectionMatrix[16], int triangleCount, const void* verticesFloat3Byte4, unsigned int texId, bool doDepthTestWrite)
{
	// Set basic render state
	glColorMask(true, true, true, true);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	if (doDepthTestWrite)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	// Setup shader program to use, and the matrices
	glUseProgram(m_Program);
	glUniformMatrix4fv(m_UniformWorldMatrix, 1, GL_FALSE, worldMatrix);
	glUniformMatrix4fv(m_UniformProjMatrix, 1, GL_FALSE, projectionMatrix);
	CheckForGLError("glUniformMatrix4fv");

	// Core profile needs VAOs, setup one
#	if SUPPORT_OPENGL_CORE
	if (m_APIType == kUnityGfxRendererOpenGLCore)
	{
		glGenVertexArrays(1, &m_VertexArray);
		glBindVertexArray(m_VertexArray);
	}
#	endif // if SUPPORT_OPENGL_CORE


	glActiveTexture(GL_TEXTURE0);
	GLuint glTex = (GLuint)(size_t)(texId);
	XR_TRACE_LOG(s_Context->trace, "DrawSimpleTriangles: glTex=%u\n", glTex);

	glBindTexture(GL_TEXTURE_2D, glTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glUniform1i(m_UniformSampler, 0);
	CheckForGLError("glUniform1i");


	// Bind a vertex buffer, and update data in it
	const int kVertexSize = 12 + 4 + 8;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, kVertexSize * triangleCount * 3, verticesFloat3Byte4);

	// Setup vertex layout
	glEnableVertexAttribArray(kVertexInputPosition);
	glVertexAttribPointer(kVertexInputPosition, 3, GL_FLOAT, GL_FALSE, kVertexSize, (char*)NULL + 0);
	glEnableVertexAttribArray(kVertexInputColor);
	glVertexAttribPointer(kVertexInputColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, kVertexSize, (char*)NULL + 12);
	glEnableVertexAttribArray(kVertexInputUV);
	glVertexAttribPointer(kVertexInputUV, 2, GL_FLOAT, GL_FALSE, kVertexSize, (char*)NULL + 12 + 4);

	CheckForGLError("glVertexAttribPointer");

	// Draw
	glDrawArrays(GL_TRIANGLES, 0, triangleCount * 3);
	CheckForGLError("glDrawArrays");

	// Cleanup VAO
#	if SUPPORT_OPENGL_CORE
	if (m_APIType == kUnityGfxRendererOpenGLCore)
	{
		glDeleteVertexArrays(1, &m_VertexArray);
	}
#	endif
}


void* RenderAPI_OpenGLCoreES::BeginModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int* outRowPitch)
{
	const int rowPitch = textureWidth * 4;
	// Just allocate a system memory buffer here for simplicity
	unsigned char* data = new unsigned char[rowPitch * textureHeight];
	*outRowPitch = rowPitch;
	return data;
}


void RenderAPI_OpenGLCoreES::EndModifyTexture(void* textureHandle, int textureWidth, int textureHeight, int rowPitch, void* dataPtr)
{
	GLuint gltex = (GLuint)(size_t)(textureHandle);
	// Update texture data, and free the memory buffer
	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_RGBA, GL_UNSIGNED_BYTE, dataPtr);
	delete[](unsigned char*)dataPtr;
}

void* RenderAPI_OpenGLCoreES::BeginModifyVertexBuffer(void* bufferHandle, size_t* outBufferSize)
{
#	if SUPPORT_OPENGL_ES
	return 0;
#	else
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)(size_t)bufferHandle);
	GLint size = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	*outBufferSize = size;
	void* mapped = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	return mapped;
#	endif
}


void RenderAPI_OpenGLCoreES::EndModifyVertexBuffer(void* bufferHandle)
{
#	if !SUPPORT_OPENGL_ES
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint)(size_t)bufferHandle);
	glUnmapBuffer(GL_ARRAY_BUFFER);
#	endif
}

#endif // #if SUPPORT_OPENGL_UNIFIED
