#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FDialogueNodeHandle.generated.h"

USTRUCT(BlueprintType)
struct FDialogueNodeHandle
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UDataTable> Table;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RowName;

    bool IsValid() const { return !Table.IsNull() && !RowName.IsNone(); }

    bool operator==(const FDialogueNodeHandle& Other) const
    {
        return Table == Other.Table && RowName == Other.RowName;
    }
};

FORCEINLINE uint32 GetTypeHash(const FDialogueNodeHandle& Handle)
{
    return HashCombine(GetTypeHash(Handle.Table.ToSoftObjectPath()), GetTypeHash(Handle.RowName));
}