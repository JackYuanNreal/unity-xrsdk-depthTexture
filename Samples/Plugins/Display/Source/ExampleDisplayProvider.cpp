#include "XR/IUnityXRDisplay.h"
#include "XR/IUnityXRTrace.h"

#include "ExampleDisplayProvider.h"
#include "ProviderContext.h"
#include "TextureManager.h"
#include "TextureManager.h"
#include <cmath>
#include <vector>

// We'll use DX11 to allocate textures if we're on windows.
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
#include "D3D11.h"
#include "IUnityGraphicsD3D11.h"
#define XR_DX11 1
#else
#define XR_DX11 0
#endif

#define NUM_RENDER_PASSES 2
static const float s_PoseXPositionPerPass[] = {-1.0f, 1.0f};

// BEGIN WORKAROUND: skip first frame since we get invalid data.  Fix coming to trunk.
static bool s_SkipFrame = true;
#define WORKAROUND_SKIP_FIRST_FRAME()           \
    if (s_SkipFrame)                            \
    {                                           \
        s_SkipFrame = false;                    \
        return kUnitySubsystemErrorCodeSuccess; \
    }
#define WORKAROUND_RESET_SKIP_FIRST_FRAME() s_SkipFrame = true;
// END WORKAROUND

UnitySubsystemErrorCode ExampleDisplayProvider::Initialize()
{
    m_TexMan = new TextureManager(m_Ctx, m_Handle);
    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleDisplayProvider::Start()
{
    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleDisplayProvider::GfxThread_Start(UnityXRRenderingCapabilities& renderingCaps)
{
    // renderingCaps.noSinglePassRenderingSupport = GetHardwareSupportsLayerIndexInVertexShader();
    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleDisplayProvider::GfxThread_SubmitCurrentFrame()
{
    // SubmitFrame();

    SUBSYSTEM_LOG(m_Ctx.trace, "[ExampleDisplayProvider] GfxThread_SubmitCurrentFrame");
    m_FrameCnt++;
    if (m_FrameCnt == 100)
        m_TexMan->CaptureFrame("/sdcard/Android/data/com.unityxr.Display/files/");
    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleDisplayProvider::GfxThread_PopulateNextFrameDesc(const UnityXRFrameSetupHints& frameHints, UnityXRNextFrameDesc& nextFrame)
{
    WORKAROUND_SKIP_FIRST_FRAME();

    // BlockUntilUnityShouldStartSubmittingRenderingCommands();

    bool reallocateTextures = (m_TexMan->GetTextureNum() == 0);
    if ((kUnityXRFrameSetupHintsChangedSinglePassRendering & frameHints.changedFlags) != 0)
    {
        reallocateTextures = true;
    }
    if ((kUnityXRFrameSetupHintsChangedRenderViewport & frameHints.changedFlags) != 0)
    {
        // Change sampling UVs for compositor, pass through new viewport on `nextFrame`
    }
    if ((kUnityXRFrameSetupHintsChangedTextureResolutionScale & frameHints.changedFlags) != 0)
    {
        reallocateTextures = true;
    }
    if ((kUnityXRFrameSetuphintsChangedContentProtectionState & frameHints.changedFlags) != 0)
    {
        // App wants different content protection mode.
    }
    if ((kUnityXRFrameSetuphintsChangedReprojectionMode & frameHints.changedFlags) != 0)
    {
        // App wants different reprojection mode, configure compositor if possible.
    }
    if ((kUnityXRFrameSetuphintsChangedFocusPlane & frameHints.changedFlags) != 0)
    {
        // App changed focus plane, configure compositor if possible.
    }

    if (reallocateTextures)
    {
        DestroyTextures();

        int numTextures = 1;
        int textureArrayLength = 0;

        CreateTextures(numTextures, textureArrayLength, frameHints.appSetup.textureResolutionScale);
    }

    m_TexMan->AcquireTexture();

    UnityXRRenderTextureId curUnityTex = m_TexMan->GetCurTexture();
    SUBSYSTEM_LOG(m_Ctx.trace, "[ExampleDisplayProvider] GfxThread_PopulateNextFrameDesc :singlePassRender=%d, unityTexId=%d", frameHints.appSetup.singlePassRendering ? 1:0, (int)curUnityTex);

    // Frame hints tells us if we should setup our renderpasses with a single pass
    {
        // Use multi-pass rendering to render

        // Can increase render pass count to do wide FOV or to have a separate view into scene.
        nextFrame.renderPassesCount = 1;

        for (int pass = 0; pass < nextFrame.renderPassesCount; ++pass)
        {
            auto& renderPass = nextFrame.renderPasses[pass];

            // Texture that unity will render to next frame.  We created it above.
            // You might want to change this dynamically to double / triple buffer.
            renderPass.textureId = curUnityTex;

            // One set of render params per pass.
            renderPass.renderParamsCount = 1;

            // Note that you can share culling between multiple passes by setting this to the same index.
            renderPass.cullingPassIndex = pass;

            // Fill out render params. View, projection, viewport for pass.
            auto& cullingPass = nextFrame.cullingPasses[pass];
            cullingPass.separation = fabs(s_PoseXPositionPerPass[1]) + fabs(s_PoseXPositionPerPass[0]);

            auto& renderParams = renderPass.renderParams[0];
            renderParams.deviceAnchorToEyePose = cullingPass.deviceAnchorToCullingPose = GetPose(pass);
            renderParams.projection = cullingPass.projection = GetProjection(pass);

            // App has hinted that it would like to render to a smaller viewport.  Tell unity to render to that viewport.
            renderParams.viewportRect = frameHints.appSetup.renderViewport;

            // Tell the compositor what pixels were rendered to for display.
            // Compositor_SetRenderSubRect(pass, renderParams.viewportRect);

        }
    }

    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleDisplayProvider::GfxThread_Stop()
{
    WORKAROUND_RESET_SKIP_FIRST_FRAME();
    return kUnitySubsystemErrorCodeSuccess;
}

UnitySubsystemErrorCode ExampleDisplayProvider::GfxThread_FinalBlitToGameViewBackBuffer(const UnityXRMirrorViewBlitInfo* mirrorBlitInfo, ProviderContext& ctx)
{
    return UnitySubsystemErrorCode::kUnitySubsystemErrorCodeSuccess;
}

void ExampleDisplayProvider::Stop()
{
}

void ExampleDisplayProvider::Shutdown()
{
}

void ExampleDisplayProvider::CreateTextures(int numTextures, int textureArrayLength, float requestedTextureScale)
{
    const int texWidth = (int)(1920.0f * requestedTextureScale);
    const int texHeight = (int)(1080.0f * requestedTextureScale);

    m_TexMan->CreateTextures(numTextures, texWidth, texHeight, textureArrayLength, true, true);
}

void ExampleDisplayProvider::DestroyTextures()
{
   m_TexMan->DestroyTextures();
}

UnityXRPose ExampleDisplayProvider::GetPose(int pass)
{
    UnityXRPose pose{};
    if (pass < (sizeof(s_PoseXPositionPerPass) / sizeof(s_PoseXPositionPerPass[0])))
        pose.position.x = s_PoseXPositionPerPass[pass];
    pose.position.z = -10.0f;
    pose.rotation.w = 1.0f;
    return pose;
}

UnityXRProjection ExampleDisplayProvider::GetProjection(int pass)
{
    UnityXRProjection ret;
    ret.type = kUnityXRProjectionTypeHalfAngles;
    ret.data.halfAngles.left = -1.0;
    ret.data.halfAngles.right = 1.0;
    ret.data.halfAngles.top = 0.625;
    ret.data.halfAngles.bottom = -0.625;
    return ret;
}

UnitySubsystemErrorCode ExampleDisplayProvider::QueryMirrorViewBlitDesc(const UnityXRMirrorViewBlitInfo* mirrorBlitInfo, UnityXRMirrorViewBlitDesc* blitDescriptor, ProviderContext& ctx)
{
    // return kUnitySubsystemErrorCodeSuccess;
    
    if (m_TexMan->GetTextureNum() == 0)
    {
        // Eye texture is not available yet, return failure
        return UnitySubsystemErrorCode::kUnitySubsystemErrorCodeFailure;
    }
    int srcTexId = m_TexMan->GetCurTexture();
    const UnityXRVector2 sourceTextureSize = {static_cast<float>(1920), static_cast<float>(1080)};
    const UnityXRRectf sourceUVRect = {0.0f, 0.0f, 1.0f, 1.0f};
    const UnityXRVector2 destTextureSize = {static_cast<float>(mirrorBlitInfo->mirrorRtDesc->rtScaledWidth), static_cast<float>(mirrorBlitInfo->mirrorRtDesc->rtScaledHeight)};
    const UnityXRRectf destUVRect = {0.0f, 0.0f, 1.0f, 1.0f};

    // By default, The source rect will be adjust so that it matches the dest rect aspect ratio.
    // This has the visual effect of expanding the source image, resulting in cropping
    // along the non-fitting axis. In this mode, the destination rect will be completely
    // filled, but not all the source image may be visible.
    UnityXRVector2 sourceUV0, sourceUV1, destUV0, destUV1;

    float sourceAspect = (sourceTextureSize.x * sourceUVRect.width) / (sourceTextureSize.y * sourceUVRect.height);
    float destAspect = (destTextureSize.x * destUVRect.width) / (destTextureSize.y * destUVRect.height);
    float ratio = sourceAspect / destAspect;
    UnityXRVector2 sourceUVCenter = {sourceUVRect.x + sourceUVRect.width * 0.5f, sourceUVRect.y + sourceUVRect.height * 0.5f};
    UnityXRVector2 sourceUVSize = {sourceUVRect.width, sourceUVRect.height};
    UnityXRVector2 destUVCenter = {destUVRect.x + destUVRect.width * 0.5f, destUVRect.y + destUVRect.height * 0.5f};
    UnityXRVector2 destUVSize = {destUVRect.width, destUVRect.height};

    if (ratio > 1.0f)
    {
        sourceUVSize.x /= ratio;
    }
    else
    {
        sourceUVSize.y *= ratio;
    }

    sourceUV0 = {sourceUVCenter.x - (sourceUVSize.x * 0.5f), sourceUVCenter.y - (sourceUVSize.y * 0.5f)};
    sourceUV1 = {sourceUV0.x + sourceUVSize.x, sourceUV0.y + sourceUVSize.y};
    destUV0 = {destUVCenter.x - destUVSize.x * 0.5f, destUVCenter.y - destUVSize.y * 0.5f};
    destUV1 = {destUV0.x + destUVSize.x, destUV0.y + destUVSize.y};

    (*blitDescriptor).blitParamsCount = 1;
    (*blitDescriptor).blitParams[0].srcTexId = srcTexId;
    (*blitDescriptor).blitParams[0].srcTexArraySlice = 0;
    (*blitDescriptor).blitParams[0].srcRect = {sourceUV0.x, sourceUV0.y, sourceUV1.x - sourceUV0.x, sourceUV1.y - sourceUV0.y};
    (*blitDescriptor).blitParams[0].destRect = {destUV0.x, destUV0.y, destUV1.x - destUV0.x, destUV1.y - destUV0.y};
    return kUnitySubsystemErrorCodeSuccess;
}

void ExampleDisplayProvider::DebugDraw()
{
   m_TexMan->DebugDraw();
}

// Binding to C-API below here

static UnitySubsystemErrorCode UNITY_INTERFACE_API Display_Initialize(UnitySubsystemHandle handle, void* userData)
{
    auto& ctx = GetProviderContext(userData);

    ctx.displayProvider = new ExampleDisplayProvider(ctx, handle);

    // Register for callbacks on the graphics thread.
    UnityXRDisplayGraphicsThreadProvider gfxThreadProvider{};
    gfxThreadProvider.userData = &ctx;

    gfxThreadProvider.Start = [](UnitySubsystemHandle handle, void* userData, UnityXRRenderingCapabilities* renderingCaps) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->GfxThread_Start(*renderingCaps);
    };

    gfxThreadProvider.SubmitCurrentFrame = [](UnitySubsystemHandle handle, void* userData) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->GfxThread_SubmitCurrentFrame();
    };

    gfxThreadProvider.PopulateNextFrameDesc = [](UnitySubsystemHandle handle, void* userData, const UnityXRFrameSetupHints* frameHints, UnityXRNextFrameDesc* nextFrame) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->GfxThread_PopulateNextFrameDesc(*frameHints, *nextFrame);
    };

    gfxThreadProvider.Stop = [](UnitySubsystemHandle handle, void* userData) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->GfxThread_Stop();
    };

    gfxThreadProvider.BlitToMirrorViewRenderTarget = [](UnitySubsystemHandle handle, void* userData, const UnityXRMirrorViewBlitInfo mirrorBlitInfo) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->GfxThread_FinalBlitToGameViewBackBuffer(&mirrorBlitInfo, ctx);
    };

    ctx.display->RegisterProviderForGraphicsThread(handle, &gfxThreadProvider);

    UnityXRDisplayProvider provider{&ctx, NULL, NULL};
    provider.QueryMirrorViewBlitDesc = [](UnitySubsystemHandle handle, void* userData, const UnityXRMirrorViewBlitInfo mirrorBlitInfo, UnityXRMirrorViewBlitDesc* blitDescriptor) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->QueryMirrorViewBlitDesc(&mirrorBlitInfo, blitDescriptor, ctx);
    };

    ctx.display->RegisterProvider(handle, &provider);

    return ctx.displayProvider->Initialize();
}

UnitySubsystemErrorCode Load_Display(ProviderContext& ctx)
{
    XR_TRACE_LOG(ctx.trace, "Load_Display\n");
    ctx.display = ctx.interfaces->Get<IUnityXRDisplayInterface>();
    if (ctx.display == NULL)
        return kUnitySubsystemErrorCodeFailure;

    UnityLifecycleProvider displayLifecycleHandler{};
    displayLifecycleHandler.userData = &ctx;
    displayLifecycleHandler.Initialize = &Display_Initialize;

    displayLifecycleHandler.Start = [](UnitySubsystemHandle handle, void* userData) -> UnitySubsystemErrorCode {
        auto& ctx = GetProviderContext(userData);
        return ctx.displayProvider->Start();
    };

    displayLifecycleHandler.Stop = [](UnitySubsystemHandle handle, void* userData) -> void {
        auto& ctx = GetProviderContext(userData);
        ctx.displayProvider->Stop();
    };

    displayLifecycleHandler.Shutdown = [](UnitySubsystemHandle handle, void* userData) -> void {
        auto& ctx = GetProviderContext(userData);
        ctx.displayProvider->Shutdown();
        delete ctx.displayProvider;
    };

    XR_TRACE_LOG(ctx.trace, "Load_Display regist\n");
    return ctx.display->RegisterLifecycleProvider("XR SDK Display Sample", "Display Sample", &displayLifecycleHandler);
}
