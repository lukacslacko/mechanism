#include "MaterialHall.h"
#include "DynamicResolutionState.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#if PLATFORM_WINDOWS
#include "HallXRFrameTiming.h"
#endif
#include "Serialization/JsonSerializer.h"

namespace
{
    constexpr float TargetFrameMs = 1000.f / 60.f;
    const TCHAR* SettingsSection = TEXT("MaterialHall.Performance");

    void SetPerformanceCVar(const TCHAR* Name, float Value)
    {
        if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
            Variable->Set(Value, ECVF_SetByCode);
    }

    float PerformanceCVar(const TCHAR* Name)
    {
        const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
        return Variable ? Variable->GetFloat() : -1.f;
    }
}

void AMaterialHallController::ApplyPerformanceSettings()
{
    if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
    {
        Settings->SetVSyncEnabled(!bVRMode && bVSync);
        Settings->SetDynamicResolutionEnabled(!bVRMode && bAutoTune);
        Settings->SetFrameRateLimit(bVRMode ? 0 : 60);
    }
    // OpenXR's compositor owns headset pacing. A desktop 60 Hz cap or mirror
    // VSync would prevent a 72/80/90/120 Hz headset reaching its native cadence.
    SetPerformanceCVar(TEXT("r.VSync"), !bVRMode && bVSync ? 1 : 0);
    SetPerformanceCVar(TEXT("t.MaxFPS"), bVRMode ? 0 : 60);
    SetPerformanceCVar(TEXT("r.AntiAliasingMethod"), AntiAliasingMethod);
    SetPerformanceCVar(TEXT("r.TemporalAA.Upsampling"), 1);
    SetPerformanceCVar(TEXT("r.DynamicRes.OperationMode"), 1);
    SetPerformanceCVar(TEXT("r.DynamicRes.FrameTimeBudget"), TargetFrameMs);
    SetPerformanceCVar(TEXT("r.DynamicRes.MinScreenPercentage"), 50);
    SetPerformanceCVar(TEXT("r.DynamicRes.MaxScreenPercentage"), 100);
    SetPerformanceCVar(TEXT("r.DynamicRes.TargetedGPUHeadRoomPercentage"), 10);
    SetPerformanceCVar(TEXT("r.DynamicRes.OverBudgetGPUHeadRoomPercentage"), 10);
    SetPerformanceCVar(TEXT("r.DynamicRes.MinResolutionChangePeriod"), 8);
    GEngine->SetDynamicResolutionUserSetting(!bVRMode && bAutoTune);
    if (bVRMode)
    {
        SetPerformanceCVar(TEXT("vr.PixelDensity"), 1);
        SetPerformanceCVar(TEXT("r.ScreenPercentage"), bAutoTune ? VRResolutionGovernor.Percentage : QualityIndex==0 ? 60 : QualityIndex==1 ? 80 : 100);
        SetPerformanceCVar(TEXT("r.MotionBlurQuality"), 0);
        SetPerformanceCVar(TEXT("r.DepthOfFieldQuality"), 0);
    }
}

void AMaterialHallController::SavePerformanceSettings()
{
    // Test runs must not change the user's preferences. Automatic adjustments are
    // deliberately transient; only explicit choices are saved.
    if (bSmokeTest || bPerformanceTest || bVRTest || bVRHardwareTest) return;
    const TCHAR* Section = bVRMode ? TEXT("MaterialHall.VRPerformance") : SettingsSection;
    GConfig->SetBool(Section, TEXT("AutoTune"), bAutoTune, GGameUserSettingsIni);
    GConfig->SetBool(Section, TEXT("VSync"), bVSync, GGameUserSettingsIni);
    GConfig->SetInt(Section, TEXT("AntiAliasingMethod"), AntiAliasingMethod, GGameUserSettingsIni);
    GConfig->SetInt(Section, TEXT("ManualQuality"), ManualQualityIndex, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void AMaterialHallController::SelectQuality(int32 Index)
{
    bAutoTune = false;
    ManualQualityIndex = FMath::Clamp(Index, 0, 2);
    QualityGovernor.Reset();
    ApplyQuality(ManualQualityIndex);
    SavePerformanceSettings();
}

void AMaterialHallController::SetAutoTune(bool bEnabled)
{
    bAutoTune = bEnabled;
    QualityGovernor.Reset();
    VRResolutionGovernor.Reset();
    ApplyPerformanceSettings();
    if (IDynamicResolutionState* State = GEngine->GetDynamicResolutionState()) State->ResetHistory();
    SavePerformanceSettings();
}

float AMaterialHallController::GetRenderPercentage() const
{
    if (bVRMode) return PerformanceCVar(TEXT("r.ScreenPercentage"));
    FDynamicResolutionStateInfos Info;
    GEngine->GetDynamicResolutionCurrentStateInfos(Info);
    if (bAutoTune && (Info.Status == EDynamicResolutionStatus::Enabled || Info.Status == EDynamicResolutionStatus::Paused))
    {
        const float Fraction = Info.ResolutionFractionApproximations[GDynamicPrimaryResolutionFraction];
        if (FMath::IsFinite(Fraction) && Fraction > 0) return Fraction * 100.f;
    }
    return PerformanceCVar(TEXT("r.ScreenPercentage"));
}

void AMaterialHallController::TickPerformance(float DeltaSeconds)
{
    const double Now = FPlatformTime::Seconds();
    uint64 Cycles = 0;
    while (GPUTimingState.PopFrameCycles(Cycles) != FRHIGPUFrameTimeHistory::EResult::Empty)
    {
        const double Sample = FPlatformTime::ToMilliseconds64(Cycles);
        if (FMath::IsFinite(Sample) && Sample > 0 && Sample < 250)
        {
            GPUTimeMs = bHasGPUTiming ? FMath::Lerp(GPUTimeMs, Sample, .05) : Sample;
            bHasGPUTiming = true;
            LastGPUTimingTime = Now;
        }
    }
    // These engine counters exclude idle time, unlike DeltaSeconds under VSync.
    const double WorkMs = FPlatformTime::ToMilliseconds(FMath::Max3(GGameThreadTime, GRenderThreadTime, GRHIThreadTime));
    CPUTimeMs = FMath::Lerp(CPUTimeMs, WorkMs, .05);
    if (bVRMode)
    {
#if PLATFORM_WINDOWS
        const float Hz = GetHallXRRefreshRate();
#else
        const float Hz = 0;
#endif
        if (Hz >= 40 && Hz <= 180 && FMath::Abs(Hz - VRRefreshRate) > .5f)
        {
            VRRefreshRate = Hz;
            QualityGovernor.Reset();
            VRResolutionGovernor.Reset();
            UE_LOG(LogTemp, Display, TEXT("VR compositor target: %.2f Hz, %.2f ms budget"), Hz, 1000.f / Hz);
        }
        if (!bAutoTune) return;
        const bool bValid = IsVRActive() && bHasGPUTiming && Now - LastGPUTimingTime < .5 && VRRefreshRate > 0;
        const double Budget = VRRefreshRate > 0 ? 1000.0 / VRRefreshRate : 1000.0 / 72.0;
        const float Percentage = VRResolutionGovernor.Update(DeltaSeconds, GPUTimeMs, CPUTimeMs, Budget, bValid);
        SetPerformanceCVar(TEXT("r.ScreenPercentage"), Percentage);
        // The slow effects governor works in normalized 60 Hz work units.
        const int32 Next = QualityGovernor.Update(DeltaSeconds, GPUTimeMs * TargetFrameMs / Budget,
            CPUTimeMs * TargetFrameMs / Budget, Percentage, QualityIndex, bValid);
        if (Next != QualityIndex)
        {
            UE_LOG(LogTemp, Display, TEXT("VR auto effects %d -> %d, GPU %.2f ms, target %.1f Hz, resolution %.0f%%"), QualityIndex, Next, GPUTimeMs, VRRefreshRate, Percentage);
            ApplyQuality(Next);
            VRResolutionGovernor.Reset();
        }
        return;
    }
    const bool bActive = bSmokeTest || bPerformanceTest || FApp::HasFocus();
    if (bAutoTune && !bActive && !bDynamicResolutionPaused)
    {
        GEngine->PauseDynamicResolution();
        bDynamicResolutionPaused = true;
    }
    else if (bDynamicResolutionPaused && (bActive || !bAutoTune))
    {
        GEngine->ResumeDynamicResolution();
        bDynamicResolutionPaused = false;
        QualityGovernor.Reset();
        if (IDynamicResolutionState* State = GEngine->GetDynamicResolutionState()) State->ResetHistory();
    }
    if (!bAutoTune) return;
    FDynamicResolutionStateInfos Info;
    GEngine->GetDynamicResolutionCurrentStateInfos(Info);
    const bool bValid = bActive && bHasGPUTiming && Now - LastGPUTimingTime < .5 && Info.Status == EDynamicResolutionStatus::Enabled;
    const int32 NextQuality = QualityGovernor.Update(DeltaSeconds, GPUTimeMs, CPUTimeMs, GetRenderPercentage(), QualityIndex, bValid);
    if (NextQuality != QualityIndex)
    {
        UE_LOG(LogTemp, Display, TEXT("AUTO 60: effects %d -> %d, GPU %.2f ms, CPU %.2f ms, resolution %.1f%%"),
            QualityIndex, NextQuality, GPUTimeMs, CPUTimeMs, GetRenderPercentage());
        ApplyQuality(NextQuality);
    }
    if (bPerformanceTest && Now - TestStart > 45)
    {
        PerformanceFrameSum += DeltaSeconds;
        ++PerformanceFrameCount;
    }
}

FString AMaterialHallController::GetPerformanceStatus() const
{
    const TCHAR* Names[] = {TEXT("Smooth"), TEXT("Balanced"), TEXT("Fine")};
    if (bVRMode)
    {
        FString Status = FString::Printf(TEXT("%s | %s | %.0f fps / %.0f Hz\nResolution %.0f%% | GPU %.1f ms"),
            bAutoTune ? TEXT("Auto") : TEXT("Manual"), Names[QualityIndex],
            1.f / FMath::Max(.001f, SmoothedFrameTime), VRRefreshRate, GetRenderPercentage(), GPUTimeMs);
        if (VRRefreshRate <= 0) Status += TEXT("\nWaiting for headset frame timing");
        else if (!IsVRActive()) Status += TEXT("\nTuning paused: headset not focused / tracked");
        else if (CPUTimeMs > 1000.f / VRRefreshRate && CPUTimeMs > GPUTimeMs) Status += TEXT("\nCPU workload exceeds headset frame budget");
        else if (QualityIndex == 0 && GetRenderPercentage() <= 55 && GPUTimeMs > 1000.f / VRRefreshRate) Status += TEXT("\nGPU workload exceeds quality floor");
        return Status;
    }
    FString Status = FString::Printf(TEXT("%s · %s · %.0f fps / 60\nResolution %.0f%% · GPU %.1f ms"),
        bAutoTune ? TEXT("Auto") : TEXT("Manual"), Names[QualityIndex],
        1.f / FMath::Max(.001f, SmoothedFrameTime), GetRenderPercentage(), GPUTimeMs);
    FDynamicResolutionStateInfos Info;
    GEngine->GetDynamicResolutionCurrentStateInfos(Info);
    if (bAutoTune && Info.Status == EDynamicResolutionStatus::Unsupported)
        Status += TEXT("\nAuto resolution unavailable on this renderer");
    else if (bAutoTune && bDynamicResolutionPaused)
        Status += TEXT("\nTuning paused while in background");
    else if (bAutoTune && bHasGPUTiming && CPUTimeMs > 17.5 && CPUTimeMs > GPUTimeMs)
        Status += TEXT("\nCPU workload exceeds the 60 fps budget");
    else if (bAutoTune && QualityIndex == 0 && GetRenderPercentage() <= 55 && GPUTimeMs > TargetFrameMs)
        Status += TEXT("\nGPU workload exceeds the quality floor");
    return Status;
}

void AMaterialHallController::TickPerformanceTest()
{
    const double Elapsed = FPlatformTime::Seconds() - TestStart;
    auto Check = [this](const TCHAR* Name, bool bPassed)
    {
        bPerformanceTestPassed &= bPassed;
        TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("name"), Name);
        Record->SetBoolField(TEXT("passed"), bPassed);
        PerformanceResults.Add(MakeShared<FJsonValueObject>(Record));
    };
    FDynamicResolutionStateInfos Info;
    GEngine->GetDynamicResolutionCurrentStateInfos(Info);
    if (TestStage == 1) LowestTestPercentage = FMath::Min(LowestTestPercentage, GetRenderPercentage());
    if (TestStage == 2) RecoveredTestPercentage = FMath::Max(RecoveredTestPercentage, GetRenderPercentage());

    if (TestStage == 0 && Elapsed > 8)
    {
        ++TestStage;
        Check(TEXT("default_auto_vsync_tsr_60"), bAutoTune && bVSync && AntiAliasingMethod == 4 &&
            PerformanceCVar(TEXT("r.VSync")) == 1 && PerformanceCVar(TEXT("r.AntiAliasingMethod")) == 4 &&
            PerformanceCVar(TEXT("t.MaxFPS")) == 60 && Info.Status == EDynamicResolutionStatus::Enabled);
        Check(TEXT("gpu_work_timing_available"), bHasGPUTiming && GPUTimeMs > 0);
        // Tighten the real engine budget to exercise pressure and recovery through
        // actual GPU timing and dynamic resolution, without changing scene assets.
        SetPerformanceCVar(TEXT("r.DynamicRes.FrameTimeBudget"), 1);
    }
    else if (TestStage == 1 && Elapsed > 18)
    {
        ++TestStage;
        Check(TEXT("resolution_decreases_under_pressure"), LowestTestPercentage >= 49.9f && LowestTestPercentage < 75.f);
        Capture(TEXT("performance-pressure.png"));
        SetPerformanceCVar(TEXT("r.DynamicRes.FrameTimeBudget"), TargetFrameMs);
    }
    else if (TestStage == 2 && Elapsed > 30)
    {
        ++TestStage;
        Check(TEXT("resolution_recovers_with_headroom"), RecoveredTestPercentage > LowestTestPercentage + 10 && RecoveredTestPercentage <= 100.1f);
        bVSync = false;
        AntiAliasingMethod = 2;
        ApplyPerformanceSettings();
        Check(TEXT("vsync_off_and_taa_apply"), PerformanceCVar(TEXT("r.VSync")) == 0 && PerformanceCVar(TEXT("r.AntiAliasingMethod")) == 2);
        SelectQuality(2);
    }
    else if (TestStage == 3 && Elapsed > 33)
    {
        ++TestStage;
        Check(TEXT("manual_preset_disables_auto"), !bAutoTune && QualityIndex == 2 && Info.Status == EDynamicResolutionStatus::Disabled &&
            FMath::IsNearlyEqual(GetRenderPercentage(), 85.f));
        Capture(TEXT("performance-taa.png"));
    }
    else if (TestStage == 4 && Elapsed > 35)
    {
        ++TestStage;
        bVSync = true;
        AntiAliasingMethod = 4;
        SelectQuality(1);
        SetAutoTune(true);
    }
    else if (TestStage == 5 && Elapsed > 67)
    {
        ++TestStage;
        Check(TEXT("auto_restores_vsync_tsr"), bAutoTune && bVSync && AntiAliasingMethod == 4 &&
            PerformanceCVar(TEXT("r.VSync")) == 1 && PerformanceCVar(TEXT("r.AntiAliasingMethod")) == 4 && Info.Status == EDynamicResolutionStatus::Enabled);
        Check(TEXT("quality_increases_with_headroom"), QualityIndex == 2);
        Check(TEXT("steady_60fps_target"), PerformanceFrameCount > 100 && PerformanceFrameSum / PerformanceFrameCount < .018);
        Capture(TEXT("performance-auto.png"));
    }
    else if (TestStage == 6 && Elapsed > 69)
    {
        bTestFinished = true;
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("passed"), bPerformanceTestPassed);
        Result->SetArrayField(TEXT("checks"), PerformanceResults);
        Result->SetNumberField(TEXT("minimum_pressure_resolution_percent"), LowestTestPercentage);
        Result->SetNumberField(TEXT("recovered_resolution_percent"), RecoveredTestPercentage);
        Result->SetNumberField(TEXT("final_quality"), QualityIndex);
        Result->SetNumberField(TEXT("gpu_ms"), GPUTimeMs);
        Result->SetNumberField(TEXT("cpu_ms"), CPUTimeMs);
        Result->SetNumberField(TEXT("steady_average_fps"), PerformanceFrameCount / FMath::Max(.001, PerformanceFrameSum));
        FString Output;
        auto Writer = TJsonWriterFactory<>::Create(&Output);
        FJsonSerializer::Serialize(Result, Writer);
        IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
        FFileHelper::SaveStringToFile(Output, *(CaptureDirectory / TEXT("performance-check.json")));
        UE_LOG(LogTemp, Display, TEXT("MATERIAL HALL PERFORMANCE TEST %s"), bPerformanceTestPassed ? TEXT("PASSED") : TEXT("FAILED"));
        FGenericPlatformMisc::RequestExit(false);
    }
}
