#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FDialogueNodeHandle.h"
#include "NovelStorySubsystem.generated.h"

// Forward Declarations
class USoundBase;
class UAudioComponent;
class UNovelDialogueBranchData;
class UDataTable;
class UNovelIntentBase;
struct FDialogueOption;

/** -------------------------------------------------------------------------- *
 *  System Delegates
 * --------------------------------------------------------------------------- */

/// Broadcasts when the dialogue text or speaker updates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineChanged, const FText&, Speaker, const FText&, Text);

/// Broadcasts when a game is successfully loaded or started
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);

/// Broadcasts to request the Save/Load UI to open (bIsSaveMode = true for Save, false for Load)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveLoadUIRequested, bool, bIsSaveMode);

/** -------------------------------------------------------------------------- *
 *  Visual & UI Delegates
 * --------------------------------------------------------------------------- */

/// Broadcasts when branching options should be displayed to the player
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueOptionsShow, const TArray<FDialogueOption>&, Options);

/// Broadcasts to hide the branching options UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueOptionsHide);

/// Broadcasts when the background image needs to change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackgroundChanged, TSoftObjectPtr<UTexture2D>, NewBackground);

/// Broadcasts when a character sprite should appear in a specific slot (Left, Center, Right)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterShown, FName, CharacterSlot, TSoftObjectPtr<UTexture2D>, CharacterSprite);

/// Broadcasts when a character sprite should be hidden
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterHidden, FName, CharacterSlot);

/// Broadcasts when a screen fade effect is requested
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScreenFade, FLinearColor, FadeColor, float, FadeDuration);

/// Broadcasts to force the UI to clear all currently displayed images and text
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueResetRequested);


/**
 * @class UNovelStorySubsystem
 * @brief The core State Machine and Data Manager for the Visual Novel framework.
 *        Handles dialogue progression, intent execution, audio/visual state, and save data.
 */
UCLASS()
class NOVEL_API UNovelStorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    /** ---------------------------------------------------------------------- *
     *  Initialization & Core Flow
     * ----------------------------------------------------------------------- */

    /// Initializes the subsystem and loads basic settings
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /// Advances the state machine to the next dialogue row or intent
    void NextDialogue();

    /// Starts a completely new game from Chapter 0
    UFUNCTION(BlueprintCallable, Category = "Story")
    void StartNewGame();

    /// Continues the game from the most recent auto-save or progress
    UFUNCTION(BlueprintCallable, Category = "Story")
    void ContinueLastGame();

    /// Exits the application
    UFUNCTION(BlueprintCallable, Category = "Story")
    void QuitGame();

    /** ---------------------------------------------------------------------- *
     *  Event Dispatchers
     * ----------------------------------------------------------------------- */
    
    UPROPERTY(BlueprintAssignable)
    FOnBackgroundChanged OnBackgroundChangedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnCharacterShown OnCharacterShownEvent;

    UPROPERTY(BlueprintAssignable)
    FOnCharacterHidden OnCharacterHiddenEvent;

    UPROPERTY(BlueprintAssignable)
    FOnScreenFade OnScreenFadeEvent;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueLineChanged OnDialogueLineChanged;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueOptionsShow OnDialogueOptionsShowEvent;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueOptionsHide OnDialogueOptionsHideEvent;

    UPROPERTY(BlueprintAssignable)
    FOnSaveLoadUIRequested OnSaveLoadUIRequestedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnGameStarted OnGameStartedEvent;

    UPROPERTY(BlueprintAssignable)
    FOnDialogueResetRequested OnDialogueResetRequestedEvent;

    /** ---------------------------------------------------------------------- *
     *  Global State Properties
     * ----------------------------------------------------------------------- */

    /// Cached reference to the current chapter's branching data
    UPROPERTY()
    UNovelDialogueBranchData* BranchData;

    /// The ID of the currently playing chapter
    UPROPERTY(BlueprintReadOnly, Category = "Story")
    int32 CurrentChapterID = 0;

    /// The soft reference to the currently displayed background (Used for Save/Load restoration)
    UPROPERTY(BlueprintReadWrite, Category = "Story")
    TSoftObjectPtr<UTexture2D> CurrentBackground;

    /** ---------------------------------------------------------------------- *
     *  UI Triggers
     * ----------------------------------------------------------------------- */

    /// Requests the main Save/Load UI to open
    UFUNCTION(BlueprintCallable, Category = "Story")
    void ShowSaveLoadMenu(bool bIsSaveMode);

    /// Requests the Dialogue History / Backlog UI to open
    UFUNCTION(BlueprintCallable, Category = "Story")
    void ShowHistory();

    /** ---------------------------------------------------------------------- *
     *  Save & Load System
     * ----------------------------------------------------------------------- */

    /// Retrieves all existing save slot names from the local disk
    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    TArray<FString> GetAllSaveSlotNames() const;

    /// Saves the current game state to a specified slot (Generates a timestamp name if empty)
    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    void SaveGame(FString SlotNameOverride = TEXT(""));

    /// Loads the game state from a specified slot
    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    void LoadGame(FString SlotName);

    /// Deletes a specific save slot from the local disk
    UFUNCTION(BlueprintCallable, Category = "Story|SaveLoad")
    bool DeleteSaveSlot(const FString& SlotName);

    /** ---------------------------------------------------------------------- *
     *  Intent & Node Execution
     * ----------------------------------------------------------------------- */

    /// Forces the state machine pointer to jump to a specific node
    UFUNCTION(BlueprintCallable, Category = "Story|Intent")
    void JumpToNode(FDialogueNodeHandle TargetNode);

    /// Injects an array of intents into the execution queue (Used heavily by Dialogue Options)
    UFUNCTION(BlueprintCallable, Category = "Story|Intent")
    void ProcessIntents(const TArray<UNovelIntentBase*>& Intents);

    /** ---------------------------------------------------------------------- *
     *  Visual & Audio Control API
     * ----------------------------------------------------------------------- */

    /// Wipes all currently active visual states and requests the UI to clear slots
    UFUNCTION(BlueprintCallable, Category = "Story|Visuals")
    void ResetVisualState();

    /// Plays a new Background Music track, fading out the old one if it exists
    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void PlayBGM(TSoftObjectPtr<USoundBase> NewBGM, float FadeInTime = 1.0f);

    /// Fades out and stops the currently playing Background Music
    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void StopBGM(float FadeOutTime = 1.0f);

    /// Plays a Sound Effect by fetching it from the global SFX Dictionary
    UFUNCTION(BlueprintCallable, Category = "Story|Audio")
    void PlaySFX(FName SFXKey);


private:

    /** ---------------------------------------------------------------------- *
     *  Internal Async Callbacks
     * ----------------------------------------------------------------------- */

    /// Called when the assets for a new chapter have finished loading into memory
    void OnChapterLoadedAsync(int32 ChapterIndex);

    /// Called when the assets required for a saved game have finished loading
    void OnLoadGameAsyncComplete(int32 LoadedChapterID, FDialogueNodeHandle LoadedNode);

    /** ---------------------------------------------------------------------- *
     *  Internal State Machine Engine
     * ----------------------------------------------------------------------- */

    /// Pointer to the currently executing dialogue line
    UPROPERTY()
    FDialogueNodeHandle CurrentNode;

    /// The queue of visual/audio/logic intents currently being processed sequentially
    UPROPERTY()
    TArray<UNovelIntentBase*> CurrentIntentQueue;

    /// Reference to the active Audio Component handling the BGM
    UPROPERTY()
    UAudioComponent* CurrentBGMComponent;

    /// Broadcasts the Text and Speaker from the CurrentNode to the UI
    UFUNCTION()
    void BroadcastCurrentLine();

    /// Moves the pointer to the next consecutive row in the Data Table
    UFUNCTION()
    void AdvanceToNextRow();

    /// The core execution hub: Evaluates options, queues auto-intents, and starts intent playback
    UFUNCTION()
    void ExecuteCurrentNode();

    /// Pops the next intent from the queue and executes it
    UFUNCTION()
    void PlayNextIntent();

    /// Helper: Gets the active Data Table from the current node
    UDataTable* GetCurrentTable() const { return CurrentNode.Table.Get(); }

    /// Helper: Gets the active Row Name from the current node
    FName GetCurrentRowName() const { return CurrentNode.RowName; }

};