#include "NovelGameUserSettings.h"
#include "NovelStorySubsystem.h"
#include "Kismet/GameplayStatics.h"

UNovelGameUserSettings::UNovelGameUserSettings()
{
    /// Initialize default data values
    MasterVolume = 1.0f;
    BGMVolume = 1.0f;
    SFXVolume = 1.0f;
    TextSpeed = 1.0f;
}

UNovelGameUserSettings* UNovelGameUserSettings::GetNovelGameUserSettings()
{
    /// Safely cast the engine's generic settings object to our custom class
    return Cast<UNovelGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UNovelGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
    /// 1. Execute engine-native applications (Resolution, Window Mode, VSync)
    Super::ApplySettings(bCheckForCommandLineOverrides);

    /// 2. Execute custom applications (Audio, VN Mechanics)
    /// We route this to the Subsystem, because the Subsystem holds the actual Audio Assets (SoundMix, etc.)
    /// The Data Class itself shouldn't load assets; it just tells the system to apply the data.
    
    // UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull);
    // if (World)
    // {
    //     if (UNovelStorySubsystem* StorySys = World->GetGameInstance()->GetSubsystem<UNovelStorySubsystem>())
    //     {
    //         StorySys->ApplyAudioSettingsFromData(MasterVolume, BGMVolume, SFXVolume);
    //     }
    // }
}