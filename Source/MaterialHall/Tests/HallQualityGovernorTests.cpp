#include "../HallQualityGovernor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallQualityGovernorTest, "MaterialHall.Performance.QualityGovernor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHallQualityGovernorTest::RunTest(const FString& Parameters)
{
    FHallQualityGovernor Governor;
    int32 Quality = 1;
    auto Run = [&](double Seconds, double GPUms, double CPUms, float Resolution, bool Active = true)
    {
        for (int32 Frame = 0; Frame < FMath::RoundToInt(Seconds * 60); ++Frame)
            Quality = Governor.Update(1.0 / 60.0, GPUms, CPUms, Resolution, Quality, Active);
    };
    Run(4, 30, 4, 50);
    TestEqual(TEXT("Startup warmup does not downgrade"), Quality, 1);
    Run(20, 19, 4, 80);
    TestEqual(TEXT("Allow resolution tuning before reducing effects"), Quality, 1);
    Run(1, 30, 4, 50);
    Run(1, 8, 4, 100);
    TestEqual(TEXT("Short spike does not reduce effects"), Quality, 1);
    Run(4, 25, 4, 50);
    TestEqual(TEXT("Sustained GPU pressure reduces effects"), Quality, 0);
    Run(5, 8, 4, 100);
    TestEqual(TEXT("Cooldown prevents immediate bounce"), Quality, 0);
    Run(40, 8, 4, 100);
    TestEqual(TEXT("Sustained spare GPU capacity restores Fine"), Quality, 2);
    Run(30, 8, 4, 100);
    TestEqual(TEXT("Upper quality bound"), Quality, 2);
    Run(30, 25, 4, 50);
    TestEqual(TEXT("Lower quality bound"), Quality, 0);
    Quality = 1;
    Governor.Reset(0);
    Run(30, 8, 4, 100, false);
    TestEqual(TEXT("Background frames cannot upgrade"), Quality, 1);
    Run(30, 0, 4, 100);
    TestEqual(TEXT("Missing GPU timings cannot upgrade"), Quality, 1);
    Governor.Reset(0);
    Run(30, 8, 4, 80);
    TestEqual(TEXT("Recover resolution before upgrading effects"), Quality, 1);
    Run(4, 5, 25, 100);
    TestEqual(TEXT("CPU rendering pressure also reduces effects"), Quality, 0);
    Quality = 1;
    Governor.Reset(0);
    Run(2, 25, 4, 50);
    Quality = Governor.Update(2, 2000, 2000, 50, Quality, true);
    Run(2, 25, 4, 50);
    TestEqual(TEXT("Long stalls reset tuning evidence"), Quality, 1);
    return true;
}
#endif
