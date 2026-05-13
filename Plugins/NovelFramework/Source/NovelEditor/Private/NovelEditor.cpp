#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "NovelAssetTypeActions.h"
#include "NovelChapterEditor.h"

class FNovelEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FNovelChapterEditorCommands::Register();
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
        RegisteredAssetActions.Add(MakeShared<FNovelAssetTypeActions_Chapter>());
        RegisteredAssetActions.Add(MakeShared<FNovelAssetTypeActions_Story>());
        for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetActions)
        {
            AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
        }
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetTools")))
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
            for (const TSharedPtr<IAssetTypeActions>& Action : RegisteredAssetActions)
            {
                if (Action.IsValid())
                {
                    AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
                }
            }
        }
        RegisteredAssetActions.Reset();
        FNovelChapterEditorCommands::Unregister();
    }

private:
    TArray<TSharedPtr<IAssetTypeActions>> RegisteredAssetActions;
};

IMPLEMENT_MODULE(FNovelEditorModule, NovelEditor)