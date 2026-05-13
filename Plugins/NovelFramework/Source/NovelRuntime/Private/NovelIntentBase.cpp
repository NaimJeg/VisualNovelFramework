// Fill out your copyright notice in the Description page of Project Settings.

#include "NovelIntentBase.h"
#include "NovelActionContext.h"
#include "NovelStorySubsystem.h"

void UNovelIntentCompletionProxy::Initialize(UNovelStorySubsystem* InStorySubsystem, int32 InSessionId, int32 InNodeExecutionId, int32 InIntentExecutionId)
{
    StorySubsystem = InStorySubsystem;
    SessionId = InSessionId;
    NodeExecutionId = InNodeExecutionId;
    IntentExecutionId = InIntentExecutionId;
    bCompleted = false;
}

void UNovelIntentCompletionProxy::Complete()
{
    if (bCompleted)
    {
        return;
    }

    bCompleted = true;

    if (StorySubsystem)
    {
        StorySubsystem->CompleteIntentFromProxy(SessionId, NodeExecutionId, IntentExecutionId);
    }
}

void UNovelIntentBase::ExecuteAction_Implementation(UNovelActionContext* Context)
{
    if (!Context)
    {
        return;
    }

    FNovelIntentCompleted Completion;
    Completion.BindDynamic(Context, &UNovelActionContext::Complete);
    ExecuteIntent(Context->GetStorySubsystemForLegacy(), Completion);
}
void UNovelIntentBase::ExecuteIntent_Implementation(UNovelStorySubsystem* StorySys, const FNovelIntentCompleted& Completion)
{
    Completion.ExecuteIfBound();
}