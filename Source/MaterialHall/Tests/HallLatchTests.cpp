#include "../HallLatchDynamics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHallLatchTest,"MaterialHall.Mechanism.RSLatch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHallLatchTest::RunTest(const FString& Parameters)
{
    for (double FPS : {30.0,60.0,144.0})
    {
        FHallLatchDynamics D;
        auto Advance=[&](double Seconds)
        {
            for (int32 Frame=0;Frame<FMath::CeilToInt(Seconds*FPS);++Frame)
            {
                D.Update(1/FPS);
                for (int32 I=0;I<2;++I)
                    if (!D.Pose(I).bValid || D.Control[I]<0 || D.Control[I]>1 || D.Feedback[I]<0 || D.Feedback[I]>1)
                        AddError(TEXT("Latch left its continuous reachable configuration space"));
            }
        };
        D.SetControls(false,true); Advance(3);
        TestEqual(TEXT("Set pulse produces Q=1"),D.Bit(0),1);
        D.SetControls(true,true); Advance(30);
        TestTrue(TEXT("Released set remains stored with closed feedback"),D.Bit(0)==1 && D.Bit(1)==0 && D.FeedbackError()<1.e-7);
        D.SetControls(true,false); Advance(3);
        TestEqual(TEXT("Reset pulse produces Q=0"),D.Bit(0),0);
        D.SetControls(true,true); Advance(30);
        TestTrue(TEXT("Released reset remains stored with closed feedback"),D.Bit(0)==0 && D.Bit(1)==1 && D.FeedbackError()<1.e-7);
        // Repeat with a short automatic pulse, and reverse during movement.
        D.SetControls(false,true); Advance(.15); D.SetControls(true,false); Advance(3);
        TestEqual(TEXT("Reversal during a set transition settles reset"),D.Bit(0),0);
        D.SetControls(false,true); Advance(1.6); D.SetControls(true,true); Advance(4);
        TestEqual(TEXT("One UI pulse is sufficient to set memory"),D.Bit(0),1);
        D.SetControls(false,false); Advance(5);
        TestTrue(TEXT("Both controls low gives the forbidden high/high outputs"),D.Bit(0)==1 && D.Bit(1)==1);
        D.SetControls(true,true); Advance(3);
        TestTrue(TEXT("Symmetric release is unresolved, not reported as stored memory"),D.Bit(0)==-1 && D.Bit(1)==-1);
        D.SetControls(true,false); Advance(3); D.SetControls(true,true); Advance(4);
        TestEqual(TEXT("Reset recovers from the invalid/released state"),D.Bit(0),0);
    }
    // Swapping the two halves must only swap the two trajectories.
    FHallLatchDynamics A,B; B.Feedback[0]=0; B.Feedback[1]=1;
    A.SetControls(false,true); B.SetControls(true,false);
    for (int32 I=0;I<1000;++I)
    {
        A.Update(1.0/120); B.Update(1.0/120);
        if (FMath::Abs(A.Signal(0)-B.Signal(1))>1.e-10) { AddError(TEXT("Feedback update has a gate-order bias")); break; }
    }
    return true;
}
#endif
