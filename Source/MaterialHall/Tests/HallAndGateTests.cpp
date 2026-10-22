#include "../HallAndKinematics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallAndTest,"MaterialHall.Mechanism.AndLinkage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHallAndTest::RunTest(const FString& Parameters)
{
    double MaxError=0,MaxSingleDrift=0;
    for (int32 I=0;I<=100;++I) for (int32 J=0;J<=100;++J)
    {
        const double A=I/100.0,B=J/100.0;
        const FHallAndPose P=FHallAndKinematics::Solve(A,B),Swapped=FHallAndKinematics::Solve(B,A);
        if (!P.bValid || !Swapped.bValid) { AddError(TEXT("Unreachable pose in the continuous input square")); return false; }
        MaxError=FMath::Max(MaxError,FMath::Abs(FVector2D::Distance(P.A,P.Coupler)-FHallAndKinematics::InputLinkLength()));
        MaxError=FMath::Max(MaxError,FMath::Abs(FVector2D::Distance(P.B,P.Coupler)-FHallAndKinematics::InputLinkLength()));
        MaxError=FMath::Max(MaxError,FMath::Abs(FVector2D::Distance(P.Output,P.Coupler)-FHallAndKinematics::OutputLinkLength()));
        MaxError=FMath::Max(MaxError,FMath::Abs(P.OutputTravel-Swapped.OutputTravel));
        if (P.OutputTravel < -1.e-9 || P.OutputTravel > 1+1.e-9) { AddError(TEXT("Output left its stroke")); return false; }
        if (J==0) MaxSingleDrift=FMath::Max(MaxSingleDrift,FMath::Abs(P.OutputTravel));
        if (I==J) MaxError=FMath::Max(MaxError,FMath::Abs(P.OutputTravel-A));
    }
    TestTrue(TEXT("Fixed-length joints, input symmetry and equal-input translation"),MaxError<1.e-10);
    TestTrue(TEXT("A single advancing input causes less than 8% transient output travel"),MaxSingleDrift<.08);
    for (int32 State=0;State<4;++State)
    {
        const FHallAndPose P=FHallAndKinematics::Solve(State/2,State%2);
        TestTrue(FString::Printf(TEXT("Truth table %d%d"),State/2,State%2),FMath::Abs(P.OutputTravel-(State==3 ? 1 : 0))<1.e-10);
    }
    TestFalse(TEXT("Reject nonphysical inputs"),FHallAndKinematics::Solve(-.1,0).bValid);
    for (double FPS : {30.0,60.0,90.0,120.0})
    {
        FHallRodMotion Motion;
        for (int32 Frame=0;Frame<2.5*FPS;++Frame) Motion.Update(1,1/FPS);
        TestEqual(TEXT("Animation settles exactly at one"),Motion.Position,1.0);
        for (int32 Frame=0;Frame<FPS/5;++Frame) Motion.Update(0,1/FPS);
        for (int32 Frame=0;Frame<2.5*FPS;++Frame) Motion.Update(1,1/FPS);
        TestEqual(TEXT("Mid-move reversal still settles"),Motion.Position,1.0);
        for (int32 Frame=0;Frame<2.5*FPS;++Frame) Motion.Update(0,1/FPS);
        TestEqual(TEXT("Animation settles exactly at zero"),Motion.Position,0.0);
    }
    return true;
}
#endif
