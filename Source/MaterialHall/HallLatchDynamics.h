#pragma once
#include "HallAndKinematics.h"

// Continuous, overdamped feedback followers. Only geometry drives the outputs;
// no stored Boolean chooses either of the two stable hold configurations.
// The rendered spring couplers expose follower compliance during transitions.
struct FHallLatchDynamics
{
    double Control[2]={1,1};
    double Feedback[2]={1,0}; // Power-on demonstration starts with Q=0.
    bool Target[2]={true,true};
    void SetControls(bool SBar,bool RBar) { Target[0]=SBar; Target[1]=RBar; }
    FHallAndPose Pose(int32 Index) const { return FHallAndKinematics::Solve(Control[Index],Feedback[Index]); }
    double Signal(int32 Index) const { return 1-Pose(Index).OutputTravel; }
    double FeedbackError() const
    {
        return FMath::Max(FMath::Abs(Feedback[0]-Signal(1)),FMath::Abs(Feedback[1]-Signal(0)));
    }
    bool Settled() const
    {
        return FeedbackError()<1.e-4 && FMath::Abs(Control[0]-double(Target[0]))<1.e-4 && FMath::Abs(Control[1]-double(Target[1]))<1.e-4;
    }
    // A logic value is only reported inside an end zone. Intermediate positions
    // after simultaneous release are explicitly unresolved, not a random bit.
    int32 Bit(int32 Index) const { const double Q=Signal(Index); return Q<.02 ? 0 : Q>.98 ? 1 : -1; }
    void Update(double DT)
    {
        if (!FMath::IsFinite(DT) || DT<=0) return;
        double Remaining=FMath::Min(DT,.25);
        while (Remaining>1.e-9)
        {
            const double Step=FMath::Min(Remaining,1.0/240.0);
            const double Alpha=1-FMath::Exp(-10*Step);
            // Read both old outputs before updating either side: no order bias.
            const double Next[]={FMath::Clamp(Signal(1),0.0,1.0),FMath::Clamp(Signal(0),0.0,1.0)};
            for (int32 I=0;I<2;++I)
            {
                Control[I]+=Alpha*(double(Target[I])-Control[I]);
                Feedback[I]+=Alpha*(Next[I]-Feedback[I]);
                if (FMath::Abs(Control[I]-double(Target[I]))<1.e-12) Control[I]=double(Target[I]);
            }
            Remaining-=Step;
        }
    }
};
