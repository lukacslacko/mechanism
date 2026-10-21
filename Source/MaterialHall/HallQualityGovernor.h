#pragma once

#include "CoreMinimal.h"

// Slow effects governor layered over Unreal's fast GPU-driven dynamic resolution.
// Inputs are work times, excluding the frame limiter / VSync's idle time.
struct FHallQualityGovernor
{
    double Cooldown = 5.0;
    double OverBudget = 0.0;
    double Headroom = 0.0;

    void Reset(double SettleSeconds = 5.0)
    {
        Cooldown = SettleSeconds;
        OverBudget = Headroom = 0.0;
    }

    int32 Update(double DT, double GPUms, double CPUms, float ScreenPercentage, int32 Quality, bool bActive)
    {
        if (!bActive || !FMath::IsFinite(DT) || DT <= 0 || DT > .25 ||
            !FMath::IsFinite(GPUms) || GPUms <= 0 || GPUms > 250 ||
            !FMath::IsFinite(CPUms) || CPUms < 0 || CPUms > 250)
        {
            Reset();
            return Quality;
        }
        if (Cooldown > 0)
        {
            Cooldown = FMath::Max(0.0, Cooldown - DT);
            return Quality;
        }
        // Let dynamic resolution respond first; reduce effects only near its floor,
        // or when CPU rendering work itself exceeds the frame budget.
        const bool bOverBudget = (GPUms > 16.67 && ScreenPercentage <= 55.f) || CPUms > 17.5;
        const bool bHeadroom = GPUms < 12.0 && CPUms < 12.0 && ScreenPercentage >= 95.f;
        OverBudget = bOverBudget ? OverBudget + DT : 0.0;
        Headroom = bHeadroom ? Headroom + DT : 0.0;
        if (OverBudget >= 3.0 && Quality > 0)
        {
            Reset(8.0);
            return Quality - 1;
        }
        if (Headroom >= 12.0 && Quality < 2)
        {
            Reset(8.0);
            return Quality + 1;
        }
        return Quality;
    }
};
