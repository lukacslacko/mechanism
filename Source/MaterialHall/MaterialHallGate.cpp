#include "MaterialHall.h"
#include "HallAndGate.h"
#include "HallLatch.h"
#include "HallVRPawn.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void AMaterialHallController::SelectDemo(bool bGate)
{
    bGateDemo=bGate;
    bLatchDemo=false;
    if (Latch) Latch->SetDemoVisible(false);
    if (AndGate) AndGate->SetDemoVisible(bGate);
    if (VRPawn) VRPawn->GetPanel()->SetDrawSize(FVector2D(900,bGate ? 1280 : 980));
    for (AStaticMeshActor* Actor : {BrassRod.Get(),SteelPlate.Get(),PivotPin.Get(),PivotCollar.Get()})
        if (Actor) { Actor->SetActorHiddenInGame(bGate); Actor->SetActorEnableCollision(!bGate); }
    SetView(0);
}
void AMaterialHallController::ToggleGateInput(int32 Index) { if (bLatchDemo && Latch) Latch->ToggleControl(Index); else if (bGateDemo && AndGate) AndGate->ToggleInput(Index); }
bool AMaterialHallController::ActivateGateHandle(UPrimitiveComponent* Component) { return bGateDemo && AndGate && AndGate->ActivateHandle(Component); }
FText AMaterialHallController::GetMotionLabel() const
{
    if (bLatchDemo && Latch) return FText::FromString(Latch->IsCycling() ? TEXT("Pause cycle") : TEXT("Cycle memory"));
    if (bGateDemo && AndGate) return FText::FromString(AndGate->IsCycling() ? TEXT("Pause cycle") : TEXT("Cycle all states"));
    return FText::FromString(bAnimateRod ? TEXT("Pause rotation") : TEXT("Resume rotation"));
}

TSharedRef<SWidget> AMaterialHallController::BuildGateControls(bool bForVR)
{
    static FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor(.055f,.11f,.13f,1)))
        .SetHovered(FSlateColorBrush(FLinearColor(.10f,.24f,.27f,1)))
        .SetPressed(FSlateColorBrush(FLinearColor(.04f,.08f,.10f,1)))
        .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
    const FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Regular",bForVR ? 24 : 13);
    const FLinearColor Ink=bForVR ? FLinearColor::White : FLinearColor(.035f,.055f,.06f,1);
    TSharedRef<SHorizontalBox> Inputs=SNew(SHorizontalBox);
    for (int32 Index=0;Index<2;++Index)
        Inputs->AddSlot().FillWidth(1).Padding(0,0,6,0)
        [SNew(SButton).ButtonStyle(&Style).IsFocusable(false).ContentPadding(FMargin(bForVR ? 18 : 12,bForVR ? 14 : 10))
            .OnClicked_Lambda([this,Index](){ToggleGateInput(Index);return FReply::Handled();})
            [SNew(STextBlock).Font(Font).ColorAndOpacity(Index ? FLinearColor(1,.62f,.32f) : FLinearColor(.40f,.86f,.95f))
                .Text_Lambda([this,Index](){return FText::FromString(FString::Printf(TEXT("Input %s   %d"),Index ? TEXT("B") : TEXT("A"),AndGate && AndGate->InputTarget(Index) ? 1 : 0));})]];
    TSharedRef<SHorizontalBox> States=SNew(SHorizontalBox);
    for (int32 State=0;State<4;++State)
        States->AddSlot().FillWidth(1).Padding(0,0,5,0)
        [SNew(SButton).ButtonStyle(&Style).IsFocusable(false).ContentPadding(FMargin(7,bForVR ? 13 : 9))
            .OnClicked_Lambda([this,State](){if(AndGate) AndGate->SetInputs((State&2)!=0,(State&1)!=0);return FReply::Handled();})
            [SNew(STextBlock).Font(Font)
                .ColorAndOpacity_Lambda([this,State](){return AndGate && ((AndGate->InputTarget(0) ? 2 : 0)+(AndGate->InputTarget(1) ? 1 : 0))==State ? FLinearColor(.55f,1,.64f) : FLinearColor(.8f,.85f,.85f);})
                .Text(FText::FromString(FString::Printf(TEXT("%d%d : %d"),State/2,State%2,State==3 ? 1 : 0)))]];
    return SNew(SVerticalBox).Visibility_Lambda([this](){return bGateDemo ? EVisibility::Visible : EVisibility::Collapsed;})
        +SVerticalBox::Slot().AutoHeight()[Inputs]
        +SVerticalBox::Slot().AutoHeight().Padding(0,bForVR ? 16 : 10)
            [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",bForVR ? 27 : 16)).ColorAndOpacity(Ink)
                .Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("Output C   %d   %s"),AndGate && AndGate->OutputBit() ? 1 : 0,AndGate && !AndGate->IsSettled() ? TEXT("/ moving") : TEXT("/ settled")));})]
        +SVerticalBox::Slot().AutoHeight()[States]
        +SVerticalBox::Slot().AutoHeight().Padding(0,bForVR ? 12 : 8,0,0)
            [SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).Text(FText::FromString(TEXT("Both inputs at 1 advance the output.")))];
}

void AMaterialHallController::TickGateTest()
{
    if (!AndGate) { bGateTestPassed=false; return; }
    double Error=0;
    bGateTestPassed &= AndGate->CheckVisualConstraints(Error);
    MaxGateConstraintError=FMath::Max(MaxGateConstraintError,Error);
    ++GateTestSamples;
    const double Now=FPlatformTime::Seconds();
    if (GateTestStep==-1 && Now-TestStart>3) { Capture(TEXT("and-table.png")); GateTestStep=-2; }
    if (Now-TestStart<5) return;
    // Gray-code circuit, simultaneous transitions, and both mixed states.
    static const int32 States[]={0,2,0,1,3,2,3,0,3,1,0};
    if (GateTestStep<0)
    {
        GateTestStep=0; GateStepTime=Now;
        AndGate->SetInputs(false,false); SetView(3); return;
    }
    const FHallAndPose Pose=AndGate->GetPose();
    if (GateTestStep==1 || GateTestStep==2 || GateTestStep==3)
        MaxGateSingleInputDrift=FMath::Max(MaxGateSingleInputDrift,FMath::Abs(Pose.OutputTravel));
    if (Now-GateStepTime<2.3) return;
    const int32 State=States[GateTestStep];
    const double Expected=State==3 ? 1 : 0;
    const bool bPassed=AndGate->IsSettled() && FMath::Abs(Pose.OutputTravel-Expected)<.00001;
    bGateTestPassed &= bPassed;
    TSharedRef<FJsonObject> Record=MakeShared<FJsonObject>();
    Record->SetStringField(TEXT("inputs"),FString::Printf(TEXT("%d%d"),State/2,State%2));
    Record->SetNumberField(TEXT("output_travel"),Pose.OutputTravel);
    Record->SetBoolField(TEXT("passed"),bPassed);
    GateTestResults.Add(MakeShared<FJsonValueObject>(Record));
    if (GateTestStep<5) Capture(FString::Printf(TEXT("and-%d%d.png"),State/2,State%2));
    ++GateTestStep;
    GateStepTime=Now;
    if (GateTestStep<UE_ARRAY_COUNT(States))
    {
        const int32 Next=States[GateTestStep];
        // Exercise the same keyboard input path as desktop interaction when a
        // single input changes; exercise the state buttons' path for both.
        const int32 Changed=State^Next;
        if (Changed==1 || Changed==2)
        {
            const FKey Key=Changed==2 ? EKeys::Z : EKeys::X;
            InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),Key,IE_Pressed,FPlatformTime::Cycles64()));
            InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),Key,IE_Released,FPlatformTime::Cycles64()));
        }
        else AndGate->SetInputs((Next&2)!=0,(Next&1)!=0);
        return;
    }
    // Both click and Quest laser paths call ActivateHandle after a visibility ray hit.
    FHitResult Hit;
    const FVector Handle(FHallAndKinematics::InputRestX-65,-18,90);
    const bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Handle+FVector(0,0,30),Handle-FVector(0,0,10),ECC_Visibility);
    const bool bHandlePassed=bHit && ActivateGateHandle(Hit.GetComponent()) && AndGate->InputTarget(0);
    bGateTestPassed &= bHandlePassed && MaxGateSingleInputDrift<.08 && GateTestSamples>100;
    // Check that switching demos restores the original assembly visibility.
    SelectDemo(false);
    bGateTestPassed &= !BrassRod->IsHidden() && AndGate->IsHidden();
    SelectDemo(true);
    bGateTestPassed &= BrassRod->IsHidden() && !AndGate->IsHidden();
    TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("passed"),bGateTestPassed);
    Result->SetArrayField(TEXT("transitions"),GateTestResults);
    Result->SetNumberField(TEXT("max_visual_joint_error_cm"),MaxGateConstraintError);
    Result->SetNumberField(TEXT("max_single_input_output_fraction"),MaxGateSingleInputDrift);
    Result->SetNumberField(TEXT("constraint_samples"),GateTestSamples);
    Result->SetBoolField(TEXT("input_handle_trace_and_toggle"),bHandlePassed);
    Result->SetNumberField(TEXT("input_link_length_cm"),FHallAndKinematics::InputLinkLength());
    Result->SetNumberField(TEXT("output_link_length_cm"),FHallAndKinematics::OutputLinkLength());
    FString Output; auto Writer=TJsonWriterFactory<>::Create(&Output);FJsonSerializer::Serialize(Result,Writer);
    IFileManager::Get().MakeDirectory(*CaptureDirectory,true);
    FFileHelper::SaveStringToFile(Output,*(CaptureDirectory/TEXT("and-check.json")));
    UE_LOG(LogTemp,Display,TEXT("AND GATE TEST %s"),bGateTestPassed ? TEXT("PASSED") : TEXT("FAILED"));
    bTestFinished=true;
    FGenericPlatformMisc::RequestExit(false);
}
