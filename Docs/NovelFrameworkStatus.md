# Novel Framework Implementation Status

## Baseline

The original project had one `Novel` module combining story runtime, persistence, audio, loading, and UMG. Runtime-state guards, deferred jumps, stale callback checks, async saves, history persistence, fade handling, and validation already existed and were preserved. Known defects included non-virtualized history, loading-owned UMG, mixed settings, DataTable row-order progression, synchronous save-menu scans, no variables/expressions, no unified assets, no plugin modules, no graph editor, and no automation module.

## Phase gates

| Phase | Status | Key result | Build | Tests |
|---|---|---|---|---|
| 0 Baseline | Complete | Audited project and established UE 5.7.4 baseline | Editor passed | Not present |
| 1 History | Complete in C++; manual WBP layout remains | Virtualized list, typed root ownership, restore without side effects | Game passed | Manual UI required |
| 2 Plugin | Complete | Runtime, UMG, editor, and test modules with per-type redirects | Game/editor passed | Data validation passed |
| 3 Presentation | Complete | Presentation subsystem owns UI routing/loading overlay | Game/editor passed | Static delegate audit |
| 4 Unified assets | Complete with legacy adapter | Story/chapter/node assets, explicit targets, converter, validation | Game/editor passed | Data validation passed |
| 5 Variables | Complete | Typed values, read-only expressions, variable actions and conditional choices | Editor passed | `Novel.Runtime.ValuesAndExpressions` passed |
| 6 Persistence | Complete | Snapshot schema 1, save schema 4, indexed metadata, repair/import | Game/editor passed | `Novel.Persistence.SnapshotAndIndex` passed |
| 7 Graph editor | Complete in C++; interactive asset QA remains | Factories, asset actions, transient graph, toolkit, Details, undo, validation | Editor passed | Manual editor checklist required |
| 8 Test harness | Partial production adoption | Headless runner and five total test groups; subsystem still uses stabilized execution path | Editor passed | Five tests passed |

## Public runtime surfaces

- `UNovelStorySubsystem`: production start/stop/advance/choice/save/load/history/variables/presentation-state facade.
- `UNovelStoryAsset`, `UNovelChapterAsset`, `FNovelNode`, `FNovelChoice`, `FNovelNodeRef`: explicit authored model.
- `UNovelIntentBase`, `UNovelActionContext`: compatible action API with exactly-once completion and deferred transitions.
- `FNovelValue`, `UNovelExpression`, built-in expressions and variable actions.
- `FNovelRuntimeSnapshot`, `UNovelSaveIndex`, `FNovelSaveSlotMetadata`.
- `FNovelRunner`: headless deterministic progression and test service.

## Verification notes

The editor and game targets compile with UnrealHeaderTool warnings treated as errors. Project data validation checks 45 assets with zero errors. Eight warnings are pre-existing references to disabled ACL compression-setting assets. The commandlet also emits two generic `LogAutomationTest: Error: Condition failed` lines during engine startup before test discovery; all discovered Novel tests subsequently report success and the command exits zero.

## Remaining work

- Perform interactive graph editor QA for drag/move/save/reopen, multi-node duplication, undo/redo, and cross-chapter target editing.
- Replace the graph toolbar's converter guidance with a full source-selection wizard if non-programmer conversion is required.
- Route the production subsystem's async adapters through `FNovelRunner` after parity tests cover streaming, audio, fade, and UMG event ordering.
- Add file-backed persistence automation for corrupt slot import, index repair, legacy schema migration, and Continue-newest selection.
- Add optional UMG smoke tests for history list counts and presentation binding lifecycle.
- Resave redirected Blueprint/Data Assets and complete the documented history Widget Blueprint layout.
- Consider a persistence backend interface only when a host needs cloud, console, or unified RPG saves.