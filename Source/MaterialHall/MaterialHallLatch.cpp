#include "MaterialHall.h"
#include "HallLatch.h"
#include "HallAndGate.h"
#include "Engine/StaticMeshActor.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"

void AMaterialHallController::SelectLatchDemo()
{
    SelectDemo(false); bLatchDemo=true;
    if (Latch) Latch->SetDemoVisible(true);
    for (AStaticMeshActor* Actor : {BrassRod.Get(),SteelPlate.Get(),PivotPin.Get(),PivotCollar.Get()})
        if (Actor) { Actor->SetActorHiddenInGame(true); Actor->SetActorEnableCollision(false); }
    SetView(0);
}
TSharedRef<SWidget> AMaterialHallController::BuildLatchControls()
{
    static FButtonStyle Style=FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor(.055f,.11f,.13f,1)))
        .SetHovered(FSlateColorBrush(FLinearColor(.10f,.24f,.27f,1)))
        .SetPressed(FSlateColorBrush(FLinearColor(.04f,.08f,.10f,1)))
        .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
    const auto Font=FCoreStyle::GetDefaultFontStyle("Regular",13);
    const FLinearColor Ink(.035f,.055f,.06f,1);
    auto Button=[&](FString Text,TFunction<void()> Action)
    {
        return SNew(SButton).ButtonStyle(&Style).IsFocusable(false).ContentPadding(FMargin(9,8))
            .OnClicked_Lambda([Action](){Action();return FReply::Handled();})
            [SNew(STextBlock).Text(FText::FromString(Text)).Font(Font).ColorAndOpacity(FLinearColor::White)];
    };
    TSharedRef<SHorizontalBox> Inputs=SNew(SHorizontalBox);
    for (int32 I=0;I<2;++I)
        Inputs->AddSlot().FillWidth(1).Padding(0,0,5,0)
        [SNew(SButton).ButtonStyle(&Style).IsFocusable(false).ContentPadding(FMargin(10,8))
            .OnClicked_Lambda([this,I](){Latch->ToggleControl(I);return FReply::Handled();})
            [SNew(STextBlock).Font(Font).ColorAndOpacity(I ? FLinearColor(1,.62f,.32f) : FLinearColor(.4f,.86f,.95f))
                .Text_Lambda([this,I](){return FText::FromString(FString::Printf(TEXT("%s   %d"),I ? TEXT("Reset R'") : TEXT("Set S'"),Latch->State().Target[I] ? 1 : 0));})]];
    return SNew(SVerticalBox).Visibility_Lambda([this](){return bLatchDemo ? EVisibility::Visible : EVisibility::Collapsed;})
        +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,5,0)[Button(TEXT("Set pulse"),[this](){Latch->Pulse(0);})]
            +SHorizontalBox::Slot().FillWidth(1).Padding(0,0,5,0)[Button(TEXT("Reset pulse"),[this](){Latch->Pulse(1);})]
            +SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("Hold"),[this](){Latch->SetControls(true,true);})]]
        +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Inputs]
        +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,4)
            [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",16)).ColorAndOpacity(Ink)
                .Text_Lambda([this](){const auto& D=Latch->State(); const auto Bit=[&](int32 I){return D.Bit(I)<0 ? TEXT("?") : D.Bit(I) ? TEXT("1") : TEXT("0");}; return FText::FromString(FString::Printf(TEXT("Q  %s      Q'  %s"),Bit(0),Bit(1)));})]
        +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(Font).ColorAndOpacity(Ink).Text_Lambda([this](){return FText::FromString(Latch->Status());})]
        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,12)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",11)).ColorAndOpacity(Ink)
            .Text(FText::FromString(TEXT("0 asserts a control; 1 releases it.\nSpring feedback retains the released state.")))];
}
void AMaterialHallController::TickLatchTest()
{
    if (!Latch) { bLatchTestPassed=false; return; }
    double Joint=0,Connection=0; bLatchTestPassed &= Latch->CheckAssembly(Joint,Connection);
    MaxLatchJointError=FMath::Max(MaxLatchJointError,Joint); MaxLatchConnectionError=FMath::Max(MaxLatchConnectionError,Connection); ++LatchSamples;
    const double Now=FPlatformTime::Seconds();
    if (LatchTestStep==-1 && Now-TestStart>3) { Capture(TEXT("latch-table.png")); LatchTestStep=-2; }
    if (Now-TestStart<5) return;
    if (LatchTestStep<0) { LatchTestStep=0; LatchStepTime=Now; SetView(3); Latch->SetControls(true,true); return; }
    const int32 ExpectedQ[]={0,1,1,0,0,1,-1,1,0,1};
    const int32 ExpectedBar[]={1,0,0,1,1,1,-1,0,1,0};
    const TCHAR* Names[]={TEXT("initial-hold"),TEXT("set"),TEXT("hold-set"),TEXT("reset"),TEXT("hold-reset"),TEXT("invalid"),TEXT("unresolved"),TEXT("set-pulse"),TEXT("reset-pulse"),TEXT("keyboard-set")};
    // Capture before changing the next target so the rendered parts and the
    // Slate status describe the same settled state.
    if (Now-LatchStepTime>4 && LatchCapturedStep!=LatchTestStep)
    {
        Capture(FString::Printf(TEXT("latch-%s.png"),Names[LatchTestStep])); LatchCapturedStep=LatchTestStep;
    }
    if (Now-LatchStepTime<4.5) return;
    const auto& D=Latch->State();
    const bool Passed=D.Bit(0)==ExpectedQ[LatchTestStep] && D.Bit(1)==ExpectedBar[LatchTestStep] && D.Settled();
    bLatchTestPassed &= Passed;
    TSharedRef<FJsonObject> R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("stage"),Names[LatchTestStep]); R->SetBoolField(TEXT("passed"),Passed);
    R->SetNumberField(TEXT("Q"),D.Signal(0)); R->SetNumberField(TEXT("Q_bar"),D.Signal(1)); R->SetNumberField(TEXT("feedback_error"),D.FeedbackError());
    LatchResults.Add(MakeShared<FJsonValueObject>(R));
    ++LatchTestStep; LatchStepTime=Now;
    switch (LatchTestStep)
    {
        case 1: Latch->SetControls(false,true); return;
        case 2: case 4: case 6: Latch->SetControls(true,true); return;
        case 3: Latch->SetControls(true,false); return;
        case 5: Latch->SetControls(false,false); return;
        case 7: Latch->Pulse(0); return;
        case 8: Latch->Pulse(1); return;
        case 9:
            InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),EKeys::Z,IE_Pressed,FPlatformTime::Cycles64()));
            InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),EKeys::Z,IE_Released,FPlatformTime::Cycles64())); return;
    }
    SelectDemo(true); bLatchTestPassed &= Latch->IsHidden() && !AndGate->IsHidden();
    SelectLatchDemo(); bLatchTestPassed &= !Latch->IsHidden() && AndGate->IsHidden() && BrassRod->IsHidden();
    TSharedRef<FJsonObject> Result=MakeShared<FJsonObject>(); Result->SetBoolField(TEXT("passed"),bLatchTestPassed && LatchSamples>100);
    Result->SetArrayField(TEXT("stages"),LatchResults); Result->SetNumberField(TEXT("constraint_samples"),LatchSamples);
    Result->SetNumberField(TEXT("max_joint_error_cm"),MaxLatchJointError); Result->SetNumberField(TEXT("max_spring_endpoint_error_cm"),MaxLatchConnectionError);
    FString Output; auto Writer=TJsonWriterFactory<>::Create(&Output); FJsonSerializer::Serialize(Result,Writer);
    IFileManager::Get().MakeDirectory(*CaptureDirectory,true); FFileHelper::SaveStringToFile(Output,*(CaptureDirectory/TEXT("latch-check.json")));
    UE_LOG(LogTemp,Display,TEXT("RS LATCH TEST %s"),bLatchTestPassed ? TEXT("PASSED") : TEXT("FAILED"));
    bTestFinished=true; FGenericPlatformMisc::RequestExit(false);
}
