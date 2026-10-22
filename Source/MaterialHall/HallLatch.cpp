#include "HallLatch.h"
#include "HallAndGate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace { constexpr double GateScale=.64; }

AHallLatch::AHallLatch()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("LatchAssembly"));
    for (const TCHAR* Name : {TEXT("AndBase"),TEXT("AndBlock"),TEXT("AndPin"),TEXT("AndWasher"),TEXT("AndBolt"),TEXT("LatchSpring")})
    {
        ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(*(FString(TEXT("/Game/MaterialHall/AndGate/"))+Name+TEXT(".")+Name));
        Meshes.Add(Name,Asset.Object);
    }
    for (const TCHAR* Name : {TEXT("GateBase"),TEXT("GateSteel"),TEXT("GateBrass"),TEXT("GateInputA"),TEXT("GateInputB"),TEXT("GateOutput"),TEXT("GateIvory")})
    {
        ConstructorHelpers::FObjectFinder<UMaterialInterface> Asset(*(FString(TEXT("/Game/MaterialHall/AndGate/"))+Name+TEXT(".")+Name));
        Finishes.Add(Name,Asset.Object);
    }
}
UStaticMeshComponent* AHallLatch::Part(const FString& Name,const TCHAR* Mesh,const TCHAR* Finish,FVector Position,FVector Scale)
{
    auto* C=NewObject<UStaticMeshComponent>(this,*Name);
    C->SetupAttachment(RootComponent); C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(Meshes.FindChecked(Mesh)); C->SetMaterial(0,Finishes.FindChecked(Finish));
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetRelativeLocation(Position); C->SetRelativeScale3D(Scale); C->RegisterComponent(); return C;
}
UTextRenderComponent* AHallLatch::Label(const FString& Name,const FString& Text,FVector Position,float Size)
{
    auto* C=NewObject<UTextRenderComponent>(this,*Name); C->SetupAttachment(RootComponent);
    C->SetRelativeLocationAndRotation(Position,FRotator(90,90,0)); C->SetWorldSize(Size);
    C->SetHorizontalAlignment(EHTA_Center); C->SetVerticalAlignment(EVRTA_TextCenter);
    C->SetTextRenderColor(FColor(8,14,18)); C->SetText(FText::FromString(Text)); C->RegisterComponent(); return C;
}
void AHallLatch::BeginPlay()
{
    Super::BeginPlay();
    Part(TEXT("Bedplate"),TEXT("AndBase"),TEXT("GateBase"),FVector(0,0,1));
    for (int32 I=0;I<2;++I)
    {
        const FTransform Local(FRotator(0,I ? 180 : 0,0),FVector(I ? 32 : -32,I ? 5.76 : -5.76,.72),FVector(GateScale));
        AHallAndGate* Gate=GetWorld()->SpawnActorDeferred<AHallAndGate>(AHallAndGate::StaticClass(),Local*GetActorTransform(),this);
        Gate->ConfigureEmbedded(); Gate->FinishSpawning(Local*GetActorTransform());
        Gate->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform); Gate->SetActorTickEnabled(false);
        Gate->SetExternalColor(I==1);
        Gate->OnInputRequested.AddLambda([this,I](int32){ToggleControl(I);}); Gates.Add(Gate);
        Springs.Add(Part(FString::Printf(TEXT("FeedbackSpring%d"),I),TEXT("LatchSpring"),I ? TEXT("GateInputB") : TEXT("GateInputA"),FVector::ZeroVector));
        Cores.Add(Part(FString::Printf(TEXT("FeedbackCore%d"),I),TEXT("AndPin"),TEXT("GateSteel"),FVector::ZeroVector));
        Cores.Last()->SetRelativeRotation(FRotator(90,0,0));
        for (int32 End=0;End<2;++End)
        {
            Collars.Add(Part(FString::Printf(TEXT("CouplingCollar%d%d"),I,End),TEXT("AndWasher"),TEXT("GateSteel"),FVector::ZeroVector,FVector(1.25,1.25,2)));
            Collars.Last()->SetRelativeRotation(FRotator(90,0,0));
        }
    }
    const FVector Plates[]={FVector(-89,-29,2.2),FVector(89,29,2.2),FVector(89,-23,2.2),FVector(-89,23,2.2)};
    for (int32 I=0;I<4;++I)
    {
        Part(FString::Printf(TEXT("SignalPlate%d"),I),TEXT("AndBlock"),TEXT("GateIvory"),Plates[I],FVector(45,9,.4));
        StateLabels.Add(Label(FString::Printf(TEXT("SignalText%d"),I),TEXT(""),Plates[I]+FVector(0,0,.25),3.2f));
    }
    Part(TEXT("TitlePlate"),TEXT("AndBlock"),TEXT("GateIvory"),FVector(-55,43,2.2),FVector(112,10,.4));
    Label(TEXT("Title"),TEXT("02   /   ROD LOGIC   /   RS LATCH"),FVector(-55,43,2.45),3.4f);
    Part(TEXT("StatusPlate"),TEXT("AndBlock"),TEXT("GateIvory"),FVector(61,43,2.2),FVector(106,10,.4));
    StateLabels.Add(Label(TEXT("State"),TEXT(""),FVector(61,43,2.45),3.3f));
    Part(TEXT("NotePlate"),TEXT("AndBlock"),TEXT("GateIvory"),FVector(0,-43,2.2),FVector(170,8,.4));
    Label(TEXT("Note"),TEXT("FACING GATES   /   INVERTED SPRING FEEDBACK   /   1 = HOLD"),FVector(0,-43,2.45),2.65f);
    for (int32 X : {-1,1}) for (int32 Y : {-1,1})
        Part(FString::Printf(TEXT("BaseScrew%d%d"),X,Y),TEXT("AndBolt"),TEXT("GateSteel"),FVector(X*116,Y*48,2.3),FVector(.75));
    UpdateAssembly();
}
FVector AHallLatch::GatePoint(int32 I,FVector Local) const { return GetActorTransform().InverseTransformPosition(Gates[I]->GetActorTransform().TransformPosition(Local)); }
void AHallLatch::SetControls(bool SBar,bool RBar) { Dynamics.SetControls(SBar,RBar); PulseTime=0; bCycle=false; }
void AHallLatch::Pulse(int32 I) { SetControls(I!=0,I==0); PulseTime=1.6; }
void AHallLatch::ToggleControl(int32 I) { SetControls(I==0 ? !Dynamics.Target[0] : Dynamics.Target[0],I==1 ? !Dynamics.Target[1] : Dynamics.Target[1]); }
void AHallLatch::ToggleCycle() { bCycle=!bCycle; CycleTime=0; CycleStep=0; PulseTime=0; }
void AHallLatch::ResetDemo() { bCycle=false; Pulse(1); }
void AHallLatch::SetDemoVisible(bool Visible)
{
    SetActorHiddenInGame(!Visible); SetActorTickEnabled(Visible);
    for (AHallAndGate* Gate : Gates) Gate->SetDemoVisible(Visible);
}
void AHallLatch::SetFinish(float Roughness) { for (AHallAndGate* Gate : Gates) Gate->SetFinish(Roughness); }
FString AHallLatch::Status() const
{
    if (!Dynamics.Target[0] && !Dynamics.Target[1]) return TEXT("INVALID / both controls low");
    if (!Dynamics.Settled()) return TEXT("Settling feedback...");
    if (Dynamics.Bit(0)<0 || Dynamics.Bit(1)<0 || Dynamics.Bit(0)==Dynamics.Bit(1)) return TEXT("UNRESOLVED / pulse Set or Reset");
    if (!Dynamics.Target[0]) return TEXT("SET / release to hold");
    if (!Dynamics.Target[1]) return TEXT("RESET / release to hold");
    return FString::Printf(TEXT("HOLD / stored %d"),Dynamics.Bit(0));
}
void AHallLatch::Tick(float DT)
{
    Super::Tick(DT);
    if (PulseTime>0) { PulseTime-=FMath::Min(DT,.25f); if (PulseTime<=0) Dynamics.SetControls(true,true); }
    if (bCycle)
    {
        CycleTime+=FMath::Min(DT,.25f);
        if (CycleTime>=3.5) { CycleTime=0; CycleStep=(CycleStep+1)%4; Dynamics.SetControls(CycleStep!=1,CycleStep!=3); }
    }
    Dynamics.Update(DT); UpdateAssembly();
}
void AHallLatch::UpdateAssembly()
{
    for (int32 I=0;I<2;++I) Gates[I]->SetDrivenPose(Dynamics.Control[I],Dynamics.Feedback[I]);
    for (int32 I=0;I<2;++I)
    {
        const FVector P=GatePoint(I,FVector(Dynamics.Pose(I).Output.X+64,0,8));
        const FVector Q=GatePoint(1-I,FVector(Dynamics.Pose(1-I).B.X,18,8));
        const double Length=FMath::Abs(Q.X-P.X);
        Springs[I]->SetRelativeLocation((P+Q)*.5); Springs[I]->SetRelativeScale3D(FVector(Length/20,GateScale,GateScale));
        Cores[I]->SetRelativeLocation((P+Q)*.5); Cores[I]->SetRelativeScale3D(FVector(.6,.6,Length/10));
        Collars[I*2]->SetRelativeLocation(P); Collars[I*2+1]->SetRelativeLocation(Q);
    }
    StateLabels[0]->SetText(FText::FromString(FString::Printf(TEXT("SET  /  S' %d"),Dynamics.Target[0] ? 1 : 0)));
    StateLabels[1]->SetText(FText::FromString(FString::Printf(TEXT("RESET  /  R' %d"),Dynamics.Target[1] ? 1 : 0)));
    for (int32 I=0;I<2;++I)
    {
        const int32 Bit=Dynamics.Bit(I);
        StateLabels[I+2]->SetText(FText::FromString(FString::Printf(TEXT("%s   %s"),I ? TEXT("Q'") : TEXT("Q"),Bit<0 ? TEXT("?") : Bit ? TEXT("1") : TEXT("0"))));
    }
    StateLabels[4]->SetText(FText::FromString(Status()));
}
bool AHallLatch::CheckAssembly(double& JointError,double& ConnectionError) const
{
    JointError=0; ConnectionError=0; bool Valid=true;
    for (int32 I=0;I<2;++I)
    {
        double Error=0; Valid &= Gates[I]->CheckVisualConstraints(Error); JointError=FMath::Max(JointError,Error);
        const FVector P=GatePoint(I,FVector(Dynamics.Pose(I).Output.X+64,0,8));
        const FVector Q=GatePoint(1-I,FVector(Dynamics.Pose(1-I).B.X,18,8));
        const FTransform T=Springs[I]->GetRelativeTransform();
        const FVector Left=T.TransformPosition(FVector(-10,0,0)),Right=T.TransformPosition(FVector(10,0,0));
        ConnectionError=FMath::Max(ConnectionError,FMath::Min(FVector::Dist(Left,P)+FVector::Dist(Right,Q),FVector::Dist(Right,P)+FVector::Dist(Left,Q)));
        Valid &= FMath::Abs(P.Y-Q.Y)<.001 && FMath::Abs(P.Z-Q.Z)<.001;
    }
    return Valid && ConnectionError<.001;
}
