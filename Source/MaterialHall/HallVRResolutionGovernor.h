#pragma once
#include "CoreMinimal.h"

// Stereo cannot use UE's default dynamic-resolution state. Adjust its primary
// screen percentage using GPU work (not compositor waits), with slow recovery.
struct FHallVRResolutionGovernor
{
    float Percentage = 80.f;
    double Cooldown = 3.0;
    double Pressure = 0;
    double Headroom = 0;

    void Reset() { Cooldown = 3; Pressure = Headroom = 0; }

    float Update(double DT, double GPUms, double CPUms, double BudgetMs, bool bActive)
    {
        if (!bActive || !FMath::IsFinite(DT) || DT <= 0 || DT > .25 ||
            !FMath::IsFinite(GPUms) || GPUms <= 0 || GPUms > 250 ||
            !FMath::IsFinite(CPUms) || CPUms < 0 || CPUms > 250 ||
            !FMath::IsFinite(BudgetMs) || BudgetMs < 5 || BudgetMs > 25)
        {
            Reset();
            return Percentage;
        }
        if (Cooldown > 0) { Cooldown -= DT; return Percentage; }
        Pressure = GPUms > BudgetMs * .9 ? Pressure + DT : 0;
        Headroom = GPUms < BudgetMs * .72 && CPUms < BudgetMs * .8 ? Headroom + DT : 0;
        if (Pressure >= .3 && Percentage > 50)
        {
            Percentage = FMath::Max(50.f, Percentage - 5.f);
            Pressure = Headroom = 0;
            Cooldown = .75;
        }
        else if (Headroom >= 2 && Percentage < 100)
        {
            Percentage = FMath::Min(100.f, Percentage + 2.5f);
            Pressure = Headroom = 0;
            Cooldown = .75;
        }
        return Percentage;
    }
};
