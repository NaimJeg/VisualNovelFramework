// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelDialogueScreenWidget.h"
#include "NovelLog.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "NovelDialogueOptionWidget.h"
#include "NovelPresentationSettings.h"
#include "NovelPresentationSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "NovelCharacterSpriteWidget.h"
#include "NovelDialogueBranchData.h"
#include "FDialogueNodeHandle.h"
#include "NovelStorySubsystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

/** -------------------------------------------------------------------------- *
 *  Initialization & Setup
 * --------------------------------------------------------------------------- */

void UNovelDialogueScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    /// Establish the reactive link between the UI presentation layer and the underlying state machine
    if (UGameInstance* GI = GetGameInstance())
    {
        StorySys = GI->GetSubsystem<UNovelStorySubsystem>();
        if (StorySys)
        {
            /// Core Text
            StorySys->OnDialogueLineChanged.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnDialogueLineUpdated);
            
            /// Visual Intents
            StorySys->OnBackgroundChangedEvent.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnBackgroundChanged);
            StorySys->OnCharacterShownEvent.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnCharacterShown);
            StorySys->OnCharacterHiddenEvent.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnCharacterHidden);
            
            /// State Flow & Options
            StorySys->OnDialogueResetRequestedEvent.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnDialogueResetRequested);
            StorySys->OnDialogueOptionsShowEvent.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnDialogueOptionsShow);
            StorySys->OnDialogueOptionsHideEvent.AddUniqueDynamic(this, &UNovelDialogueScreenWidget::OnDialogueOptionsHide);
        }
    }

    /// Bind local player inputs to their respective routing functions
    if (Btn_Next) { Btn_Next->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::NextDialogue); Btn_Next->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::NextDialogue); }
    if (Btn_SaveLoad) { Btn_SaveLoad->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::ShowSaveLoadScreen); Btn_SaveLoad->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::ShowSaveLoadScreen); }
    if (Btn_Hide) { Btn_Hide->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::HideDialogue); Btn_Hide->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::HideDialogue); }
    if (Btn_History) { Btn_History->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::ShowHistory); Btn_History->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::ShowHistory); }
}

void UNovelDialogueScreenWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TypewriterTimerHandle);
    }

    if (StorySys)
    {
        StorySys->OnDialogueLineChanged.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnDialogueLineUpdated);
        StorySys->OnBackgroundChangedEvent.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnBackgroundChanged);
        StorySys->OnCharacterShownEvent.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnCharacterShown);
        StorySys->OnCharacterHiddenEvent.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnCharacterHidden);
        StorySys->OnDialogueResetRequestedEvent.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnDialogueResetRequested);
        StorySys->OnDialogueOptionsShowEvent.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnDialogueOptionsShow);
        StorySys->OnDialogueOptionsHideEvent.RemoveDynamic(this, &UNovelDialogueScreenWidget::OnDialogueOptionsHide);
    }

    if (Btn_Next) Btn_Next->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::NextDialogue);
    if (Btn_SaveLoad) Btn_SaveLoad->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::ShowSaveLoadScreen);
    if (Btn_Hide) Btn_Hide->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::HideDialogue);
    if (Btn_History) Btn_History->OnClicked.RemoveDynamic(this, &UNovelDialogueScreenWidget::ShowHistory);

    StorySys = nullptr;
    Super::NativeDestruct();
}

/** -------------------------------------------------------------------------- *
 *  Visual State Receivers
 * --------------------------------------------------------------------------- */

void UNovelDialogueScreenWidget::OnBackgroundChanged(TSoftObjectPtr<UTexture2D> NewBackground)
{
    if (Img_Background)
    {
        /// If receiving a null pointer, completely hide the image widget to eliminate unnecessary render cost
        if (NewBackground.IsNull())
        {
            Img_Background->SetBrushFromTexture(nullptr);
            Img_Background->SetVisibility(ESlateVisibility::Hidden);
        }
        else
        {
            /// Soft texture assignment natively hooks into Unreal's texture streaming, preventing main-thread hitching
            Img_Background->SetBrushFromSoftTexture(NewBackground);
            Img_Background->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void UNovelDialogueScreenWidget::OnCharacterShown(FName CharacterID, TSoftObjectPtr<UTexture2D> CharacterSprite, float XPosition)
{
    if (!CharacterLayer || CharacterSprite.IsNull()) return;

    UNovelCharacterSpriteWidget* TargetWidget = nullptr;

    if (ActiveCharacters.Contains(CharacterID))
    {
        TargetWidget = ActiveCharacters[CharacterID];
    }
    else
    {
        const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>();
        if (!Settings || Settings->CharacterSpriteWidgetClass.IsNull())
        {
            UE_LOG(LogNovel, Error, TEXT("CharacterSpriteWidgetClass is missing in Project Settings."));
            return;
        }

        TargetWidget = CreateWidget<UNovelCharacterSpriteWidget>(this, Settings->CharacterSpriteWidgetClass.LoadSynchronous());
        
        if (TargetWidget)
        {
            UPanelSlot* AddedSlot = CharacterLayer->AddChild(TargetWidget);
            ActiveCharacters.Add(CharacterID, TargetWidget);
            
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
            {
                CanvasSlot->SetAnchors(FAnchors(XPosition, 0.0f, XPosition, 1.0f));
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                CanvasSlot->SetAutoSize(true);
                CanvasSlot->SetOffsets(FMargin(0.0f));
            }
        }
    }

    if (TargetWidget)
    {
        TargetWidget->SetSprite(CharacterSprite);
        TargetWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void UNovelDialogueScreenWidget::OnCharacterHidden(FName CharacterID)
{
    if (ActiveCharacters.Contains(CharacterID))
    {
        UNovelCharacterSpriteWidget* WidgetToRemove = ActiveCharacters[CharacterID];
        if (WidgetToRemove)
        {
            WidgetToRemove->RemoveFromParent();
        }
        ActiveCharacters.Remove(CharacterID);
    }
}

void UNovelDialogueScreenWidget::SetTextSpeedMultiplier(float InMultiplier)
{
    TextSpeedMultiplier = InMultiplier;
}

/** -------------------------------------------------------------------------- *
 *  Player Input & System Controls
 * --------------------------------------------------------------------------- */

void UNovelDialogueScreenWidget::NextDialogue()
{
    /// UX Check: If the player hid the UI to view the CG, the first click should restore the UI, NOT skip the dialogue line.
    if (bIsUIHidden)
    {
        bIsUIHidden = false;
        OnDialogueUIVisibilityChanged(true);
        return;
    }
    
    /// Typewriter Intercept: 如果正在打字，玩家点击的意图是“跳过打字动画，直接显示全句�?
    if (bIsTyping)
    {
        FinishTypewriter();
        return;
    }

    /// Forward the intent to progress the state machine
    if (StorySys)
    {
        StorySys->Advance();
    }
}

void UNovelDialogueScreenWidget::ShowSaveLoadScreen()
{
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UNovelPresentationSubsystem* Presentation = LocalPlayer->GetSubsystem<UNovelPresentationSubsystem>())
        {
            Presentation->RequestSaveLoadScreen(true);
        }
    }
}

void UNovelDialogueScreenWidget::ShowHistory()
{
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UNovelPresentationSubsystem* Presentation = LocalPlayer->GetSubsystem<UNovelPresentationSubsystem>())
        {
            Presentation->RequestHistoryScreen();
        }
    }
}

void UNovelDialogueScreenWidget::HideDialogue()
{
    /// Prevents double-toggling state
    if (bIsUIHidden) return;

    bIsUIHidden = true;

    /// Triggers Blueprint animations to smoothly fade out the text box and borders
    OnDialogueUIVisibilityChanged(false);
}

/** -------------------------------------------------------------------------- *
 *  Dialogue & Text Updates
 * --------------------------------------------------------------------------- */

void UNovelDialogueScreenWidget::OnDialogueLineUpdated(const FText& Speaker, const FText& Text)
{
    if (SpeakerName) SpeakerName->SetText(Speaker);
    
    TargetDialogueString = Text.ToString();
    CurrentCharacterIndex = 0;
    bIsTyping = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TypewriterTimerHandle);
    }

    const float SpeedMultiplier = TextSpeedMultiplier;

    if (SpeedMultiplier <= 0.0f || SpeedMultiplier > 10.0f)
    {
        FinishTypewriter();
        return;
    }

    float TimePerChar = 0.033f / SpeedMultiplier;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            TypewriterTimerHandle, 
            this, 
            &UNovelDialogueScreenWidget::OnTypewriterTick, 
            TimePerChar, 
            true
        );
    }
}

void UNovelDialogueScreenWidget::OnTypewriterTick()
{
    CurrentCharacterIndex++;

    if (CurrentCharacterIndex >= TargetDialogueString.Len())
    {
        FinishTypewriter();
        return;
    }

    FString CurrentSubString = TargetDialogueString.Left(CurrentCharacterIndex);
    
    if (DialogueText)
    {
        DialogueText->SetText(FText::FromString(CurrentSubString));
    }
}

void UNovelDialogueScreenWidget::FinishTypewriter()
{
    bIsTyping = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TypewriterTimerHandle);
    }
    
    if (DialogueText)
    {
        DialogueText->SetText(FText::FromString(TargetDialogueString));
    }
}

void UNovelDialogueScreenWidget::OnDialogueResetRequested()
{
    /// Guarantee no "ghost" visuals persist during hard state transitions (e.g., returning to Title or loading a Save)
    OnBackgroundChanged(nullptr);

    for (auto& Pair : ActiveCharacters)
    {
        if (Pair.Value)
        {
            Pair.Value->RemoveFromParent();
        }
    }
    ActiveCharacters.Empty();

    if (SpeakerName) SpeakerName->SetText(FText::GetEmpty());
    if (DialogueText) DialogueText->SetText(FText::GetEmpty());

    /// Forcibly un-hide the UI if the player happened to be in CG-viewing mode during the reset
    if (bIsUIHidden)
    {
        bIsUIHidden = false;
        OnDialogueUIVisibilityChanged(true);
    }

    /// Tear down any active options
    if (Box_Options)
    {
        Box_Options->ClearChildren();
        Box_Options->SetVisibility(ESlateVisibility::Collapsed);
    }
}

/** -------------------------------------------------------------------------- *
 *  Branching Options UI
 * --------------------------------------------------------------------------- */

void UNovelDialogueScreenWidget::OnDialogueOptionsShow(const TArray<FDialogueOption>& Options)
{
    /// CRITICAL UX: Raise the invisible shield panel over the main screen to intercept stray clicks, 
    /// preventing the player from accidentally triggering 'NextDialogue' while deciding.
    if (OptionLayer)
    {
        OptionLayer->SetVisibility(ESlateVisibility::Visible);
    }
    
    /// Prepare the physical container to accept newly generated buttons
    if (Box_Options) 
    {
        Box_Options->ClearChildren();
        Box_Options->SetVisibility(ESlateVisibility::Visible);
    }

    /// Retrieve the dynamically assigned UI class from the central Project Settings
    const UNovelPresentationSettings* Settings = GetDefault<UNovelPresentationSettings>();
    if (!Settings || !Settings->OptionWidgetClass)
    {
        UE_LOG(LogNovel, Error, TEXT("OptionWidgetClass is missing in Novel Story Settings."));
        return;
    }

    /// Instantiate and populate a button for every branching path configured in the Data Asset
    for (const FDialogueOption& OptionData : Options)
    {
        UNovelDialogueOptionWidget* NewOption = CreateWidget<UNovelDialogueOptionWidget>(this, Settings->OptionWidgetClass);
        if (NewOption)
        {
            /// Pass the display text and the payload (Intents) into the button
            NewOption->InitOption(OptionData.OptionText, Box_Options ? Box_Options->GetChildrenCount() : 0, StorySys ? StorySys->GetCurrentNodeHandle() : FDialogueNodeHandle());
            
            if (Box_Options)
            {
                Box_Options->AddChild(NewOption);
            }
        }
    }
}

void UNovelDialogueScreenWidget::OnDialogueOptionsHide()
{
    /// Lower the invisible shield, restoring normal click-to-continue functionality to the screen
    if (OptionLayer)
    {
        OptionLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    /// Destroy the generated buttons and collapse the container so it doesn't block raycasts
    if (Box_Options)
    {
        Box_Options->ClearChildren();
        Box_Options->SetVisibility(ESlateVisibility::Collapsed);
    }
}