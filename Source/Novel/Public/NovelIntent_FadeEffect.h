// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NovelIntentBase.h"
#include "TimerManager.h" 
#include "NovelStorySubsystem.h"
#include "NovelIntent_FadeEffect.generated.h"

UCLASS(DisplayName = "Intent: Fade Effect")
class NOVEL_API UNovelIntent_FadeEffect : public UNovelIntentBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Effect")
    FLinearColor FadeColor = FLinearColor::Black;

    UPROPERTY(EditAnywhere, Category = "Effect")
    float FadeDuration = 1.0f;

    virtual void ExecuteIntent(UNovelStorySubsystem* StorySys, FOnIntentFinished OnFinished) override
    {
        if (StorySys)
        {
            // �㲥�� UI �㣬�� DialogueScreen �ϵ�һ��ȫ�� Border ������Ŷ���
            StorySys->OnScreenFadeEvent.Broadcast(FadeColor, FadeDuration);

            // �����Ҫ�ȴ�����������ִ����һ�� Intent ����ʾ̨��
            if (bWaitUntilFinished && StorySys->GetWorld())
            {
                FTimerHandle TimerHandle;
                StorySys->GetWorld()->GetTimerManager().SetTimer(
                    TimerHandle,
                    FTimerDelegate::CreateLambda([OnFinished]()
                        {
                            OnFinished.ExecuteIfBound();
                        }),
                    FadeDuration,
                    false
                );
                return; // ��ǰ return��������������ִ��
            }
        }

        // �������Ҫ�ȴ������� StorySys ��Ч��ֱ�ӽ���
        OnFinished.ExecuteIfBound();
    }
};