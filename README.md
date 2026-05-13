# VisualNovelFramework-UnrealEngine5.7.4
Developed with Unreal Engine 5

#Features
1. Story Subsystem (UNovelStorySubsystem)
The beating heart of the framework. It operates as a global state machine that manages dialogue progression, chapter loading, and macro scene transitions. It runs entirely independent of UI widgets, ensuring your game logic never breaks when UI elements are destroyed or changed.

2. Data-Driven Narrative
Narrative content is strictly data-driven for easy localization and designer workflow:

Sequential Dialogue: Handled via Engine UDataTables.

Branching & Choices: Managed via Custom UDataAssets, mapping specific dialogue rows to player choices and branching paths.

Intent System: A queue-based action system (UNovelIntentBase) that fires visual (backgrounds, character sprites) and auditory effects perfectly synced with specific dialogue lines.

3. Advanced Audio Management
An architect-level audio pipeline utilizing Unreal's native USoundClass and USoundMix.

Centralized Bank: SFX and BGM assets are registered in the Project Settings, allowing designers to tweak assets and start-time offsets without opening blueprints.

Dynamic Routing: The Subsystem automatically intercepts audio playback and applies the correct SoundClassOverride, ensuring UI volume sliders work globally and instantly.

4. "Dumb View" UI Architecture
All UMG widgets (Dialogue Screen, Settings Screen, Save/Load Menu) are built as pure views. They do not hold asset references or execute heavy game logic. They merely reflect the data and pass user input (like slider releases or button clicks) back to the Subsystem.

#Usage
Global Configuration
Once the plugin/module is integrated, navigate to your Unreal Engine Editor:

Go to Edit -> Project Settings -> Game -> Novel Story Settings.
