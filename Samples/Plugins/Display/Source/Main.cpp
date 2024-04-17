#include "IUnityInterface.h"
#include "XR/IUnityXRTrace.h"
#include "XR/UnitySubsystemTypes.h"

#include "ProviderContext.h"
#include "ExampleDisplayProvider.h"

ProviderContext* s_Context{};

UnitySubsystemErrorCode Load_Display(ProviderContext&);
UnitySubsystemErrorCode Load_Input(ProviderContext&);


void RegistGraphicsDeviceEvent();
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static bool ReportError(const char* name, UnitySubsystemErrorCode err)
{
    if (err != kUnitySubsystemErrorCodeSuccess)
    {
        XR_TRACE_ERROR(s_Context->trace, "Error loading subsystem: %s (%d)\n", name, err);
        return true;
    }
    return false;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    auto* ctx = s_Context = new ProviderContext;

    ctx->interfaces = unityInterfaces;
    ctx->trace = unityInterfaces->Get<IUnityXRTrace>();

    RegistGraphicsDeviceEvent();

    if (ReportError("Display", Load_Display(*ctx)))
        return;

    if (ReportError("Input", Load_Input(*ctx)))
        return;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    delete s_Context;
}




// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

float g_Time;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity (float t) { g_Time = t; }



void RegistGraphicsDeviceEvent()
{
    s_Context->graphics = s_Context->interfaces->Get<IUnityGraphics>();
    s_Context->graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
        
    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    // to not miss the event in case the graphics device is already initialized
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
	// Create graphics API implementation upon initialization
	if (eventType == kUnityGfxDeviceEventInitialize)
	{
		UnityGfxRenderer deviceType = s_Context->graphics->GetRenderer();
        XR_TRACE_LOG(s_Context->trace, "[Main]  : %d\n", (int)deviceType);
		s_Context->renderAPI = CreateRenderAPI(deviceType);
	}

	// Let the implementation process the device related events
	if (s_Context->renderAPI)
	{
		s_Context->renderAPI->ProcessDeviceEvent(eventType, s_Context->interfaces);
	}

	// Cleanup graphics API implementation upon shutdown
	if (eventType == kUnityGfxDeviceEventShutdown)
	{
		delete s_Context->renderAPI;
		s_Context->renderAPI = NULL;
	}
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
    // XR_TRACE_LOG(s_Context->trace, "[Main] OnRenderEvent: %d\n", eventID);
	// Unknown / unsupported graphics device type? Do nothing
	if (s_Context->renderAPI == NULL || s_Context->displayProvider == NULL)
		return;

    s_Context->displayProvider->DebugDraw();
}

// --------------------------------------------------------------------------
// GetRenderEventFunc, an example function we export which is used to get a rendering event callback function.

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
	return OnRenderEvent;
}

// --------------------------------------------------------------------------
// SetTextureFromUnity, an example function we export which is called by one of the scripts.

void* g_TextureHandle = NULL;
static int   g_TextureWidth  = 0;
static int   g_TextureHeight = 0;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTextureFromUnity(void* textureHandle, int w, int h)
{
	// A script calls this at initialization time; just remember the texture pointer here.
	// Will update texture pixels each frame from the plugin rendering event (texture update
	// needs to happen on the rendering thread).
	g_TextureHandle = textureHandle;
	g_TextureWidth = w;
	g_TextureHeight = h;
       
	uint32_t idTex = (uint32_t)(size_t)(g_TextureHandle);
	XR_TRACE_LOG(s_Context->trace, "[Main] SetTextureFromUnity: %u, %dx%d\n", idTex, w, h);
}
