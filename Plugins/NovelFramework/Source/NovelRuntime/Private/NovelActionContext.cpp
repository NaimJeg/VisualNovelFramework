#include "NovelActionContext.h"

#include "NovelStorySubsystem.h"

void UNovelActionContext::Initialize(UNovelStorySubsystem* InStorySubsystem, int32 InSessionId, int32 InNodeExecutionId, int32 InActionExecutionId)
{
    StorySubsystem = InStorySubsystem;
    SessionId = InSessionId;
    NodeExecutionId = InNodeExecutionId;
    ActionExecutionId = InActionExecutionId;
    bFinished = false;
}

UWorld* UNovelActionContext::GetWorld() const
{
    return StorySubsystem ? StorySubsystem->GetWorld() : nullptr;
}

void UNovelActionContext::Complete()
{
    if (bFinished)
    {
        return;
    }
    bFinished = true;
    if (StorySubsystem)
    {
        StorySubsystem->CompleteIntentFromProxy(SessionId, NodeExecutionId, ActionExecutionId);
    }
}

void UNovelActionContext::Fail(const FText& ErrorMessage)
{
    if (bFinished)
    {
        return;
    }
    bFinished = true;
    if (StorySubsystem)
    {
        StorySubsystem->FailActionFromContext(SessionId, NodeExecutionId, ActionExecutionId, ErrorMessage);
    }
}

void UNovelActionContext::RequestTransition(FNovelNodeRef TargetNode, bool bClearScreen)
{
    if (!bFinished && StorySubsystem)
    {
        StorySubsystem->RequestNodeTransitionByRef(TargetNode, bClearScreen);
    }
}

bool UNovelActionContext::GetVariable(FName VariableName, FNovelValue& OutValue) const
{
    return !bFinished && StorySubsystem && StorySubsystem->GetVariable(VariableName, OutValue);
}

bool UNovelActionContext::SetVariable(FName VariableName, FNovelValue Value, FText& OutError)
{
    if (bFinished || !StorySubsystem)
    {
        OutError = FText::FromString(TEXT("Action context is no longer active."));
        return false;
    }
    return StorySubsystem->SetVariable(VariableName, Value, OutError);
}

bool UNovelActionContext::RemoveVariable(FName VariableName)
{
    return !bFinished && StorySubsystem && StorySubsystem->RemoveVariable(VariableName);
}
void UNovelActionContext::SetBackground(TSoftObjectPtr<UTexture2D> Background)
{
    if (!bFinished && StorySubsystem)
    {
        StorySubsystem->SetBackground(Background);
    }
}

void UNovelActionContext::ShowCharacter(FName CharacterID, TSoftObjectPtr<UTexture2D> Sprite, float XPosition)
{
    if (!bFinished && StorySubsystem)
    {
        StorySubsystem->ShowCharacter(CharacterID, Sprite, XPosition);
    }
}

void UNovelActionContext::HideCharacter(FName CharacterID)
{
    if (!bFinished && StorySubsystem)
    {
        StorySubsystem->HideCharacter(CharacterID);
    }
}

void UNovelActionContext::PlayMusic(TSoftObjectPtr<USoundBase> Music, float FadeInTime)
{
    if (!bFinished && StorySubsystem)
    {
        StorySubsystem->PlayBGM(Music, FadeInTime);
    }
}

void UNovelActionContext::StopMusic(float FadeOutTime)
{
    if (!bFinished && StorySubsystem)
    {
        StorySubsystem->StopBGM(FadeOutTime);
    }
}
