# Slavic first-person movement in Unreal Engine 5.4

## Connect the player Blueprints

1. Close the editor and build the `yellowhouseEditor` target for **Development Editor | Win64**. The runtime module declares `Core`, `CoreUObject`, `Engine`, `InputCore`, and `EnhancedInput` in `Source/yellowhouse/yellowhouse.Build.cs`. The project already selects Enhanced Player Input and Enhanced Input Component in its input settings.
2. Open `/Game/FirstPerson/BP_SlavicController`. In **Class Settings → Parent Class**, choose **SlavicPlayerController**, then compile. Add `BP_MenuControllerComponent` in **Components** if it is missing. The configured controller has one instance of this component.
3. Open `/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter` and reparent it to **SlavicCharacter**. Remove its original **IA_Move**, **IA_Look**, and **IA_Jump** event bindings. In Components, disable **Auto Activate** on the old camera and parent the first-person arms to the inherited **SlavicFirstPersonCamera**, adjusting their relative transform in the viewport. This keeps the character asset used by the project’s existing references. Compile both Blueprints.
4. On `PlayLevel`, open **World Settings → GameMode Override**. `BP_PlayLevelGameMode` uses `BP_FirstPersonCharacter` as its **Default Pawn Class** and `BP_SlavicController` as its **Player Controller Class**.

## Assign Enhanced Input assets

In `BP_SlavicController` under **Class Defaults → Slavic → Input**, assign `IMC_CharacterOnFoot` and its actions from `/Game/MenuSystemPro/Blueprints/Input/CharacterOnFoot`. Assign `IA_Move`, `IA_Look`, `IA_Look_Gamepad`, `IA_Jump`, `IA_Sprint`, and `IA_Crouch` to the matching fields. Assign Menu System Pro’s `IA_Crawl` to the **IA Prone** field so the existing Crawl setting controls the prone stance. Leave **Bind Native Gameplay Input** enabled and **Install Native Gameplay Mapping** disabled. Menu System Pro registers the contexts and applies player key profiles.

Under **Slavic → Menu**, assign the `IA_Pause` from `AlwaysAllowed` and set **Menu Controller Component Class** to `BP_MenuControllerComponent`. The controller calls the component’s `OpenPauseMenu` with its active `GlobalMenuConfig.IngameMenuClass` when the Pause action completes. Escape is the default pause key. Menu System Pro owns menu navigation, resume, and keybinding settings.

PlayLevel defaults are **WASD** to move, **Left Shift** to sprint, **C** or **Left Ctrl** to crouch, **X** to go prone, and **Space** to jump or request a taller stance. Use **Options → Controls** in the in-game menu to rebind these actions. The active Menu System Pro configuration sets **Single Player Pause Behavior** to **Never Pause**, so gameplay continues in the background while the menu is open.

On a gamepad, use the left stick to move, the right stick to look, left stick click to sprint, **Circle/B** to crouch, right stick click to go prone, **Cross/A** to jump, and **Options/Menu** to open the menu. Win64 uses the project's `WindowsDualsense_ds5w` plugin. In a standalone session with one local player, the Slavic controller assigns a lone connected Sony gamepad to that player if the plugin put it on another platform user.

To route gameplay events in Blueprint, disable **Bind Native Gameplay Input**, cast **Get Controlled Pawn** to **SlavicCharacter**, and use the following calls. A controller without Menu System Pro can enable **Install Native Gameplay Mapping** to register its own context.

| Action | Started or Triggered | Completed and Canceled |
| --- | --- | --- |
| Move | Triggered → `ApplyMoveInput(Action Value)` | `ApplyMoveInput((0, 0))` |
| Look | Triggered → `ApplyLookInput(Action Value)` | |
| Jump | Started → `RequestJump()` | `ReleaseJump()` |
| Sprint | Started → `RequestSprint(true)` | `RequestSprint(false)` |
| Crouch | Started → `RequestCrouch(true)` | `RequestCrouch(false)` |
| Prone | Started → `RequestProne()` | |

Use `RequestStand()` wherever gameplay needs to request an upright stance. The stance requests perform the same clearance checks for either input path. Use them in place of the engine’s built-in `Crouch` and `UnCrouch` nodes.

## Connect the Animation Blueprint and test

1. In `ABP_SlavicCharacter` or the Animation Blueprint assigned to your mesh, cast **Try Get Pawn Owner** to **SlavicCharacter** in **Blueprint Initialize Animation** and cache the reference. In **Blueprint Update Animation**, read `GetCurrentStance()` for the last reached stance, `GetTargetStance()` for the destination, and `GetStanceAlpha()` for the capsule’s progress toward that destination. Use the stance enum in your locomotion state machine and add `IsSprinting()`, `IsCrouching()`, or `IsProning()` where a transition rule needs a Boolean.
2. Set character Class Defaults under **Slavic | Stance**, **Camera**, and **Sprint**. By default the capsule has a 34 cm radius and standing, crouching, and prone half-heights of 88, 44, and 34 cm. Sprint shares the standing shape. The eye offsets are relative to the capsule center. Resizing preserves the capsule’s bottom position.
3. In a test map, verify that sprint stops in the air and when backing up. Try standing from prone beneath a ceiling that allows crouch only, then test under a moving ceiling. Ensure ceilings **Block Pawn** or otherwise block the character’s capsule collision profile. Implement **On Stance Change Blocked** in the character Blueprint if feedback is needed.

The custom stance transitions are designed for local play. Networked prediction and replication of stance and capsule dimensions require additional Character Movement saved-move support.
