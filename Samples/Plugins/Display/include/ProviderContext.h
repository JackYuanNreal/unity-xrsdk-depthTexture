#pragma once

#include <cassert>

struct IUnityXRTrace;
struct IUnityXRDisplayInterface;
struct IUnityXRInputInterface;
struct IUnityGraphics;

class ExampleDisplayProvider;
class ExampleTrackingProvider;

#include "RenderAPI.h"
#include "XR/UnitySubsystemTypes.h"


#define SUBSYSTEM_ERROR(trace_context, ...) XR_TRACE_ERROR(trace_context, ##__VA_ARGS__);
#define SUBSYSTEM_LOG(trace_context, ...) XR_TRACE_LOG(trace_context, ##__VA_ARGS__);
#define SUBSYSTEM_DEBUG(trace_context, ...) XR_TRACE_DEBUG(trace_context, ##__VA_ARGS__);

struct ProviderContext
{
    IUnityInterfaces* interfaces;
    IUnityXRTrace* trace;

    IUnityXRDisplayInterface* display;
    ExampleDisplayProvider* displayProvider;

    IUnityXRInputInterface* input;
    ExampleTrackingProvider* trackingProvider;

    IUnityGraphics *graphics;
    RenderAPI *renderAPI;
};

inline ProviderContext& GetProviderContext(void* data)
{
    assert(data != NULL);
    return *static_cast<ProviderContext*>(data);
}

class ProviderImpl
{
public:
    ProviderImpl(ProviderContext& ctx, UnitySubsystemHandle handle)
        : m_Ctx(ctx)
        , m_Handle(handle)
    {
    }
    virtual ~ProviderImpl() {}

    virtual UnitySubsystemErrorCode Initialize() = 0;
    virtual UnitySubsystemErrorCode Start() = 0;

    virtual void Stop() = 0;
    virtual void Shutdown() = 0;

protected:
    ProviderContext& m_Ctx;
    UnitySubsystemHandle m_Handle;
};
