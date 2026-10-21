#include "HallXRFrameTiming.h"
#include "IOpenXRExtensionPlugin.h"
#include "Modules/ModuleManager.h"
#include <atomic>

namespace
{
    std::atomic<int64> DisplayPeriod{0};
    PFN_xrGetInstanceProcAddr NextGetProcAddr = nullptr;
    PFN_xrWaitFrame NextWaitFrame = nullptr;

    XrResult XRAPI_PTR HallWaitFrame(XrSession Session, const XrFrameWaitInfo* Info, XrFrameState* State)
    {
        const XrResult Result = NextWaitFrame(Session, Info, State);
        if (XR_SUCCEEDED(Result) && State && State->predictedDisplayPeriod > 0)
            DisplayPeriod.store(State->predictedDisplayPeriod, std::memory_order_relaxed);
        return Result;
    }

    XrResult XRAPI_PTR HallGetProcAddr(XrInstance Instance, const char* Name, PFN_xrVoidFunction* Function)
    {
        const XrResult Result = NextGetProcAddr(Instance, Name, Function);
        if (XR_SUCCEEDED(Result) && Function && *Function && FCStringAnsi::Strcmp(Name, "xrWaitFrame") == 0)
        {
            NextWaitFrame = reinterpret_cast<PFN_xrWaitFrame>(*Function);
            *Function = reinterpret_cast<PFN_xrVoidFunction>(&HallWaitFrame);
        }
        return Result;
    }
}

float GetHallXRRefreshRate()
{
    const int64 Period = DisplayPeriod.load(std::memory_order_relaxed);
    return Period > 0 ? 1.e9f / static_cast<float>(Period) : 0.f;
}

// Observe the standard OpenXR frame period. No vendor extension, refresh-rate
// request, or second session; the engine still owns every OpenXR call and result.
class FMaterialHallXRModule : public IModuleInterface, public IOpenXRExtensionPlugin
{
public:
    virtual void StartupModule() override { RegisterOpenXRExtensionModularFeature(); }
    virtual void ShutdownModule() override { UnregisterOpenXRExtensionModularFeature(); }
    virtual FString GetDisplayName() override { return TEXT("Material Hall frame timing"); }
    virtual bool InsertOpenXRAPILayer(PFN_xrGetInstanceProcAddr& InOutGetProcAddr) override
    {
        if (InOutGetProcAddr == &HallGetProcAddr) return false;
        NextGetProcAddr = InOutGetProcAddr;
        InOutGetProcAddr = &HallGetProcAddr;
        return true;
    }
    virtual void OnDestroySession(XrSession Session) override { DisplayPeriod.store(0, std::memory_order_relaxed); }
};
IMPLEMENT_MODULE(FMaterialHallXRModule, MaterialHallXR)
