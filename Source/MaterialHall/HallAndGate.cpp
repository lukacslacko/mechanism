#include "HallAndGate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AHallAndGate::AHallAndGate()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Assembly"));
    const TCHAR* Parts[] = {TEXT("AndBase"),TEXT("AndBlock"),TEXT("AndInputLink"),TEXT("AndOutputLink"),TEXT("AndInputSlider"),
        TEXT("AndOutputSlider"),TEXT("AndGuide"),TEXT("AndPin"),TEXT("AndWasher"),TEXT("AndBolt"),TEXT("AndHandle")};
    for (const TCHAR* Name : Parts)
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

UStaticMeshComponent* AHallAndGate::Part(const TCHAR* Name, const TCHAR* Mesh, const TCHAR* Finish, FVector Location, FVector Scale, USceneComponent* Parent)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this,Name);
    Component->SetupAttachment(Parent ? Parent : RootComponent.Get());
    Component->SetStaticMesh(Meshes.FindChecked(Mesh));
    Component->SetMaterial(0,Finishes.FindChecked(Finish));
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeScale3D(Scale);
    Component->RegisterComponent();
    return Component;
}

UTextRenderComponent* AHallAndGate::Label(const TCHAR* Name, const FString& Text, FVector Location, float Size)
{
    UTextRenderComponent* Component = NewObject<UTextRenderComponent>(this,Name);
    Component->SetupAttachment(RootComponent);
    Component->SetText(FText::FromString(Text));
    Component->SetTextRenderColor(FColor(8,14,18));
    Component->SetWorldSize(Size);
    Component->SetHorizontalAlignment(EHTA_Center);
    Component->SetVerticalAlignment(EVRTA_TextCenter);
    Component->SetRelativeLocationAndRotation(Location,FRotator(90,90,0));
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->RegisterComponent();
    return Component;
}

USceneComponent* AHallAndGate::Joint(const TCHAR* Name, double Bottom, double Top)
{
    USceneComponent* Root = NewObject<USceneComponent>(this,Name);
    Root->SetupAttachment(RootComponent);
    Root->RegisterComponent();
    Part(*(FString(Name)+TEXT("Pin")),TEXT("AndPin"),TEXT("GateSteel"),FVector(0,0,(Bottom+Top)/2),FVector(1,1,(Top-Bottom)/10),Root);
    Part(*(FString(Name)+TEXT("Cap")),TEXT("AndBolt"),TEXT("GateSteel"),FVector(0,0,Top+.3),FVector::OneVector,Root);
    Part(*(FString(Name)+TEXT("Washer")),TEXT("AndWasher"),TEXT("GateBrass"),FVector(0,0,Top-.25),FVector::OneVector,Root);
    return Root;
}

void AHallAndGate::BeginPlay()
{
    Super::BeginPlay();
    for (const TCHAR* Name : {TEXT("GateSteel"),TEXT("GateBrass")})
        Finishes[Name]=UMaterialInstanceDynamic::Create(Finishes.FindChecked(Name),this);
    if (!bEmbedded) Part(TEXT("Bedplate"),TEXT("AndBase"),TEXT("GateBase"),FVector(0,0,1));
    for (int32 Index=0;Index<2;++Index)
    {
        const double Y = Index ? 18 : -18;
        const TCHAR* Color = Index ? TEXT("GateInputB") : TEXT("GateInputA");
        for (int32 Bearing=0;Bearing<2;++Bearing)
        {
            const double X = Bearing ? -60 : -73;
            Part(*FString::Printf(TEXT("Input%dGuide%d"),Index,Bearing),TEXT("AndGuide"),TEXT("GateSteel"),FVector(X,Y,8));
            for (int32 Side : {-1,1})
                Part(*FString::Printf(TEXT("Input%dGuide%dScrew%d"),Index,Bearing,Side),TEXT("AndBolt"),TEXT("GateBase"),FVector(X,Y+Side*3.2,14.3),FVector(.55));
        }
        Sliders.Add(Part(*FString::Printf(TEXT("Input%dSlider"),Index),TEXT("AndInputSlider"),TEXT("GateBrass"),FVector::ZeroVector));
        UStaticMeshComponent* Handle = Part(*FString::Printf(TEXT("Input%dHandle"),Index),TEXT("AndHandle"),Color,FVector::ZeroVector);
        Handle->SetRelativeRotation(FRotator(90,0,0));
        Handle->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Handle->SetCollisionResponseToAllChannels(ECR_Ignore);
        Handle->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        // The handle's simple cylinder collision is generated during import.
        Handle->OnClicked.AddDynamic(this,&AHallAndGate::HandleClicked);
        Handles.Add(Handle);
        Links.Add(Part(*FString::Printf(TEXT("Input%dLink"),Index),TEXT("AndInputLink"),TEXT("GateBrass"),FVector::ZeroVector));
        Joints.Add(Joint(*FString::Printf(TEXT("Input%dJoint"),Index),6.5,Index ? 14.3 : 12.3));
        Part(*FString::Printf(TEXT("Input%dSpacer"),Index),TEXT("AndWasher"),TEXT("GateSteel"),FVector(0,0,Index ? 10.9 : 9.9),FVector(1,1,Index ? 5.5 : 1),Joints.Last());
        if (!bEmbedded)
        {
        const double LabelY=Index ? 7 : -7;
        Part(*FString::Printf(TEXT("Input%dNameplate"),Index),TEXT("AndBlock"),TEXT("GateIvory"),FVector(-98,LabelY,2.2),FVector(26,8,.4));
        StateLabels.Add(Label(*FString::Printf(TEXT("Input%dLabel"),Index),Index ? TEXT("B  /  0") : TEXT("A  /  0"),FVector(-98,LabelY,2.45),3.7f));
        const double ScaleY = Index ? Y+12 : Y-12;
        Part(*FString::Printf(TEXT("Input%dScale"),Index),TEXT("AndBlock"),TEXT("GateSteel"),FVector(-97.9,ScaleY,2.08),FVector(36,.22,.12));
        for (int32 Tick=0;Tick<=10;++Tick)
            Part(*FString::Printf(TEXT("Input%dTick%d"),Index,Tick),TEXT("AndBlock"),Color,FVector(-115+Tick*3.42,ScaleY,2.15),FVector(.35,Tick==0||Tick==10 ? 3 : 1.4,.2));
        Label(*FString::Printf(TEXT("Input%dZero"),Index),TEXT("0"),FVector(-115,ScaleY+(Index ? 4 : -4),2.3),2.8f)->SetTextRenderColor(FColor(210,210,190));
        Label(*FString::Printf(TEXT("Input%dOne"),Index),TEXT("1"),FVector(-80.8,ScaleY+(Index ? 4 : -4),2.3),2.8f)->SetTextRenderColor(FColor(210,210,190));
        }
    }
    Sliders.Add(Part(TEXT("OutputSlider"),TEXT("AndOutputSlider"),TEXT("GateBrass"),FVector::ZeroVector));
    Links.Add(Part(TEXT("OutputLink"),TEXT("AndOutputLink"),TEXT("GateSteel"),FVector::ZeroVector));
    Handles.Add(Part(TEXT("OutputHandle"),TEXT("AndHandle"),TEXT("GateOutput"),FVector::ZeroVector));
    Handles.Last()->SetRelativeRotation(FRotator(90,0,0));
    Joints.Add(Joint(TEXT("CouplerJoint"),9.7,16.3));
    Joints.Add(Joint(TEXT("OutputJoint"),6.5,16.3));
    Part(TEXT("OutputSpacer"),TEXT("AndWasher"),TEXT("GateSteel"),FVector(0,0,11.9),FVector(1,1,10.5),Joints.Last());
    for (int32 Layer=0;Layer<2;++Layer)
        Part(*FString::Printf(TEXT("CouplerSpacer%d"),Layer),TEXT("AndWasher"),TEXT("GateSteel"),FVector(0,0,12+2*Layer),FVector::OneVector,Joints[2]);
    for (int32 Index=0;Index<2;++Index)
        Part(*FString::Printf(TEXT("OutputGuide%d"),Index),TEXT("AndGuide"),TEXT("GateSteel"),FVector(Index ? 71 : 57,0,8));
    if (!bEmbedded)
    {
    Part(TEXT("OutputNameplate"),TEXT("AndBlock"),TEXT("GateIvory"),FVector(92,26,2.2),FVector(39,12,.4));
    StateLabels.Add(Label(TEXT("OutputLabel"),TEXT("C  /  0"),FVector(92,26,2.45),4.2f));
    Part(TEXT("OutputScale"),TEXT("AndBlock"),TEXT("GateSteel"),FVector(95.33,-13,2.1),FVector(36,.25,.15));
    for (int32 Tick=0;Tick<=10;++Tick)
        Part(*FString::Printf(TEXT("OutputTick%d"),Tick),TEXT("AndBlock"),TEXT("GateOutput"),FVector(78.234+Tick*3.42,-13,2.2),FVector(.35,Tick==0||Tick==10 ? 3 : 1.4,.2));
    Label(TEXT("OutputZero"),TEXT("0"),FVector(78.234,-18,2.3),2.8f)->SetTextRenderColor(FColor(210,210,190));
    Label(TEXT("OutputOne"),TEXT("1"),FVector(112.434,-18,2.3),2.8f)->SetTextRenderColor(FColor(210,210,190));
    Part(TEXT("TitlePlate"),TEXT("AndBlock"),TEXT("GateIvory"),FVector(-68,43,2.2),FVector(88,10,.4));
    Label(TEXT("Title"),TEXT("01   /   ROD LOGIC   /   AND"),FVector(-68,43,2.45),3.5);
    for (int32 State=0;State<4;++State)
    {
        const double X = -9+State*34;
        Part(*FString::Printf(TEXT("TruthPlate%d"),State),TEXT("AndBlock"),TEXT("GateIvory"),FVector(X,43,2.2),FVector(29,10,.4));
        Label(*FString::Printf(TEXT("TruthLabel%d"),State),FString::Printf(TEXT("%d %d : %d"),State/2,State%2,State==3 ? 1 : 0),FVector(X,43,2.45),3.2f);
        TruthMarkers.Add(Part(*FString::Printf(TEXT("TruthIndicator%d"),State),TEXT("AndBlock"),TEXT("GateOutput"),FVector(X,49,2.2),FVector(26,.7,.3)));
    }
    for (int32 X : {-1,1}) for (int32 Y : {-1,1})
        Part(*FString::Printf(TEXT("BaseScrew%d%d"),X,Y),TEXT("AndBolt"),TEXT("GateSteel"),FVector(X*116,Y*48,2.3),FVector(.75));
    }
    else
    {
        Handles[1]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Handles[1]->SetMaterial(0,Finishes.FindChecked(TEXT("GateOutput")));
        Handles[2]->SetVisibility(false);
    }
    UpdatePose();
}

void AHallAndGate::SetInputs(bool A, bool B) { Targets[0]=A; Targets[1]=B; bCycle=false; CycleTime=0; }
void AHallAndGate::ToggleInput(int32 Index)
{
    Index=Index ? 1 : 0;
    if (bEmbedded) { if (Index==0) OnInputRequested.Broadcast(Index); return; }
    SetInputs(Index==0 ? !Targets[0] : Targets[0],Index==1 ? !Targets[1] : Targets[1]);
}
void AHallAndGate::SetDrivenPose(double A,double B)
{
    Motion[0].Position=FMath::Clamp(A,0.0,1.0); Motion[1].Position=FMath::Clamp(B,0.0,1.0);
    UpdatePose();
}
void AHallAndGate::SetExternalColor(bool bOrange) { Handles[0]->SetMaterial(0,Finishes.FindChecked(bOrange ? TEXT("GateInputB") : TEXT("GateInputA"))); }
void AHallAndGate::ResetGate() { SetInputs(false,false); }
void AHallAndGate::SetFinish(float Roughness)
{
    for (const TCHAR* Name : {TEXT("GateSteel"),TEXT("GateBrass")})
        if (UMaterialInstanceDynamic* Material=Cast<UMaterialInstanceDynamic>(Finishes.FindChecked(Name)))
            Material->SetScalarParameterValue(TEXT("Roughness"),Roughness);
}
void AHallAndGate::ToggleCycle() { bCycle=!bCycle; CycleTime=0; CycleIndex=0; }
bool AHallAndGate::IsSettled() const { return Motion[0].Position==double(Targets[0]) && Motion[1].Position==double(Targets[1]); }
void AHallAndGate::SetDemoVisible(bool bVisible)
{
    bDemoVisible=bVisible;
    SetActorHiddenInGame(!bVisible);
    SetActorEnableCollision(bVisible);
    SetActorTickEnabled(bVisible && !bEmbedded);
}
bool AHallAndGate::ActivateHandle(UPrimitiveComponent* Component)
{
    if (!bDemoVisible) return false;
    for (int32 Index=0;Index<2;++Index) if (Component==Handles[Index]) { ToggleInput(Index); return true; }
    return false;
}
void AHallAndGate::HandleClicked(UPrimitiveComponent* Component,FKey Button) { if (Button==EKeys::LeftMouseButton) ActivateHandle(Component); }

void AHallAndGate::Tick(float DT)
{
    Super::Tick(DT);
    if (bCycle)
    {
        CycleTime+=FMath::Min(DT,.1f);
        if (CycleTime>=3.5)
        {
            CycleTime=0;
            const int32 States[]={0,2,3,1};
            CycleIndex=(CycleIndex+1)%4;
            Targets[0]=(States[CycleIndex]&2)!=0;
            Targets[1]=(States[CycleIndex]&1)!=0;
        }
    }
    Motion[0].Update(Targets[0] ? 1 : 0,DT);
    Motion[1].Update(Targets[1] ? 1 : 0,DT);
    UpdatePose();
}

void AHallAndGate::UpdatePose()
{
    Pose=FHallAndKinematics::Solve(Motion[0].Position,Motion[1].Position);
    if (!Pose.bValid) return;
    auto At=[](FVector2D XY,double Z) { return FVector(XY.X,XY.Y,Z); };
    const FVector2D Ends[]={Pose.A,Pose.B,Pose.Output};
    for (int32 Index=0;Index<3;++Index)
    {
        Sliders[Index]->SetRelativeLocation(At(Ends[Index],8));
        Handles[Index]->SetRelativeLocation(At(Ends[Index]+FVector2D(Index==2 ? 64 : -65,0),8));
        const double Height=11+Index*2;
        Links[Index]->SetRelativeLocation(At((Ends[Index]+Pose.Coupler)*.5,Height));
        Links[Index]->SetRelativeRotation(FRotator(0,(At(Pose.Coupler-Ends[Index],0)).Rotation().Yaw,0));
    }
    Joints[0]->SetRelativeLocation(At(Pose.A,0));
    Joints[1]->SetRelativeLocation(At(Pose.B,0));
    Joints[2]->SetRelativeLocation(At(Pose.Coupler,0));
    Joints[3]->SetRelativeLocation(At(Pose.Output,0));
    if (bEmbedded) return;
    StateLabels[0]->SetText(FText::FromString(FString::Printf(TEXT("A  /  %d"),Targets[0] ? 1 : 0)));
    StateLabels[1]->SetText(FText::FromString(FString::Printf(TEXT("B  /  %d"),Targets[1] ? 1 : 0)));
    StateLabels[2]->SetText(FText::FromString(FString::Printf(TEXT("C  /  %d"),OutputBit() ? 1 : 0)));
    const int32 State=(Targets[0] ? 2 : 0)+(Targets[1] ? 1 : 0);
    for (int32 Index=0;Index<4;++Index) TruthMarkers[Index]->SetVisibility(IsSettled() && Index==State);
}

bool AHallAndGate::CheckVisualConstraints(double& MaxError) const
{
    if (!Pose.bValid || Links.Num()!=3 || Sliders.Num()!=3) return false;
    const FVector2D Ends[]={Pose.A,Pose.B,Pose.Output};
    MaxError=0;
    for (int32 Index=0;Index<3;++Index)
    {
        const double Length=Index==2 ? FHallAndKinematics::OutputLinkLength() : FHallAndKinematics::InputLinkLength();
        const FTransform Transform=Links[Index]->GetRelativeTransform();
        const FVector P=Transform.TransformPosition(FVector(-Length/2,0,0));
        const FVector Q=Transform.TransformPosition(FVector(Length/2,0,0));
        MaxError=FMath::Max(MaxError,FVector2D::Distance(FVector2D(P),Ends[Index]));
        MaxError=FMath::Max(MaxError,FVector2D::Distance(FVector2D(Q),Pose.Coupler));
        MaxError=FMath::Max(MaxError,FMath::Abs(Sliders[Index]->GetRelativeLocation().Y-Ends[Index].Y));
    }
    return MaxError<.001;
}
