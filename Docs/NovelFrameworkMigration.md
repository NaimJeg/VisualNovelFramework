# Novel Framework Migration

## Enable the plugin

`NovelFramework` is enabled in `Novel.uproject`. Other projects can copy `Plugins/NovelFramework`, enable it, and add `NovelRuntime` and, optionally, `NovelUMG` to their module dependencies. `NovelEditor` and `NovelTests` load only in editor targets.

## Module direction

```text
Novel host -> NovelUMG -> NovelRuntime
Novel host ------------> NovelRuntime
NovelEditor ------------> NovelRuntime
NovelTests -------------> NovelRuntime
```

`NovelRuntime` has no UMG, Slate, UnrealEd, GraphEditor, AssetTools, or host-module dependency.

## Settings migration

Runtime data, audio configuration, save settings, and the entry story live in **Project Settings > Plugins > Novel Runtime** (`UNovelRuntimeSettings`). Widget classes live in **Novel Presentation** (`UNovelPresentationSettings`). The sample project retains `UNovelStorySettings` as a deprecated compatibility surface and mirrors its current config values into the new sections.

## Existing content and redirects

Per-type Core Redirects migrate moved classes, structs, and `ENovelRuntimeState` from `/Script/Novel` to `/Script/NovelRuntime` or `/Script/NovelUMG`. Open affected Data Assets and Widget Blueprints, compile them, and resave after confirming their parents and instanced intents resolve. Do not remove redirects until all shipped content and saves have been migrated.

## History widgets

1. Create a history screen Widget Blueprint derived from `UNovelHistoryScreenWidget`.
2. Add a `UListView` named `HistoryList` and an optional button named `Btn_Close`.
3. Create an entry Widget Blueprint derived from `UNovelHistoryEntryWidget` with `SpeakerText`, `DialogueText`, and optional `MetadataText` text blocks.
4. Set `HistoryList.Entry Widget Class` to that Blueprint and `Selection Mode` to `None`.
5. Make the popup root fill its parent and remain hit-testable so dialogue clicks cannot pass through.
6. Assign the history screen in Novel Presentation Settings.

`Txt_History` remains supported as a deprecated fallback. No binary Widget Blueprint was modified by this migration.

## Unified assets and graph editor

Create `Novel Story` and `Novel Chapter` assets from the Content Browser. Opening a chapter launches the graph editor. The graph is a transient projection of `UNovelChapterAsset::Nodes`; node data and editor positions are written back transactionally. Right-click to add nodes, use Delete or Duplicate, edit dialogue/actions/conditions in Details, connect `Next` and choice pins, and use `Validate` before saving. `Focus Target` follows a selected node's local Next or first choice target.

The `Legacy Conversion` toolbar command identifies the safe converter entry point. Use `UNovelLegacyChapterConverter::ConvertLegacyChapter` in an Editor Utility Blueprint to create a new chapter and inspect its structured report. The converter never overwrites the current chapter or invents missing targets. Legacy DataTable chapters remain supported by a load-time adapter during migration.

## Bootstrap and presentation

The sample project still uses `ANovelGameMode` and `UNovelRootScreen`. The plugin does not require either. `UNovelPresentationSubsystem` owns optional screen navigation and loading UI. Runtime loading broadcasts presentation-neutral state and never creates UMG widgets.

## Saves

Save schema 4 embeds `FNovelRuntimeSnapshot` while retaining schema 0/1 legacy fields and schema 2/3 identities. `UNovelSaveIndex` uses the reserved `__NovelSaveIndex` slot. Save menus and Continue read index metadata instead of loading every full save. Index repair imports valid legacy slots at subsystem initialization, removes missing entries, ignores corrupt saves, sorts by timestamp plus slot ID, and never deletes or overwrites unknown newer schemas.

`FNovelRuntimeSnapshot` can be embedded by a host game. Restore validates the snapshot before replacing runtime state and does not replay completed node actions.

## Headless runner and tests

`FNovelRunner` is a presentation-free in-memory progression service for explicit node traversal, choice filtering, action completion safety, history, variables, timeout control, and snapshot round trips. It accepts injected action execution and time through `Tick`, so tests require no viewport, UMG, audio device, asset streaming, or save files. The stabilized `UNovelStorySubsystem` remains the production facade; consolidating its async loading/presentation adapter onto `FNovelRunner` is intentionally deferred to avoid replacing proven runtime behavior in this migration.

## Deprecated compatibility APIs

- `UNovelStorySettings` and its combined settings remain temporarily.
- `FDialogueNodeHandle`, DataTable chapters, and branch assets remain available for legacy migration.
- `UNovelIntentBase::ExecuteIntent` remains as an adapter; new actions override `ExecuteAction` and use `UNovelActionContext`.
- Runtime UI-navigation wrappers on `UNovelStorySubsystem` remain deprecated; new UI uses `UNovelPresentationSubsystem` commands.
- `Txt_History` remains as the old history Widget Blueprint fallback.

Remove these only in a versioned compatibility release after redirected assets and old saves have been resaved and verified.