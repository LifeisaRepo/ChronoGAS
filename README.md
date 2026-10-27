# Chrono Recall

A time-rewind ability for Unreal Engine 5.3, built with C++ and the Gameplay Ability System (GAS). The character records the last 3 seconds of movement. On activation it sets off a blast at its current position, rewinds along the recorded path and recovers the health it lost during those 3 seconds.

[![Chrono Recall gameplay video](https://img.youtube.com/vi/3Vs4yQPH6fc/maxresdefault.jpg)](https://youtu.be/3Vs4yQPH6fc)

**[Watch on YouTube](https://youtu.be/3Vs4yQPH6fc)** · **[Download the Windows build](https://github.com/LifeisaRepo/ChronoGAS/releases)**

## Features

- **Rewind:** the last 3 seconds are recorded into a fixed-size ring buffer, and the rewind plays back along that path.
- **Departure blast:** radius, damage and knockback grow with the length of the rewound path.
- **Health restore:** heals the health lost since the oldest recorded moment.
- **Rewind effects:** slow motion, a pulse of field of view and chromatic aberration, the camera turning toward the blast, afterimages and a body glow.
- **Debug view:** a path trail coloured by speed, and a live telemetry overlay.
- **Network-ready:** the ability is locally predicted, and damage and healing are applied on the server.

## How it works

| Step | What happens | Code |
|---|---|---|
| Record | Every 0.05 s, the character's transform, velocity and health are written into a 60-slot ring buffer. | `UTemporalBufferComponent` |
| Activate | `E` activates the ability by its `Ability.ChronoRecall` tag. Activation is blocked during the 3 s cooldown, or if the recorded path is shorter than 25 cm. | `UGA_ChronoRecall` |
| Detonate | Pawns within a sphere around the starting point take damage and are launched outward. Power is the path length divided by `DistanceForFullPower`. Damage is passed to the gameplay effect as a Set by Caller magnitude. | `UGA_ChronoRecall::Detonate` |
| Rewind | A ticking ability task switches the character to flying, disables collision, and moves it back through the snapshots. The same task drives the camera, slow motion and afterimages. | `UChronoRewind` |
| Restore | At the oldest snapshot the character walks (or falls) again with the velocity recorded there, and the server heals the difference in health. | `UGA_ChronoRecall::OnRewindFinished` |

**Networking:**
- The server and the owning client each record their own buffer.
- The client rewinds immediately, and the server runs its own rewind from its own buffer.
- The server doesn't apply client movement corrections during the rewind.

## Getting started

### Play the build

1. Download the zip from [Releases](https://github.com/LifeisaRepo/ChronoGAS/releases), extract it, and run `ChronoGAS.exe`.
2. If the game doesn't start, run `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe` from the extracted folder.

You need 64-bit Windows 10 or 11 and a DirectX 12 GPU that supports Shader Model 6.

### Build from source

You need Unreal Engine 5.3 and Visual Studio 2022 with the *Game development with C++* workload.

1. Clone the repository with Git LFS.
2. Right-click `ChronoGAS.uproject` and choose **Generate Visual Studio project files**.
3. Open `ChronoGAS.sln`, then build **Development Editor | Win64**.
4. Open the project and press Play. `ThirdPersonMap` is the default map.

## Controls

| Input | Action |
|---|---|
| W A S D | Move |
| Mouse | Look |
| Space | Jump |
| Left Shift | Sprint |
| E | Chrono Recall |
| ` | Console |

Enter `Chrono.DrawPath 0` or `Chrono.DrawPath 1` in the console to hide or show the path trail.

## Project structure

| Path | Contents |
|---|---|
| `Source/ChronoGAS/Public/TemporalBufferComponent.h` | Ring buffer recorder and path trail |
| `Source/ChronoGAS/Public/AbilitySystem/GA_ChronoRecall.h` | The ability: activation checks, blast and heal |
| `Source/ChronoGAS/Public/AbilitySystem/AbilityTasks/ChronoRewind.h` | Ticking task that runs the rewind and its effects |
| `Source/ChronoGAS/Public/AbilitySystem/ChronoGhost*.h` | Pooled afterimages |
| `Source/ChronoGAS/Public/AbilitySystem/ChronoAttributeSet.h` | `Health`, `MaxHealth` and `IncomingDamage` attributes |
| `Source/ChronoGAS/Public/UI/ChronoTelemetryWidget.h` | Telemetry overlay and health bar data |
| `Source/ChronoGAS/ChronoGASCharacter.h` | Ability system setup, input, dummy ragdoll |
| `Content/AbilitySystem/` | Ability Blueprint, gameplay effects, materials and widgets |

## Known limitations

- The packaged build is single-player. Multiplayer was tested in the editor with 2 PIE clients.
- Slow motion only runs in single-player.
- Under high latency, the character can snap slightly at the end of the rewind.
- Reaching 0 health has no effect on the player.
- Dummies killed by the blast stay down until the game restarts.

## Credits

- Character, animations and input are from the Unreal Engine Third Person template.
- The UI uses the Sansation font.

## License

Copyright (c) 2026 Sanjyot Dahale. All rights reserved.
