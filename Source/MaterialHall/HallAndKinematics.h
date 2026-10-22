#pragma once
#include "CoreMinimal.h"

// Centimetres in the XY mechanism plane. Z separates the three links on their
// common vertical pin; the planar pivot-to-pivot lengths remain constant.
struct FHallAndPose
{
    FVector2D A, B, Coupler, Output;
    double OutputTravel = 0;
    bool bValid = false;
};

struct FHallAndKinematics
{
    static constexpr double HalfSpacing = 18;
    static constexpr double Stroke = 34.2;
    static constexpr double RestReach = 30.6;
    static constexpr double InputRestX = -50;

    static double InputLinkLength() { return FMath::Sqrt(HalfSpacing*HalfSpacing + RestReach*RestReach); }
    static double OutputLinkLength()
    {
        const double D = FMath::Sqrt(4*HalfSpacing*HalfSpacing + Stroke*Stroke);
        const double H = FMath::Sqrt(FMath::Square(InputLinkLength()) - D*D/4);
        const double X = 2*HalfSpacing*H/D;
        const double Y = Stroke*H/D;
        const double K = Stroke/2 + X - RestReach;
        // Choose M so sqrt(M^2-Y^2) = M-K: the mixed input states
        // produce exactly the same output position as 00, without a logic clamp.
        return (Y*Y + K*K)/(2*K);
    }
    static double OutputRestX() { return InputRestX + RestReach + OutputLinkLength(); }
    static FHallAndPose Solve(double A, double B)
    {
        FHallAndPose Pose;
        if (!FMath::IsFinite(A) || !FMath::IsFinite(B) || A<0 || A>1 || B<0 || B>1) return Pose;
        Pose.A = FVector2D(InputRestX + A*Stroke, -HalfSpacing);
        Pose.B = FVector2D(InputRestX + B*Stroke, HalfSpacing);
        const FVector2D Delta = Pose.B-Pose.A;
        const double D = Delta.Size();
        const double HeightSquared = FMath::Square(InputLinkLength())-D*D/4;
        if (D<=0 || HeightSquared<=0) return Pose;
        const FVector2D RightNormal(Delta.Y/D, -Delta.X/D);
        Pose.Coupler = (Pose.A+Pose.B)*.5 + RightNormal*FMath::Sqrt(HeightSquared);
        const double RunSquared = FMath::Square(OutputLinkLength())-FMath::Square(Pose.Coupler.Y);
        if (RunSquared<=0) return Pose;
        Pose.Output = FVector2D(Pose.Coupler.X+FMath::Sqrt(RunSquared), 0);
        Pose.OutputTravel = (Pose.Output.X-OutputRestX())/Stroke;
        Pose.bValid = true;
        return Pose;
    }
};

struct FHallRodMotion
{
    double Position = 0;
    double Velocity = 0;
    void Update(double Target, double DT)
    {
        if (!FMath::IsFinite(DT) || DT<=0) return;
        DT = FMath::Min(DT, .1);
        constexpr double Omega = 8;
        const double Error = Position-Target;
        const double C = Velocity+Omega*Error;
        const double Decay = FMath::Exp(-Omega*DT);
        Position = Target+(Error+C*DT)*Decay;
        Velocity = (Velocity-Omega*C*DT)*Decay;
        if (Position<0 || Position>1) { Position = FMath::Clamp(Position,0.0,1.0); Velocity = 0; }
        if (FMath::Abs(Position-Target)<.0001 && FMath::Abs(Velocity)<.001) { Position = Target; Velocity = 0; }
    }
};
