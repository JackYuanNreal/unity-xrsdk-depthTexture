#pragma once
#include <cmath>
#include <vector>
#include <GLES3/gl3.h>
#include <stdint.h>
#include <string>
#include "ProviderContext.h"
#include "XR/UnityXRDisplayStats.h"
#include "XR/IUnityXRDisplay.h"
#include "XR/IUnityXRTrace.h"
#include "TextureManager.h"


class TextureManager
{
public:
    TextureManager(ProviderContext &ctx, UnitySubsystemHandle handle)
        : m_Ctx(ctx), m_Handle(handle)
    {
        SUBSYSTEM_LOG(m_Ctx.trace, "[TextureManager] new");
    }

    uint32_t GetTextureNum() { return m_UnityTextures.size(); }
    bool CreateTextures(uint32_t texNum, uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, bool requestDepthTex, bool sRGB);
    void DestroyTextures();
    
    bool AcquireTexture();
    UnityXRRenderTextureId GetCurTexture();
    void DebugDraw();

private:
    bool CheckForGLError(const std::string &message);

    bool CreateColorTexture(uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, uint32_t &nativeTexOut, bool sRGB);
    bool CreateDepthTexture(uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, uint32_t &nativeTexOut);
    bool CreateNativeTextures(int texNum, uint32_t texWidth, uint32_t texHeight, uint32_t texArrayLength, bool requestDepthTex, std::vector<uint32_t> &nativeTextures, bool sRGB);
    void DestroyNativeTexture(uint32_t nativeTex);
    void DestroyNativeTextures(std::vector<uint32_t> &nativeTextures);
    bool CreateUnityTextures(std::vector<uint32_t> &unityTextures, bool requestDepthTex, bool sRGB);
    void DestroyUnityTextures(std::vector<UnityXRRenderTextureId> &unityTextures);

    void DrawColoredTriangle(uint32_t texId);
    void DrawDepthTriangle(uint32_t texId);

private:
    ProviderContext &m_Ctx;
    UnitySubsystemHandle m_Handle;

    bool m_IsStarted = false;
    std::vector<uint32_t> m_NativeTextures;
    std::vector<uint32_t> m_NativeDepthTextures;
    std::vector<UnityXRRenderTextureId> m_UnityTextures;

    int m_texture_index = 0;
    uint32_t m_texWidth = 1920;
    uint32_t m_texHeight = 1080;
    uint32_t m_texArrayLength = 2;
    uint32_t m_texture_num;
};
