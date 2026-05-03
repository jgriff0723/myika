# Domain Docs

How the engineering skills should consume this repo's domain documentation when exploring the codebase.

## Before exploring, read these

- `CONTEXT.md` at the repo root, or
- `CONTEXT-MAP.md` at the repo root if it exists; it points at one `CONTEXT.md` per context. Read each one relevant to the topic.
- `docs/adr/` - read ADRs that touch the area you're about to work in.
- For this Unreal Engine project, use `myikai_plugin.uproject`, `Source/`, `Config/`, and `Content/` as the first-pass project map when domain docs do not exist yet.

If any of these files don't exist, proceed silently. Don't flag their absence and don't suggest creating them upfront. The producer skills create them lazily when terms or decisions actually get resolved.

## File structure

Single-context Unreal project (current default):

```text
/
|-- myikai_plugin.uproject       <- test harness project
|-- CONTEXT.md                   <- project domain glossary (Combat/Platforming/SideScrolling Epic variants)
|-- ROADMAP.md                   <- Phase 0/1/2/3 build plan
|-- Config/
|-- Content/
|   |-- Terrain/Haven.umap       <- canonical demo / integration test bench
|   |-- Water/                   <- Phase 0 starter water content
|   `-- ...
|-- Source/                      <- project module (Combat/Platforming/SideScrolling test fixtures)
|-- Plugins/
|   |-- Myika/                   <- THE deliverable (per ADR-0001 G1)
|   |   |-- Myika.uplugin
|   |   |-- README.md
|   |   |-- Source/
|   |   |   |-- MyikaCore/       <- shared subsystem, types, MPC sync
|   |   |   |-- MyikaSky/        <- V1 (Phase 2)
|   |   |   |-- MyikaWater/      <- V2 (Phase 2)
|   |   |   `-- ...              <- V3-V11
|   |   `-- Content/
|   |       |-- MyikaCore/       <- MPC_Myika, master material macros
|   |       |-- MyikaSky/        <- BP_MyikaSky, cloud presets, curves
|   |       `-- ...
|   `-- GaeaUnrealTools/         <- third-party (heightmap import workflow)
`-- docs/
    |-- adr/
    |   `-- 0001-myika-operating-model.md
    `-- agents/                  <- this folder
```

Multi-context expansion (only if `CONTEXT-MAP.md` is added later):

```text
/
|-- myikai_plugin.uproject
|-- CONTEXT-MAP.md
|-- docs/adr/                          <- system-wide decisions
`-- Source/
    |-- MyikaiPlugin/
    |   |-- CONTEXT.md
    |   `-- docs/adr/                  <- module-specific decisions
    `-- AnotherModule/
        |-- CONTEXT.md
        `-- docs/adr/
```

## Use the project's vocabulary

When your output names a gameplay, tooling, or engine concept, use the term as defined in `CONTEXT.md` once that file exists. Don't drift to synonyms that the project later standardizes away from.

If the concept you need is not documented yet, either the repo is still greenfield or there is a real domain-doc gap. Note that gap for a later planning/doc pass instead of inventing durable terminology casually.

## Flag ADR conflicts

If your output contradicts an existing ADR, surface it explicitly rather than silently overriding it:

> Contradicts ADR-0007 - but worth reopening because...
