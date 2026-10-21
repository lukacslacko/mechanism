#include "../HallVRResolutionGovernor.h"
#include "../HallQualityGovernor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallVRResolutionTest, "MaterialHall.Performance.VRResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHallVRResolutionTest::RunTest(const FString& Parameters)
{
    for (double Hz : {72.0, 80.0, 90.0, 120.0})
    {
        FHallVRResolutionGovernor Resolution;
        FHallQualityGovernor Effects;
        int32 Quality = 1;
        const double Budget = 1000.0 / Hz;
        auto Run = [&](double Seconds, double GPU, double CPU, bool Active = true)
        {
            for (int32 Frame = 0; Frame < FMath::CeilToInt(Seconds*Hz); ++Frame)
            {
                Resolution.Update(1.0/Hz, GPU, CPU, Budget, Active);
                Quality = Effects.Update(1.0/Hz, GPU * (1000.0/60) / Budget,
                    CPU * (1000.0/60) / Budget, Resolution.Percentage, Quality, Active);
            }
        };
        Run(2, Budget*2, 3);
        TestEqual(TEXT("Warmup keeps initial resolution"), Resolution.Percentage, 80.f);
        Run(25, Budget*2, 3);
        TestEqual(TEXT("Sustained GPU overload reaches bounded resolution floor"), Resolution.Percentage, 50.f);
        TestEqual(TEXT("Effects reduce after resolution reaches its floor"), Quality, 0);
        Run(120, Budget*.35, 2);
        TestEqual(TEXT("Spare GPU capacity restores full resolution"), Resolution.Percentage, 100.f);
        TestEqual(TEXT("Spare GPU capacity restores Fine effects"), Quality, 2);
        Resolution.Percentage = 80;
        Run(30, 1, 1, false);
        TestEqual(TEXT("Unfocused headset cannot upgrade quality"), Resolution.Percentage, 80.f);
        Run(30, 0, 1);
        TestEqual(TEXT("Missing GPU timings cannot upgrade quality"), Resolution.Percentage, 80.f);
        Run(30, 1, Budget*2);
        TestEqual(TEXT("CPU bottleneck does not trigger resolution upgrades"), Resolution.Percentage, 80.f);
        Resolution.Reset();
        Run(4, Budget*.8, 1);
        Run(.15, Budget*2, 1);
        Run(1, Budget*.8, 1);
        TestEqual(TEXT("Short GPU spike does not cause a resolution step"), Resolution.Percentage, 80.f);
        Resolution.Update(2, 100, 100, Budget, true);
        TestEqual(TEXT("Stalls do not change resolution"), Resolution.Percentage, 80.f);
    }
    return true;
}
#endif
