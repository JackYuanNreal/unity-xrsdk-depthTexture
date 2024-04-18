#include <cmath>
#include <vector>
#include <GLES/gl.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <sstream>
#include "ProviderContext.h"
#include "XR/UnityXRDisplayStats.h"
#include "XR/IUnityXRDisplay.h"
#include "XR/IUnityXRTrace.h"
#include "TextureManager.h"


extern std::string str_GL_NO_ERROR;
extern std::string str_GL_INVALID_ENUM;
extern std::string str_GL_INVALID_VALUE;
extern std::string str_GL_INVALID_OPERATION;
extern std::string str_GL_INVALID_FRAMEBUFFER_OPERATION;
extern std::string str_GL_OUT_OF_MEMORY;
extern std::string str_GL_UNKNOWN_ERROR;
extern float g_Time;
extern void* g_TextureHandle;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#include "D3D11.h"
#include "IUnityGraphicsD3D11.h"
#define XR_DX11 1
#else
#define XR_DX11 0
#endif

#define NUM_RENDER_PASSES 2

// BEGIN WORKAROUND: skip first frame since we get invalid data.  Fix coming to trunk.
static bool s_SkipFrame = true;
const int ValueSetFPS = 60;
static UnityXRFocusPlane *unityFocusPlane = new UnityXRFocusPlane();
#define WORKAROUND_SKIP_FIRST_FRAME()           \
    if (s_SkipFrame)                            \
    {                                           \
        s_SkipFrame = false;                    \
        return kUnitySubsystemErrorCodeSuccess; \
    }
#define WORKAROUND_RESET_SKIP_FIRST_FRAME() s_SkipFrame = true;
// END WORKAROUND

#define TRANSFORM_LOG(_pt_, _tranPt_)                   \
    SUBSYSTEM_LOG(m_Ctx.trace, "DrawColoredTriangle tranPt: %f, %f, %f, %f --> %f, %f, %f, %f\n", _pt_[0], _pt_[1], _pt_[2], _pt_[3], _tranPt_[0], _tranPt_[1], _tranPt_[2], _tranPt_[3]);

#define CREATE_TEXTRUE_GL(__texture_target__, __internal_format__, __format__, __type__)                                                  \
    glGenTextures(1, &nativeTex);                                                                                                         \
    glBindTexture(__texture_target__, nativeTex);                                                                                         \
    glTexParameteri(__texture_target__, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                                                \
    glTexParameteri(__texture_target__, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                                                \
    glTexParameteri(__texture_target__, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);                                                             \
    glTexParameteri(__texture_target__, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);                                                             \
    if (__texture_target__ == GL_TEXTURE_2D_ARRAY)                                                                                        \
        glTexImage3D(__texture_target__, 0, __internal_format__, texWidth, texHeight, texArrayLength, 0, __format__, __type__, NULL); \
    else                                                                                                                                  \
        glTexImage2D(__texture_target__, 0, __internal_format__, texWidth, texHeight, 0, __format__, __type__, NULL);                     \
    glBindTexture(__texture_target__, 0);

bool TextureManager::CheckForGLError(const std::string &message)
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

        XR_TRACE_LOG(m_Ctx.trace, "%s OpenGL error: %s(%d)", message.c_str(), error_string.c_str(), (int)err);
        retcode = true;
    }
    return retcode;
}

bool TextureManager::AcquireTexture()
{
    m_texture_index = (m_texture_index + 1) % m_UnityTextures.size();
    SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] AcquireTexture : idTex=%d, texIdx=%d", m_UnityTextures[m_texture_index], m_texture_index);
    return true;
}

UnityXRRenderTextureId TextureManager::GetCurTexture()
{
    if (m_texture_index < 0 || m_texture_index >= (int)m_NativeTextures.size())
        return 0;

    return m_UnityTextures[m_texture_index];
}

bool TextureManager::CreateColorTexture(uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, uint32_t &nativeTexOut, bool sRGB)
{
    GLuint nativeTex = 0;
    if (texArrayLength >= 2)
    {
        if (sRGB)
        {
            CREATE_TEXTRUE_GL(GL_TEXTURE_2D_ARRAY, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
        }
        else
        {
            CREATE_TEXTRUE_GL(GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
        }
    }
    else
    {
        if (sRGB)
        {
            CREATE_TEXTRUE_GL(GL_TEXTURE_2D, GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
        }
        else
        {
            CREATE_TEXTRUE_GL(GL_TEXTURE_2D, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
        }
    }
    SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CreateColorTexture: %u", nativeTex);
    nativeTexOut = (uint32_t)nativeTex;
    return !CheckForGLError("create color texture");
}

bool TextureManager::CreateDepthTexture(uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, uint32_t &nativeTexOut)
{
    GLuint nativeTex = 0;
    if (texArrayLength >= 2)
    {
        CREATE_TEXTRUE_GL(GL_TEXTURE_2D_ARRAY, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
    }
    else
    {
        // CREATE_TEXTRUE_GL(GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
        // CREATE_TEXTRUE_GL(GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
        CREATE_TEXTRUE_GL(GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
    }
    SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CreateDepthTexture: %d", (int)nativeTex);
    nativeTexOut = (uint32_t)nativeTex;
    return !CheckForGLError("create depth texture");
}

void TextureManager::DestroyNativeTexture(uint32_t nativeTex)
{
    if (nativeTex != 0)
    {
        // SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] DestroyNativeTexture: %u", nativeTex);
        GLuint texture = static_cast<GLuint>(nativeTex);
        glDeleteTextures(1, &texture);
    }
}

bool TextureManager::CreateNativeTextures(int texNum, uint32_t texWidth, uint32_t texHeight, uint32_t texArrLength, bool requestDepthTex, std::vector<uint32_t> &nativeTextures, bool sRGB)
{
    nativeTextures.clear();
    uint32_t nativeTex = 0;
    bool result = false;
    for (int i = 0; i < texNum; ++i)
    {
        nativeTex = 0;
        if (requestDepthTex)
            result = CreateDepthTexture(texWidth, texHeight, texArrLength, nativeTex);
        else
            result = CreateColorTexture(texWidth, texHeight, texArrLength, nativeTex, sRGB);
        if (!result)
        {
            SUBSYSTEM_ERROR(m_Ctx.trace, "[TextureManager] Failed to create texture: %llu", (uint64_t)nativeTex);
            return false;
        }
        SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CreateNativeTextures [%d]: idTex=%d", i, (int)nativeTex);
        nativeTextures.push_back(nativeTex);
    }
    return true;
}

bool TextureManager::CreateUnityTextures(std::vector<uint32_t> &unityTextures, bool requestDepthTex, bool sRGB)
{
    int texNum = m_NativeTextures.size();

    // Tell unity about the native textures, getting back UnityXRRenderTextureIds.
    unityTextures.resize(texNum);
    SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CreateUnityTextures start: requestDepthTex=%d", requestDepthTex);
    for (int i = 0; i < texNum; ++i)
    {
        UnityXRRenderTextureDesc uDesc;
        uDesc.colorFormat = kUnityXRRenderTextureFormatRGBA32;
        uDesc.color.nativePtr = (void*)(uint64_t)m_NativeTextures[i];
        if (requestDepthTex)
        {
            UnityXRRenderTextureDesc depthDesc;
            depthDesc.colorFormat = kUnityXRRenderTextureFormatNone;
            depthDesc.depthFormat = kUnityXRDepthTextureFormat16bit;
            depthDesc.depth.nativePtr = (void*)(uint64_t)m_NativeDepthTextures[i];
            depthDesc.width = m_texWidth;
            depthDesc.height = m_texHeight;
            depthDesc.flags = 0;
            depthDesc.textureArrayLength = 0;

            UnityXRRenderTextureId depthId;
            UnitySubsystemErrorCode retDepth = m_Ctx.display->CreateTexture(m_Handle, &depthDesc, &depthId);
            if (retDepth != kUnitySubsystemErrorCodeSuccess)
            {
                SUBSYSTEM_ERROR(m_Ctx.trace, "[TextureManager] Failed to create unity texture for depth texture: %d", (int)retDepth);
            }

            SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CreateUnityTextures for depth: [%d], depthIdTex=%d, nativePtr=%llu", i, (int)depthId, (uint64_t)m_NativeDepthTextures[i]);

            uDesc.depthFormat = kUnityXRDepthTextureFormatReference;
            uDesc.depth.referenceTextureId = depthId;
        }
        else
        {
            uDesc.depthFormat = kUnityXRDepthTextureFormatNone;
            uDesc.depth.nativePtr = (void *)kUnityXRRenderTextureIdDontCare;
        }
        uDesc.width = m_texWidth;
        uDesc.height = m_texHeight;
        uDesc.textureArrayLength = m_texArrayLength;
        uDesc.flags = 0;
        if (sRGB)
        {
            uDesc.flags = uDesc.flags | kUnityXRRenderTextureFlagsSRGB;
        }
        else
        {
            uDesc.flags = uDesc.flags | kUnityXRRenderTextureFlagsAutoResolve;
        }

        // Create an UnityXRRenderTextureId for the native texture so we can tell unity to render to it later.
        UnityXRRenderTextureId uTexId;
        UnitySubsystemErrorCode ret = m_Ctx.display->CreateTexture(m_Handle, &uDesc, &uTexId);
        if (ret != kUnitySubsystemErrorCodeSuccess)
        {
            SUBSYSTEM_ERROR(m_Ctx.trace, "[TextureManager] Failed to create unity texture: %d", (int)ret);
            return false;
        }
        unityTextures[i] = uTexId;
        SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CreateUnityTextures: [%d], idTex=%d, nativePtr=%llu, nativePtr-dep=%llu, depthRefIdTex=%d", i, (int)uTexId, (uint64_t)uDesc.color.nativePtr, (uint64_t)uDesc.depth.nativePtr, (int)uDesc.depth.referenceTextureId);
    }
    // XR_TRACE_LOG(m_Ctx.trace, "[TextureManager] CreateUnityTextures end");
    return true;
}

bool TextureManager::CreateTextures(uint32_t texNum, uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, bool requestDepthTex, bool sRGB)
{
    DestroyTextures();

    XR_TRACE_LOG(m_Ctx.trace, "[TextureManager] CreateTextures texNum:%d width:%d height:%d textureArrayLength:%d requestDepthTex:%d sRGB:%d", texNum, texWidth, texHeight, texArrayLength, requestDepthTex, sRGB?1:0);
    m_texture_num = texNum;
    m_texWidth = texWidth;
    m_texHeight = texHeight;
    m_texArrayLength = texArrayLength;

    if (!CreateNativeTextures(m_texture_num, m_texWidth, m_texHeight, m_texArrayLength, false, m_NativeTextures, sRGB))
        return false;
    if (requestDepthTex && !CreateNativeTextures(m_texture_num, m_texWidth, m_texHeight, m_texArrayLength, true, m_NativeDepthTextures, sRGB))
        return false;


    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    for (int i = 0; i < m_texture_num; i++)
    {
        uint32_t curTexId = m_NativeTextures[i];
        uint32_t curDepthTexId = m_NativeDepthTextures[i];
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(curTexId), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, static_cast<GLuint>(curDepthTexId), 0);

        glClearDepthf(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        CheckForGLError("CreateTextures glClear");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return CreateUnityTextures(m_UnityTextures, requestDepthTex, sRGB);
}

void TextureManager::DestroyNativeTextures(std::vector<uint32_t> &nativeTextures)
{
    for (size_t i = 0; i < nativeTextures.size(); ++i)
    {
        DestroyNativeTexture(nativeTextures[i]);
    }
    nativeTextures.clear();
}

void TextureManager::DestroyUnityTextures(std::vector<UnityXRRenderTextureId> &unityTextures)
{
    for (size_t i = 0; i < unityTextures.size(); ++i)
    {
        if (unityTextures[i] != 0)
        {
            // SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] DestroyUnityTextures: %u", unityTextures[i]);
            m_Ctx.display->DestroyTexture(m_Handle, unityTextures[i]);
        }
    }
    unityTextures.clear();
}

void TextureManager::DestroyTextures()
{
    SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] Destroy textures: %d, %d", m_NativeTextures.size(), m_UnityTextures.size());
    assert(m_NativeTextures.size() == m_UnityTextures.size());
    // assert(m_NativeTextures.size() == m_NativeDepthTextures.size());
    DestroyNativeTextures(m_NativeTextures);
    DestroyNativeTextures(m_NativeDepthTextures);
    DestroyUnityTextures(m_UnityTextures);
}

void TextureManager::DebugDraw()
{
    if (m_texture_index < 0 || m_texture_index >= (int)m_NativeTextures.size())
        return;

    uint32_t curTexId = m_NativeTextures[m_texture_index];
    uint32_t curDepthTexId = m_NativeDepthTextures[m_texture_index];

    ///DEBUG_DEPTHTEXTURE：是否在当前framebuffer上进行绘制
    bool drawOnCurBuffer = true;
    if (drawOnCurBuffer)
    {
        glViewport(0, 0, m_texWidth, m_texHeight);
                
        ///DEBUG_DEPTHTEXTURE：此处通过native方式绘制一个面片，与3D场景中其他物体遮挡关系正常；
        uint32_t colorTex = (uint32_t)(size_t)(g_TextureHandle);
        DrawColoredTriangle(colorTex);

        ///DEBUG_DEPTHTEXTURE：此处调试depth texture
        // 1. 如果使用depth texture（curDepthTexId），则为全黑
        // 2. 如果指定为静态图g_TextureHandle，可以成功绘制，说明绘制命令没有问题
        // 3. 如果指定为当前帧的color texture（curTexId）,可以成功绘制（效果为无限镜像），说明当前的绘制时机无问题，可以正确采样color texture，理论上来说也应该能正确采样depth texture
        uint32_t depthTex = (uint32_t)(size_t)(curDepthTexId);
        DrawDepthTriangle(depthTex);

        glFinish();
    }
    else
    {
        ///DEBUG_DEPTHTEXTURE: 手动bind到新的framebuffer上，该framebuffer的attachment为我们自创建的color texture和depth texture
        /// 这里先通过DrawColoredTriangle绘制到framebuffer上，然后再通过DrawDepthTriangle绘制深度图，发现DrawColoredTriangle成功写入了depth texture，说明depth texture自身的格式没有问题
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(curTexId), 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, static_cast<GLuint>(curDepthTexId), 0);
        
        glViewport(0, 0, m_texWidth, m_texHeight);
        
        glClearDepthf(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        CheckForGLError("glClear");
        
        uint32_t colorTex = (uint32_t)(size_t)(g_TextureHandle);
        DrawColoredTriangle(colorTex);

        uint32_t depthTex = (uint32_t)(size_t)(curDepthTexId);
        DrawDepthTriangle(depthTex);

        glFinish();
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

// Draw a rectangle without depthTest && depthWrite.
// 正交投影
void TextureManager::DrawDepthTriangle(uint32_t texId)
{
    // SUBSYSTEM_LOG(m_Ctx.trace, "DrawDepthTriangle");

	struct MyVertex
	{
		float x, y, z;
		unsigned int color;
        float u, v;
	};
	MyVertex verts[6] =
	{
		{ -1.f,    -1.f,  0, 0xFFff0000, 0.0f, 0.0f },        // 0
		{ 0.f,     -1.0f,  0, 0xFF00ff00, 1.0f, 0.0f },        // 1
		{ -1.f,    0.f,   0, 0xFF0000ff, 0.0f, 1.0f },        // 2
		{ 0.f,     -1.0f,  0, 0xFF00ff00, 1.0f, 0.0f },        // 1
		{ 0.f,     0.f,   0, 0xFF00ffff, 1.0f, 1.0f },        // 3
		{ -1.f,    0.f,   0, 0xFF0000ff, 0.0f, 1.0f },        // 2
	};

	float worldMatrix[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};


	// Tweak the projection matrix a bit to make it match what identity projection would do in D3D case.
	float projectionMatrix[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};
	m_Ctx.renderAPI->DrawSimpleTriangles(worldMatrix, projectionMatrix, 2, verts, texId, false);
}

// This creates a symmetric frustum with horizontal FOV
// by converting 4 params (fovx, aspect=w/h, near, far)
// to 6 params (l, r, b, t, n, f) 
static float* makeFrustum(float fovX, float aspectRatio, float front, float back)
{
    const float DEG2RAD = acos(-1.0f) / 180;

    float tangent = tan(fovX/2 * DEG2RAD);    // tangent of half fovX
    float right = front * tangent;            // half width of near plane
    float top = right / aspectRatio;          // half height of near plane

    // params: left, right, bottom, top, near(front), far(back)
    float* matrix = new float[16];
    memset(matrix, 0, sizeof(float) * 16);
    matrix[0]  =  front / right;
    matrix[5]  =  front / top;
    matrix[10] = -(back + front) / (back - front);
    matrix[11] = -1;
    matrix[14] = -(2 * back * front) / (back - front);
    matrix[15] =  0;
    return matrix;
}

// Draw a rectangle with normal depthTest && depthWrite
// 透视投影
void TextureManager::DrawColoredTriangle(uint32_t texId)
{
    // SUBSYSTEM_LOG(m_Ctx.trace, "DrawColoredTriangle");

	// Draw a colored triangle. Note that colors will come out differently
	// in D3D and OpenGL, for example, since they expect color bytes
	// in different ordering.
	struct MyVertex
	{
		float x, y, z;
		unsigned int color;
        float u, v;
	};
	float finalDepth = 15.f;

	MyVertex verts[6] =
	{
		{ -2.7f,    -2.7f,  0, 0xFFff0000, 0.0f, 0.0f },        // 0
		{ 2.7f,     -2.7f,  0, 0xFF00ff00, 1.0f, 0.0f },        // 1
		{ -2.7f,    2.7f,   0, 0xFF0000ff, 0.0f, 1.0f },        // 2
		{ 2.7f,     -2.7f,  0, 0xFF00ff00, 1.0f, 0.0f },        // 1
		{ 2.7f,     2.7f,   0, 0xFF00ffff, 1.0f, 1.0f },        // 3
		{ -2.7f,    2.7f,   0, 0xFF0000ff, 0.0f, 1.0f },        // 2
	};

	// Transformation matrix: rotate around Z axis based on time.
	float phi = g_Time; // time set externally from Unity script
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);
	float worldMatrix[16] = {
		cosPhi,0,-sinPhi,0,
		0,1,0,0,
		sinPhi,0,cosPhi,0,
		0,0,-finalDepth,1,
	};

    float* projectionMatrix = makeFrustum(67.5f, 16.0f/9.0f, 0.3f, 1000.f);

	m_Ctx.renderAPI->DrawSimpleTriangles(worldMatrix, projectionMatrix, 2, verts, texId, true);
}

void TextureManager::CaptureFrame(const char* path)
{
    // path = "/storage/emulated/0/Android/data/com.nreal.NRInput/files/XrealShotsXR";

    uint32_t curTexId = m_NativeTextures[m_texture_index];
    uint32_t curDepthTexId = m_NativeDepthTextures[m_texture_index];

    SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] CaptureFrame: texId=%u, depthTexId=%u, %s", curTexId, curDepthTexId, path);

    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t timeStamp = t.tv_sec * 1000000000LL + t.tv_nsec;

    std::stringstream fileTex;
    std::stringstream fileDepthTex;
    fileTex << path << "/" << timeStamp << ".bmp";
    fileDepthTex << path << "/" << timeStamp << "_dep.bmp";

    // As for the depth texture, draw a rectangle in a new color texture basing on the depth texture, and save it.
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    uint32_t nativeColorTex = 0;
    CreateColorTexture(m_texWidth, m_texHeight, 0, nativeColorTex, true);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(nativeColorTex), 0);

    // uint32_t nativeDepthTex = 0;
    // CreateDepthTexture(m_texWidth, m_texHeight, 0, nativeDepthTex);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, static_cast<GLuint>(nativeDepthTex), 0);

    DrawColoredTriangle(curDepthTexId);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    SaveTextureToBMP(fileDepthTex.str().c_str(), nativeColorTex, m_texWidth, m_texHeight);


    // For the color texture, just save it directly.
    // SaveTextureToBMP(fileTex.str().c_str(), curTexId, m_texWidth, m_texHeight);
}

#pragma pack(1)
typedef struct BITMAPFILEHEADER  
{   
    uint16_t bfType;   
    uint32_t bfSize;   
    uint16_t bfReserved1;   
    uint16_t bfReserved2;   
    uint32_t bfOffBits;   
}BITMAPFILEHEADER;   

typedef struct BITMAPINFOHEADER  
{   
    uint32_t biSize;   
    uint32_t biWidth;   
    uint32_t biHeight;   
    uint16_t biPlanes;   
    uint16_t biBitCount;   
    uint32_t biCompression;   
    uint32_t biSizeImage;   
    uint32_t biXPelsPerMeter;   
    uint32_t biYPelsPerMeter;   
    uint32_t biClrUsed;   
    uint32_t biClrImportant;   
}BITMAPINFOHEADER;

typedef struct tagRGBTRIPLE { 
    uint8_t rgbtBlue; 
    uint8_t rgbtGreen; 
    uint8_t rgbtRed; 
    uint8_t rgbtAlpha; 
} RGBTRIPLE;
#pragma pack()

bool TextureManager::SaveTextureToBMP(const char *filename, uint32_t texture, uint32_t width, uint32_t height) {
    SUBSYSTEM_LOG(m_Ctx.trace, "SaveTextureToBMP texture=%u width=%u height=%u", texture, width, height);

    GLubyte* pixels = (GLubyte*) malloc(width * height * sizeof(GLubyte) * 4);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<GLuint>(texture), 0);    

    //glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, output_image);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    CheckForGLError("ReadPixels");
    // // glReadPixels(0, 0, width, height, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, pixels_depth);
    // glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, pixels_depth);
    // CheckForGLError("ReadPixelsDepth");

    unsigned char *data = reinterpret_cast<unsigned char *>(pixels);
    SaveDataBufferToBMP(filename, data, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    SUBSYSTEM_LOG(m_Ctx.trace, "SaveTextureToBMP end");
    return true;
}

bool TextureManager::SaveDataBufferToBMP(const char *filename, unsigned char *data, uint32_t width, uint32_t height) {
    SUBSYSTEM_LOG(m_Ctx.trace, "SaveDataBufferToBMP filename=%s", filename);
    if (data == nullptr)
    {
        return false;
    }
    SUBSYSTEM_LOG(m_Ctx.trace, "SaveDataBufferToBMP 1");
    // Open file first - if that fails, we can skip the remaining altogether
    FILE* pFile = nullptr;
    if ((pFile = fopen(filename, "wb")) == NULL)
        return false;
    SUBSYSTEM_LOG(m_Ctx.trace, "SaveDataBufferToBMP 2");
    std::vector<unsigned char> fileBuffer;
    //compose the buffer, starting with the bitmap header and infoheader
    BITMAPFILEHEADER bmpFileHeader = // - BitmapFileHeader
    {
        0x4d42,
        0, 0, 0,
        sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER),
    };
    BITMAPINFOHEADER bmpInfoHeader = // - BitmapInfoHeader
    {
        sizeof(BITMAPINFOHEADER),
        (decltype(bmpInfoHeader.biWidth))width, 
        (decltype(bmpInfoHeader.biHeight))height,
        1,32, 0,0,0,0,0,0
    };
    fileBuffer.resize(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 4); // replace with size from format
    memcpy(&fileBuffer[0], &bmpFileHeader, sizeof(bmpFileHeader));
    memcpy(&fileBuffer[sizeof(bmpFileHeader)], &bmpInfoHeader, sizeof(bmpInfoHeader));
    // Copying one pixel at a time to rearrange the planes from BGRA to BGRA, while the bmp store a pixel data as BGRA order
    for (unsigned int i = 0; i < width * height; ++i)
    {
        // if (i==0)
        // {
        //    SUBSYSTEM_LOG(m_Ctx.trace, "pixel data rgba:{} {} {} {}", data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
        // }
        fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4] = data[i * 4];
        fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4 + 1] = data[i * 4 + 1];
        fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4 + 2] = data[i * 4 + 2];
        fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4 + 3] = data[i * 4 + 3];
        // fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4] = data[i * 4 + 2];
        // fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4 + 1] = data[i * 4 + 1];
        // fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4 + 2] = data[i * 4];
        // fileBuffer[sizeof(bmpFileHeader) + sizeof(bmpInfoHeader) + i * 4 + 3] = data[i * 4 + 3];
    }
    SUBSYSTEM_LOG(m_Ctx.trace, "SaveDataBufferToBMP 3");
    // Now the buffer is ready, simply write to file
    if (fwrite(&fileBuffer[0], fileBuffer.size(), 1, pFile) < 1)
    {
        fclose(pFile);
        return false;
    }
    fclose(pFile);
    SUBSYSTEM_LOG(m_Ctx.trace, "SaveDataBufferToBMP end");
    return true;
}
