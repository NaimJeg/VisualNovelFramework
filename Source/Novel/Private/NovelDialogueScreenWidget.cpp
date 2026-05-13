// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelDialogueScreenWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "NovelDialogueOptionWidget.h"
#include "NovelStorySettings.h"
#include "Components/VerticalBox.h"
#include "NovelDialogueBranchData.h"
#include "FDialogueNodeHandle.h"
#include "NovelStorySubsystem.h"
#include "Components/Image.h"

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
            StorySys->OnDialogueLineChanged.AddDynamic(this, &UNovelDialogueScreenWidget::OnDialogueLineUpdated);
            
            /// Visual Intents
            StorySys->OnBackgroundChangedEvent.AddDynamic(this, &UNovelDialogueScreenWidget::OnBackgroundChanged);
            StorySys->OnCharacterShownEvent.AddDynamic(this, &UNovelDialogueScreenWidget::OnCharacterShown);
            StorySys->OnCharacterHiddenEvent.AddDynamic(this, &UNovelDialogueScreenWidget::OnCharacterHidden);
            
            /// State Flow & Options
            StorySys->OnDialogueResetRequestedEvent.AddDynamic(this, &UNovelDialogueScreenWidget::OnDialogueResetRequested);
            StorySys->OnDialogueOptionsShowEvent.AddDynamic(this, &UNovelDialogueScreenWidget::OnDialogueOptionsShow);
            StorySys->OnDialogueOptionsHideEvent.AddDynamic(this, &UNovelDialogueScreenWidget::OnDialogueOptionsHide);
        }
    }

    /// Bind local player inputs to their respective routing functions
    if (Btn_Next) Btn_Next->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::NextDialogue);
    if (Btn_SaveLoad) Btn_SaveLoad->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::ShowSaveLoadScreen);
    if (Btn_Hide) Btn_Hide->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::HideDialogue);
    if (Btn_History) Btn_History->OnClicked.AddDynamic(this, &UNovelDialogueScreenWidget::ShowHistory);
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

void UNovelDialogueScreenWidget::OnCharacterShown(FName CharacterSlot, TSoftObjectPtr<UTexture2D> CharacterSprite)
{
    UImage* TargetImage = nullptr;

    /// Route the logical character slot requested by the Subsystem to the physical UI container
    if (CharacterSlot == TEXT("Left")) TargetImage = Img_CharLeft;
    else if (CharacterSlot == TEXT("Right")) TargetImage = Img_CharRight;
    else if (CharacterSlot == TEXT("Center")) TargetImage = Img_CharCenter;

    if (TargetImage)
    {
        if (!CharacterSprite.IsNull())
        {
            TargetImage->SetBrushFromSoftTexture(CharacterSprite);
            TargetImage->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void UNovelDialogueScreenWidget::OnCharacterHidden(FName CharacterSlot)
{
    UImage* TargetImage = nullptr;

    if (CharacterSlot == TEXT("Left")) TargetImage = Img_CharLeft;
    else if (CharacterSlot == TEXT("Right")) TargetImage = Img_CharRight;
    else if (CharacterSlot == TEXT("Center")) TargetImage = Img_CharCenter;

    if (TargetImage)
    {
        TargetImage->SetBrushFromTexture(nullptr);
        TargetImage->SetVisibility(ESlateVisibility::Hidden);
    }
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
    
    /// Forward the intent to progress the state machine
    if (StorySys)
    {
        StorySys->NextDialogue();
    }
}

void UNovelDialogueScreenWidget::ShowSaveLoadScreen()
{
    /// Dispatch request back to root/subsystem level, requesting a Save-mode overlay
    if (StorySys) { StorySys->OnSaveLoadUIRequestedEvent.Broadcast(true); }
}

void UNovelDialogueScreenWidget::ShowHistory()
{
    if (StorySys) StorySys->ShowHistory();
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
    UE_LOG(LogTemp, Warning, TEXT("OnDialogueLineUpdated called: %s | %s"), *Speaker.ToString(), *Text.ToString());
    
    if (SpeakerName) SpeakerName->SetText(Speaker);
    else UE_LOG(LogTemp, Error, TEXT("SpeakerName is null!"));
    
    if (DialogueText) DialogueText->SetText(Text);
    else UE_LOG(LogTemp, Error, TEXT("DialogueText is null!"));
}

void UNovelDialogueScreenWidget::OnDialogueResetRequested()
{
    /// Guarantee no "ghost" visuals persist during hard state transitions (e.g., returning to Title or loading a Save)
    OnBackgroundChanged(nullptr);

    OnCharacterHidden(TEXT("Left"));
    OnCharacterHidden(TEXT("Center"));
    OnCharacterHidden(TEXT("Right"));

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
    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
    if (!Settings || !Settings->OptionWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("OptionWidgetClass is missing! Please assign it in Project Settings -> Novel Story Settings."));
        return;
    }

    /// Instantiate and populate a button for every branching path configured in the Data Asset
    for (const FDialogueOption& OptionData : Options)
    {
        UNovelDialogueOptionWidget* NewOption = CreateWidget<UNovelDialogueOptionWidget>(this, Settings->OptionWidgetClass);
        if (NewOption)
        {
            /// Pass the display text and the payload (Intents) into the button
            NewOption->InitOption(OptionData.OptionText, OptionData.Intents);
            
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