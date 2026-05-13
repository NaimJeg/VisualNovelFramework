// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelStorySubsystem.h"
#include "UnrealClient.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "NovelIntentBase.h"
#include "NovelLoadingSubsystem.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "NovelSaveGame.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NovelStorySettings.h"
#include "NovelDialogueScreenWidget.h"
#include "FDialogueNode.h"
#include "NovelDialogueBranchData.h"
#include "Engine/AssetManager.h"

/** -------------------------------------------------------------------------- *
 *  Initialization & Lifecycle
 * --------------------------------------------------------------------------- */

void UNovelStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    /// Cache settings early to ensure global configurations are available immediately on boot
    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
}

/** -------------------------------------------------------------------------- *
 *  Game Flow Management
 * --------------------------------------------------------------------------- */

void UNovelStorySubsystem::StartNewGame()
{
    /// Purge any lingering UI elements from previous sessions to ensure a clean slate
    ResetVisualState();
    
    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
    const FChapterData* Chapter0Data = Settings->ChapterMap.Find(0);

    if (Chapter0Data && !Chapter0Data->DialogueTable.IsNull())
    {
        TArray<FSoftObjectPath> PathsToLoad;
        PathsToLoad.Add(Chapter0Data->DialogueTable.ToSoftObjectPath());

        /// Branch data is optional; only request its load if the designer explicitly configured it
        if (!Chapter0Data->BranchData.IsNull())
        {
            PathsToLoad.Add(Chapter0Data->BranchData.ToSoftObjectPath());
        }

        /// Delegate asset loading to the loading subsystem to prevent main-thread hitches
        if (UNovelLoadingSubsystem* LoadingSys = GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>())
        {
            LoadingSys->RequestLoad(
                PathsToLoad,
                FOnLoadCompleted::CreateUObject(this, &UNovelStorySubsystem::OnChapterLoadedAsync, 0)
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StartNewGame:Chapter 0 Not Found"));
    }
}

void UNovelStorySubsystem::ContinueLastGame()
{
    /// @TODO: Implement auto-load logic targeting the most recent timestamp in the save directory
}

void UNovelStorySubsystem::QuitGame()
{
    /// Route through Kismet system library for a safe, standard application shutdown
    if (UWorld* World = GetWorld())
    {
        UKismetSystemLibrary::QuitGame(
            World,
            World->GetFirstPlayerController(),
            EQuitPreference::Quit,
            false
        );
    }
}

/** -------------------------------------------------------------------------- *
 *  State Machine: Node Execution & Intent Processing
 * --------------------------------------------------------------------------- */

void UNovelStorySubsystem::NextDialogue()
{
    /// Public wrapper to shift the state machine forward
    AdvanceToNextRow();
}

void UNovelStorySubsystem::AdvanceToNextRow()
{
    /// Guard against uninitialized state execution
    if (!CurrentNode.IsValid()) return;
    
    UDataTable* Table = CurrentNode.Table.Get();
    if (!Table) return;

    TArray<FName> RowNames = Table->GetRowNames();
    int32 Idx = RowNames.IndexOfByKey(CurrentNode.RowName);
    
    /// Shift the logic pointer to the consecutive row to maintain linear narrative flow
    if (Idx != INDEX_NONE && Idx + 1 < RowNames.Num())
    {
        CurrentNode.RowName = RowNames[Idx + 1];
        ExecuteCurrentNode();
    }
    else
    {
        /// Halt execution; the data table has been exhausted
        UE_LOG(LogTemp, Warning, TEXT("Chapter Ended!"));
    }
}

void UNovelStorySubsystem::ExecuteCurrentNode()
{
    if (!CurrentNode.IsValid()) return;

    const FBranchData* CurrentBranch = nullptr;
    
    /// 1. Evaluate Branching Logic: Check if the current line acts as an active decision point
    if (BranchData && BranchData->BranchMap.Contains(CurrentNode.RowName))
    {
        CurrentBranch = &BranchData->BranchMap[CurrentNode.RowName];
        
        if (CurrentBranch->bIsChoiceNode)
        {
            BroadcastCurrentLine();
            OnDialogueOptionsShowEvent.Broadcast(CurrentBranch->Options);
            
            /// CRITICAL: Halt the state machine here. Await player input via ProcessIntents()
            return;
        }
    }

    /// 2. Queue Auto-Intents: Wipe old intents and load fresh ones tied to the current text
    CurrentIntentQueue.Empty();
    if (CurrentBranch && CurrentBranch->AutoIntents.Num() > 0)
    {
        CurrentIntentQueue.Append(CurrentBranch->AutoIntents);
    }

    /// 3. Update UI: Push the raw text to the dialogue box
    BroadcastCurrentLine(); 
    
    /// 4. Execute Logic: Start draining the intent queue for visual/audio effects
    PlayNextIntent();       
}

void UNovelStorySubsystem::BroadcastCurrentLine()
{
    if (!CurrentNode.IsValid()) return;

    UDataTable* Table = CurrentNode.Table.Get();
    if (!Table) return;

    /// Extract raw string data and dispatch it to listening UI widgets
    FDialogueRow* Row = CurrentNode.Table->FindRow<FDialogueRow>(CurrentNode.RowName, TEXT("Dialogue"));
    if (Row)
    {
        OnDialogueLineChanged.Broadcast(
            FText::FromString(Row->Speaker),
            FText::FromString(Row->Text)
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Row not found for RowName: %s"), *CurrentNode.RowName.ToString());
    }
}

void UNovelStorySubsystem::PlayNextIntent()
{
    /// Base case for recursion: The queue is fully drained
    if (CurrentIntentQueue.IsEmpty())
    {
        return; 
    }

    /// Pop the intent from the front of the queue
    UNovelIntentBase* NextIntent = CurrentIntentQueue[0];
    CurrentIntentQueue.RemoveAt(0);

    if (NextIntent)
    {
        /// Execute and bind the callback to trigger the next intent, creating a continuous execution chain
        NextIntent->ExecuteIntent(this, FOnIntentFinished::CreateUObject(this, &UNovelStorySubsystem::PlayNextIntent));
    }
    else
    {
        /// Skip null pointers defensively
        PlayNextIntent();
    }
}

void UNovelStorySubsystem::ProcessIntents(const TArray<UNovelIntentBase*>& Intents)
{
    /// Appends incoming intents (usually from clicked choices) and kickstarts the execution chain
    CurrentIntentQueue.Append(Intents);
    PlayNextIntent();
}

void UNovelStorySubsystem::JumpToNode(FDialogueNodeHandle TargetNode)
{
    /// Absolute override of the state machine pointer. Used for macro scene transitions and choice resolutions.
    if (TargetNode.IsValid())
    {
        CurrentNode = TargetNode;
        ExecuteCurrentNode();
    }
}

void UNovelStorySubsystem::OnChapterLoadedAsync(int32 ChapterIndex)
{
    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
    const FChapterData* ChapterData = Settings->ChapterMap.Find(ChapterIndex);

    if (ChapterData)
    {
        UDataTable* LoadedTable = ChapterData->DialogueTable.Get();
        BranchData = ChapterData->BranchData.Get();

        if (LoadedTable)
        {
            TArray<FName> RowNames = LoadedTable->GetRowNames();
            if (RowNames.Num() > 0)
            {
                /// Anchor the execution pointer to the very first row of the newly loaded chapter
                CurrentNode = FDialogueNodeHandle(LoadedTable, RowNames[0]);
                
                BroadcastCurrentLine();
                OnGameStartedEvent.Broadcast();
                
                /// Ignite the state machine
                ExecuteCurrentNode();
            }
        }
    }
}

/** -------------------------------------------------------------------------- *
 *  Save & Load Serialization
 * --------------------------------------------------------------------------- */

TArray<FString> UNovelStorySubsystem::GetAllSaveSlotNames() const
{
    TArray<FString> SaveFiles;
    FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");

    /// Direct OS-level file query is used here because Unreal's UGameplayStatics doesn't provide a bulk index getter
    IFileManager::Get().FindFiles(SaveFiles, *SaveDirectory, TEXT(".sav"));

    TArray<FString> SlotNames;
    for (const FString& File : SaveFiles)
    {
        SlotNames.Add(FPaths::GetBaseFilename(File));
    }

    /// Sort chronologically/alphabetically so the UI presents the most recent saves properly
    SlotNames.Sort([](const FString& A, const FString& B) { return A > B; });

    return SlotNames;
}

void UNovelStorySubsystem::SaveGame(FString SlotNameOverride)
{
    if (UNovelSaveGame* SaveGameInstance = Cast<UNovelSaveGame>(UGameplayStatics::CreateSaveGameObject(UNovelSaveGame::StaticClass())))
    {
        /// Snapshot the critical execution parameters required to perfectly reconstruct the timeline
        SaveGameInstance->SaveNode = CurrentNode;
        SaveGameInstance->SavedChapterID = CurrentChapterID;
        SaveGameInstance->SaveBackground = CurrentBackground;

        /// Fallback to a timestamped filename to prevent accidental slot overwrites if no name is provided
        FString SlotName = SlotNameOverride.IsEmpty() ? FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) : SlotNameOverride;

        if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, 0))
        {
            UE_LOG(LogTemp, Warning, TEXT("SaveGame Success! SlotName: %s"), *SlotName);
        }
    }
}

void UNovelStorySubsystem::LoadGame(FString SlotName)
{
    /// 1. Wipe the screen immediately to prevent visual bleeding of the current scene into the loaded scene
    ResetVisualState();
    
    UE_LOG(LogTemp, Warning, TEXT("NovelStorySubsystem: Trying Load Save Slot -> %s"), *SlotName);

    if (UNovelSaveGame* LoadedGame = Cast<UNovelSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
    {
        int32 TargetChapterID = LoadedGame->SavedChapterID;
        FDialogueNodeHandle TargetNode = LoadedGame->SaveNode;
        
        /// 2. Hydrate the background memory immediately so the async callback has access to the correct image
        CurrentBackground = LoadedGame->SaveBackground;

        const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
        const FChapterData* ChapterData = Settings->ChapterMap.Find(TargetChapterID);

        if (ChapterData && !ChapterData->DialogueTable.IsNull())
        {
            TArray<FSoftObjectPath> PathsToLoad;
            PathsToLoad.Add(ChapterData->DialogueTable.ToSoftObjectPath());

            if (!ChapterData->BranchData.IsNull())
            {
                PathsToLoad.Add(ChapterData->BranchData.ToSoftObjectPath());
            }

            /// 3. Re-trigger the async asset pipeline to guarantee the saved chapter's assets exist in memory before executing
            if (UNovelLoadingSubsystem* LoadingSys = GetGameInstance()->GetSubsystem<UNovelLoadingSubsystem>())
            {
                LoadingSys->RequestLoad(
                    PathsToLoad,
                    FOnLoadCompleted::CreateUObject(this, &UNovelStorySubsystem::OnLoadGameAsyncComplete, TargetChapterID, TargetNode)
                );
            }
        }
    }
}

void UNovelStorySubsystem::OnLoadGameAsyncComplete(int32 LoadedChapterID, FDialogueNodeHandle LoadedNode)
{
    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
    const FChapterData* ChapterData = Settings->ChapterMap.Find(LoadedChapterID);

    if (ChapterData)
    {
        /// Re-anchor the execution pointers to the serialized data
        BranchData = ChapterData->BranchData.Get();
        CurrentChapterID = LoadedChapterID;
        CurrentNode = LoadedNode;

        OnGameStartedEvent.Broadcast();
        
        /// Visually restore the background prior to logic execution so the player doesn't stare at a void
        if (!CurrentBackground.IsNull())
        {
            OnBackgroundChangedEvent.Broadcast(CurrentBackground);
        }
        
        /// Let the universal execution hub naturally resolve text, choices, and remaining intents for this specific loaded row
        ExecuteCurrentNode();

        UE_LOG(LogTemp, Warning, TEXT("Save Loaded Successfully! Chapter: %d, RowName: %s"), CurrentChapterID, *CurrentNode.RowName.ToString());
    }
}

bool UNovelStorySubsystem::DeleteSaveSlot(const FString& SlotName)
{
    if (SlotName.IsEmpty()) return false;

    bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, 0);

    if (bDeleted)
    {
        UE_LOG(LogTemp, Warning, TEXT("Deleted Save Slot: %s"), *SlotName);
    }

    return bDeleted;
}

/** -------------------------------------------------------------------------- *
 *  UI & Visual Triggers
 * --------------------------------------------------------------------------- */

void UNovelStorySubsystem::ShowSaveLoadMenu(bool bIsSaveMode)
{
    /// Pass a boolean flag so the UI knows whether to configure buttons for saving or loading
    OnSaveLoadUIRequestedEvent.Broadcast(bIsSaveMode);
}

void UNovelStorySubsystem::ShowHistory()
{
    UE_LOG(LogTemp, Warning, TEXT("NovelStorySubsystem: ShowHistory requested."));
}

void UNovelStorySubsystem::ResetVisualState()
{
    /// Nullifying pointers forces the Garbage Collector to clean up unused textures, and broadcasting instructs UI to hide widgets
    CurrentBackground = nullptr;
    CurrentIntentQueue.Empty(); 

    OnDialogueResetRequestedEvent.Broadcast();
}

/** -------------------------------------------------------------------------- *
 *  Audio Management
 * --------------------------------------------------------------------------- */

void UNovelStorySubsystem::PlayBGM(TSoftObjectPtr<USoundBase> NewBGM, float FadeInTime)
{
    if (NewBGM.IsNull()) return;

    /// Ensure graceful cross-fading by telling the existing component to silence itself before we spawn a new one
    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        CurrentBGMComponent->FadeOut(FadeInTime, 0.0f);
    }

    /// Assuming relatively small BGM files or a pre-warmed cache, synchronous load is utilized here to avoid async callback hell for simple audio
    USoundBase* LoadedSound = NewBGM.LoadSynchronous();
    if (LoadedSound)
    {
        /// Retain the handle so we maintain authority to stop/fade this exact track later
        CurrentBGMComponent = UGameplayStatics::SpawnSound2D(this, LoadedSound);
        if (CurrentBGMComponent)
        {
            CurrentBGMComponent->FadeIn(FadeInTime);
        }
    }
}

void UNovelStorySubsystem::StopBGM(float FadeOutTime)
{
    if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
    {
        CurrentBGMComponent->FadeOut(FadeOutTime, 0.0f);
        
        /// Sever the link. The component will destroy itself automatically when the fade-out completes, preventing memory leaks
        CurrentBGMComponent = nullptr;
    }
}

void UNovelStorySubsystem::PlaySFX(FName SFXKey)
{
    /// Data-driven approach: Consult the global configuration dictionary rather than hardcoding asset paths
    const UNovelStorySettings* Settings = GetDefault<UNovelStorySettings>();
    if (Settings && Settings->SFXBank.Contains(SFXKey))
    {
        const FNovelSoundConfig& SoundConfig = Settings->SFXBank[SFXKey];
        
        if (USoundBase* LoadedSound = SoundConfig.SoundAsset.LoadSynchronous())
        {
            UGameplayStatics::PlaySound2D(
                this, 
                LoadedSound, 
                1.0f, /// VolumeMultiplier
                1.0f, /// PitchMultiplier
                SoundConfig.StartTimeOffset /// Applies precise temporal offsets to trim 'dead air' from raw audio files
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot Find SFX: %s, Please Check Project Settings -> SFXBank"), *SFXKey.ToString());
    }
}