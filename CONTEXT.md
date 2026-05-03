# myikai_plugin Context

This is a greenfield Unreal Engine 5.7 project currently based on Epic's template gameplay variants. Use this file as the shared project vocabulary before planning, writing issues, or changing gameplay code.

## Product Frame

`myikai_plugin` is the current project and runtime module name. Despite the folder name, the checked-in root is a `.uproject` game project, not a standalone `.uplugin` package yet.

The current codebase is a playable template foundation with these lanes:

- Third Person baseline: default map, default game mode, shared input and camera pattern.
- Combat variant: melee combat, HP, damage reactions, enemy AI, StateTree tasks, and combat interactables.
- Platforming variant: advanced third-person movement with dash, coyote time, double jump, and wall jump.
- Side Scrolling variant: side-view movement, soft platforms, pickups, interactables, NPC AI, and side-scrolling UI.

## Domain Glossary

Use these terms consistently in issues, tests, class names, and docs.

| Term | Meaning |
| --- | --- |
| Project | The Unreal project rooted at `myikai_plugin.uproject`. |
| Runtime module | The C++ module named `myikai_plugin` under `Source/myikai_plugin/`. |
| Gameplay variant | A self-contained template gameplay lane under `Variant_Combat`, `Variant_Platforming`, or `Variant_SideScrolling`. |
| Third Person baseline | The default starter experience in `Content/ThirdPerson` and the root `myikai_plugin*` C++ classes. |
| Player character | The pawn controlled by the local player for a variant. |
| Player controller | The input/mapping owner that installs Enhanced Input mapping contexts and dispatches to the player character. |
| Enhanced Input action | A `UInputAction` asset under `Content/**/Input/Actions` bound by a player controller or character. |
| Input mapping context | A `UInputMappingContext` asset such as `IMC_Default`, `IMC_Combat`, or `IMC_Platforming`. |
| Touch controls | Mobile UI widgets and virtual joystick support spawned by player controllers when touch input is active or forced. |
| AnimNotify | Animation event object that calls gameplay code at montage timing points, especially combat attack traces and combo checks. |
| Combo attack | A melee attack string advanced by cached attack input and montage section checks. |
| Charged attack | A hold-and-release melee attack that may loop a charge montage section before striking. |
| Attack trace | The combat sphere trace that detects damageable targets during an attack animation. |
| Danger notification | A warning sent to nearby combat damageables before a hit lands so AI can react. |
| Damageable | Any actor implementing `ICombatDamageable`; it can receive damage, healing, death, and danger notifications. |
| Attacker | Any actor implementing `ICombatAttacker`; it can perform attack traces and montage-driven attack checks. |
| Activatable | Any actor implementing `ICombatActivatable`; it can be activated, deactivated, or toggled by another actor. |
| Combat enemy | AI-controlled combat character using `ACombatEnemy`, `ACombatAIController`, StateTree, and combat interfaces. |
| Combat spawner | `ACombatEnemySpawner`, an activatable actor that creates combat enemies. |
| Life bar | UMG health display used by combat player/enemy actors via `UCombatLifeBar`. |
| Ragdoll death | Combat death behavior that enables physics and later respawns/removes the actor. |
| Coyote time | Short grace period after leaving ground where a jump is still allowed. |
| Wall jump | Movement action that traces for a nearby wall, applies directional impulse, and locks repeated wall jumps briefly. |
| Double jump | Extra air jump state tracked separately from wall jump state. |
| Dash | Platforming movement burst driven by `DashAction`, `DashMontage`, and `UAnimNotify_EndDash`. |
| Soft platform | A side-scrolling platform the player can pass through or drop through by changing collision response. |
| Side-scrolling interaction | Interface-driven interaction via `ISideScrollingInteractable::Interaction`. |
| StateTree utility | C++ task/condition structs exposed to StateTree assets for AI behavior. |
| Level prototyping asset | Reusable blockout mesh, material, or interaction asset under `Content/LevelPrototyping`. |
| External actors/objects | World Partition generated actor/object assets under `Content/__ExternalActors__` and `Content/__ExternalObjects__`. |

## Codebase Map

### Project and Module

- `myikai_plugin.uproject`: UE 5.7 project descriptor. Enables `StateTree`, `GameplayStateTree`, and editor modeling tools.
- `Source/myikai_plugin/myikai_plugin.Build.cs`: runtime module dependencies. Current dependencies include Enhanced Input, AI, StateTree, Gameplay StateTree, UMG, and Slate.
- `Source/myikai_plugin.Target.cs`: game target.
- `Source/myikai_pluginEditor.Target.cs`: editor target.

### Third Person Baseline

- `Source/myikai_plugin/myikai_pluginCharacter.*`: abstract third-person player character with spring arm, follow camera, and `DoMove`, `DoLook`, `DoJumpStart`, `DoJumpEnd` methods for input and UI callers.
- `Source/myikai_plugin/myikai_pluginPlayerController.*`: installs default Enhanced Input mapping contexts and spawns mobile touch controls.
- `Source/myikai_plugin/myikai_pluginGameMode.*`: baseline game mode.
- `Content/ThirdPerson`: default map and Blueprint subclasses for the baseline character/controller/game mode.

Callers and flow:

- `DefaultEngine.ini` points `GameDefaultMap`, `EditorStartupMap`, and `GlobalDefaultGameMode` at the Third Person baseline.
- Input assets in `Content/Input` feed the controller/character methods.

### Combat Variant

- `Variant_Combat/CombatCharacter.*`: player combat character. Owns HP, camera, combo attack, charged attack, damage, death, respawn, danger notification, and Blueprint effect hooks.
- `Variant_Combat/CombatPlayerController.*`: combat input mapping and touch input routing for the combat character.
- `Variant_Combat/CombatGameMode.*`: combat game mode.
- `Variant_Combat/Interfaces`: attack, damage, and activation contracts.
- `Variant_Combat/Animation`: AnimNotify classes that call attack trace, combo, and charged attack checks.
- `Variant_Combat/AI`: AI controller, combat enemy, enemy spawner, EQS contexts, and StateTree task/condition structs.
- `Variant_Combat/Gameplay`: combat world actors such as activation volumes, checkpoints, damageable boxes, dummies, and lava floor.
- `Variant_Combat/UI`: life bar widget class.
- `Content/Variant_Combat`: combat map, Blueprint subclasses, AI StateTree/EQS assets, input assets, UI, materials, animations, and VFX.

Callers and flow:

- Combat input calls character `Do*` methods.
- Attack montages call AnimNotifies.
- AnimNotifies call `ICombatAttacker` methods.
- Attack traces call `ICombatDamageable` targets.
- Danger traces notify enemies before impact.
- Combat enemies run StateTree tasks that call `ACombatEnemy` attack and movement helpers.
- Activation volumes call `ICombatActivatable` actors such as enemy spawners.

### Platforming Variant

- `Variant_Platforming/PlatformingCharacter.*`: advanced third-person movement character with dash, coyote time, double jump, wall jump, and jump trail Blueprint hook.
- `Variant_Platforming/PlatformingPlayerController.*`: platforming input mapping and touch input routing.
- `Variant_Platforming/PlatformingGameMode.*`: platforming game mode.
- `Variant_Platforming/Animation/AnimNotify_EndDash.*`: montage notify that exits dash state.
- `Content/Variant_Platforming`: platforming map, Blueprint subclasses, input assets, animation Blueprint, dash montage, and VFX.

Callers and flow:

- Input routes to `DoMove`, `DoLook`, `DoDash`, `DoJumpStart`, and `DoJumpEnd`.
- Movement mode changes and landing reset jump/dash state.
- Dash montage calls `UAnimNotify_EndDash`.

### Side Scrolling Variant

- `Variant_SideScrolling/SideScrollingCharacter.*`: side-view player character with horizontal movement, jump, drop-through platforms, wall jump, soft collision, and interaction.
- `Variant_SideScrolling/SideScrollingPlayerController.*`: side-scrolling input mapping and touch input routing.
- `Variant_SideScrolling/SideScrollingGameMode.*`: side-scrolling game mode and UI ownership.
- `Variant_SideScrolling/SideScrollingCameraManager.*`: side-scrolling camera manager.
- `Variant_SideScrolling/Interfaces/SideScrollingInteractable.*`: generic side-scrolling interaction contract.
- `Variant_SideScrolling/Gameplay`: moving platform, jump pad, pickup, and soft platform actors.
- `Variant_SideScrolling/AI`: NPC, AI controller, and StateTree player-targeting utility.
- `Variant_SideScrolling/UI`: side-scrolling UI widget class.
- `Content/Variant_SideScrolling`: side-scrolling map, Blueprint subclasses, input, AI, UI, and level assets.

Callers and flow:

- Input routes to movement, jump, drop, and interaction methods.
- Soft platforms toggle player collision based on overlap/drop state.
- Interactables expose `Interaction(AActor* Interactor)`.
- Side-scrolling StateTree tasks locate and validate the player target for NPC behavior.

## Asset and Config Anchors

- `Config/DefaultEngine.ini`: current default startup map and game mode.
- `Config/DefaultInput.ini`: Enhanced Input is the active input implementation.
- `Content/Input`: shared input actions, mapping contexts, and touch UI.
- `Content/LevelPrototyping`: reusable blockout, door, jump pad, target, mesh, material, and texture assets.
- `Content/__ExternalActors__` and `Content/__ExternalObjects__`: World Partition generated assets associated with maps.

## Naming Rules

- Preserve the existing class prefixes for current template lanes: `Combat`, `Platforming`, `SideScrolling`, and root `myikai_plugin`.
- Name new gameplay work after the player-facing mechanic or system, not the implementation detail.
- When extending a gameplay variant, keep variant-specific code inside that variant folder unless it is deliberately shared across variants.
- When creating shared gameplay behavior, introduce the shared abstraction under the module root only after at least two variants need the same contract.

## Validation Language

Use exact validation terms in tasks:

- Compile proof: editor target or game target builds successfully.
- Automation proof: named Unreal automation test or commandlet passes.
- PIE proof: behavior observed in Play-In-Editor.
- Asset proof: named Blueprint, map, widget, StateTree, or input asset exists and is wired to the expected class.
- Visual proof: screenshot or viewport observation confirms visible behavior.

Do not call visual, PIE, or asset acceptance criteria complete from compile proof alone.

## Open Questions

- Is `myikai_plugin` the durable product/module name, or only the initial project scaffold name?
- Is the long-term target a game project, an Unreal plugin, or both?
- Which gameplay variant is the first real vertical slice: Combat, Platforming, Side Scrolling, or a new Myika-specific lane?
- Should future AI work build on StateTree, Gameplay Ability System, or another runtime contract?
- Which maps and template assets should remain committed once the first real slice exists?
