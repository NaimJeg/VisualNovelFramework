#pragma once

#include "CoreMinimal.h"
#include "NovelValue.h"
#include "NovelExpression.generated.h"

UCLASS(BlueprintType)
class NOVELRUNTIME_API UNovelExpressionContext : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(const TMap<FName, FNovelValue>& InVariables) { Variables = InVariables; }

    UFUNCTION(BlueprintPure, Category = "Novel|Expression")
    bool GetVariable(FName VariableName, FNovelValue& OutValue) const;

private:
    UPROPERTY()
    TMap<FName, FNovelValue> Variables;
};

UCLASS(Abstract, EditInlineNew, Blueprintable)
class NOVELRUNTIME_API UNovelExpression : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, Category = "Novel|Expression")
    bool Evaluate(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const;
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const;
    virtual ENovelValueType GetStaticResultType() const { return ENovelValueType::None; }
};

UCLASS(DisplayName = "Expression: Literal")
class NOVELRUNTIME_API UNovelExpression_Literal : public UNovelExpression
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Expression") FNovelValue Value;
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return Value.Type; }
};

UCLASS(DisplayName = "Expression: Read Variable")
class NOVELRUNTIME_API UNovelExpression_ReadVariable : public UNovelExpression
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Expression") FName VariableName = NAME_None;
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
};

UCLASS(Abstract, EditInlineNew)
class NOVELRUNTIME_API UNovelBinaryExpression : public UNovelExpression
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Expression") TObjectPtr<UNovelExpression> Left;
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Expression") TObjectPtr<UNovelExpression> Right;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
    bool EvaluateOperands(const UNovelExpressionContext* Context, FNovelValue& OutLeft, FNovelValue& OutRight, FText& OutError) const;
};

UCLASS(DisplayName = "Expression: Equals")
class NOVELRUNTIME_API UNovelExpression_Equals : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Not Equals")
class NOVELRUNTIME_API UNovelExpression_NotEquals : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Less Than")
class NOVELRUNTIME_API UNovelExpression_LessThan : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Less Than Or Equal")
class NOVELRUNTIME_API UNovelExpression_LessThanOrEqual : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Greater Than")
class NOVELRUNTIME_API UNovelExpression_GreaterThan : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Greater Than Or Equal")
class NOVELRUNTIME_API UNovelExpression_GreaterThanOrEqual : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: And")
class NOVELRUNTIME_API UNovelExpression_And : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Or")
class NOVELRUNTIME_API UNovelExpression_Or : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
};

UCLASS(DisplayName = "Expression: Not")
class NOVELRUNTIME_API UNovelExpression_Not : public UNovelExpression
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Expression") TObjectPtr<UNovelExpression> Operand;
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override { return ENovelValueType::Bool; }
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

UCLASS(DisplayName = "Expression: Add")
class NOVELRUNTIME_API UNovelExpression_Add : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override;
};

UCLASS(DisplayName = "Expression: Subtract")
class NOVELRUNTIME_API UNovelExpression_Subtract : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override;
};

UCLASS(DisplayName = "Expression: Multiply")
class NOVELRUNTIME_API UNovelExpression_Multiply : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override;
};

UCLASS(DisplayName = "Expression: Divide")
class NOVELRUNTIME_API UNovelExpression_Divide : public UNovelBinaryExpression
{
    GENERATED_BODY()
public:
    virtual bool Evaluate_Implementation(const UNovelExpressionContext* Context, FNovelValue& OutValue, FText& OutError) const override;
    virtual ENovelValueType GetStaticResultType() const override;
};
