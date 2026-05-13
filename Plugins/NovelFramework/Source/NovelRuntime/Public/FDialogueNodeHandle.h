#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FDialogueNodeHandle.generated.h"

USTRUCT(BlueprintType)
struct NOVELRUNTIME_API FDialogueNodeHandle
{
    GENERATED_BODY()

    FDialogueNodeHandle() = default;

    FDialogueNodeHandle(UDataTable* InTable, FName InRowName)
        : Table(InTable)
        , RowName(InRowName)
    {
    }

    FDialogueNodeHandle(TSoftObjectPtr<UDataTable> InTable, FName InRowName)
        : Table(InTable)
        , RowName(InRowName)
    {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UDataTable> Table;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RowName = NAME_None;

    bool IsValid() const { return !Table.IsNull() && !RowName.IsNone(); }

    bool IsLoaded() const { return Table.Get() != nullptr && !RowName.IsNone(); }

    bool operator==(const FDialogueNodeHandle& Other) const
    {
        return Table == Other.Table && RowName == Other.RowName;
    }

    bool operator!=(const FDialogueNodeHandle& Other) const
    {
        return !(*this == Other);
    }
};

FORCEINLINE uint32 GetTypeHash(const FDialogueNodeHandle& Handle)
{
    return HashCombine(GetTypeHash(Handle.Table.ToSoftObjectPath()), GetTypeHash(Handle.RowName));
}