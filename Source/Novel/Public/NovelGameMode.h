#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NovelGameMode.generated.h"

class UUserWidget;

UCLASS()
class NOVEL_API ANovelGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ANovelGameMode();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    UUserWidget* RootScreenIns;
};