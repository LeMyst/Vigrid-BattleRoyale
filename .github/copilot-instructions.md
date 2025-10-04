# Copilot Instructions for Vigrid Battle Royale

## Repository Overview

This is a **DayZ Battle Royale mod** that adds battle royale gameplay mechanics to the DayZ survival game. It's a community mod originally created by Kegan and modified by Myst, licensed under DSPL-SA (DayZ Standalone Public License Share Alike).

### High-Level Repository Information
- **Project Type**: DayZ mod (Bohemia Interactive engine)
- **Languages**: EnfusionScript (.c files - C-like syntax specific to DayZ)
- **Size**: ~240 files total, 94 script files (~11,600 lines of code), medium-sized mod
- **Version**: 0.1.0-Vigrid (defined in `Scripts/Client/2_GameLib/BattleRoyaleConstants.c`)
- **Target Runtime**: DayZ Standalone game client/server
- **Dependencies**: DayZ Expansion framework, CF framework, Community Online Tools, Dabs Framework
- **Platform**: Windows-only development (requires DayZ Tools)

## Build Instructions

**CRITICAL**: This mod requires Windows with DayZ Tools installed. Linux/macOS development is not supported.

### Prerequisites
1. **DayZ Tools** installed via Steam (under Tools in Steam Library)
2. **P:\ drive mount** - This is MANDATORY. The entire DayZ modding toolchain requires P:\ to be mounted to your DayZ working directory
3. **Project configuration files**:
   - `Workbench/project.cfg` (provided)
   - `Workbench/user.cfg` (copy from `user_sample.cfg` and modify paths)

### Required Setup Steps
1. **Always** ensure P:\ drive is mounted before any build operation
2. **Always** configure `user.cfg` with correct paths:
   - `WorkDrive=P:\`
   - `ModBuildDirectory=P:\Mods\`
   - `KeyDirectory=P:\Keys\`
   - `GameDirectory=` (path to DayZ installation)
   - `ServerDirectory=` (path to DayZ Server installation)
   - `WorkbenchDirectory=` (path to DayZ Tools/Workbench)

### Build Commands (Execute from `Workbench/Batchfiles/` directory)

#### Main Build Process
```batch
Deploy.bat
```
- This is the primary build command
- Calls `CI.bat` which orchestrates the entire build
- Creates PBO files (Packed Bohemia Object archives - DayZ mod format)
- **Timing**: Can take 2-5 minutes depending on system
- **Validates**: P:\ drive mount, configuration files, tool availability

#### Build Validation Steps
1. Check for P:\ drive: `if not exist "P:\" ( echo Mount P: drive first & exit /b 1 )`
2. Verify configuration: Build will fail if `project.cfg` or `user.cfg` missing
3. Monitor build logs in `Workbench/Logs/` directory
4. Success indicator: `Workbench/Logs/Build.success` file created
5. Failure indicator: `Workbench/Logs/Build.failure` file created

#### Testing/Launch Commands
```batch
LaunchOffline.bat     # Test mod in single-player
LaunchLocalMP.bat     # Test mod in local multiplayer
LaunchServer.bat      # Start dedicated server with mod
```

#### Utility Commands
```batch
KillGame.bat         # Kill all DayZ processes
ClearLogs.bat        # Clear build logs
SetupLaunch.bat      # Validate configuration (run before launch)
```

### Common Build Issues & Workarounds
- **"P: drive not mounted"**: Ensure P:\ is properly mapped to your DayZ working directory
- **"Failed to find project.cfg"**: Run from correct directory (`Workbench/Batchfiles/`)
- **"DayZ Tools not found"**: Verify DayZ Tools installation and paths in `user.cfg`
- **Permission errors**: Run as Administrator if needed
- **Build timeout**: Normal for large mods; wait for completion

## Project Layout & Architecture

### Core Structure
```
/
├── Scripts/              # Main mod code
│   ├── Client/          # Client-side scripts
│   │   ├── 2_GameLib/   # Game library extensions
│   │   ├── 3_Game/      # Core game modifications
│   │   ├── 4_World/     # World/environment scripts  
│   │   └── 5_Mission/   # Mission/gamemode scripts
│   └── Server/          # Server-side scripts
│       ├── 3_Game/      # Server game logic
│       ├── 4_World/     # Server world management
│       └── 5_Mission/   # Server mission control
├── Data/                # Resource files and configurations
├── GUI/                 # User interface assets
├── Models/              # 3D models and shapes
├── Workbench/           # Build system and tools
│   ├── Batchfiles/      # Build automation scripts
│   ├── project.cfg      # Project configuration
│   └── user_sample.cfg  # User configuration template
└── Extra/               # Optional mod components
```

### Key Configuration Files
- **`mod.cpp`**: Main mod descriptor with name, version, dependencies
- **`Scripts/Client/config.cpp`**: Client-side mod configuration and patches
- **`Scripts/Server/config.cpp`**: Server-side mod configuration
- **`Scripts/Client/2_GameLib/BattleRoyaleConstants.c`**: Version and debug constants
- **`Workbench/project.cfg`**: Build project settings (ModName, dependencies, launch params)
- **`Workbench/user.cfg`**: User-specific paths and settings (copy from user_sample.cfg)

### Main Code Entry Points
- **`Scripts/Client/3_Game/DayZGame.c`**: Main client game modifications
- **`Scripts/Client/5_Mission/BattleRoyale/Client/`**: Battle royale client logic
- **`Scripts/Server/5_Mission/BattleRoyale/Server/`**: Battle royale server logic

### Architecture Components
- **Battle Royale System**: Zone management, player tracking, match logic
- **Client UI**: HUD elements, notifications, match information
- **Server Management**: Player spawning, zone calculations, match state
- **Webhook Integration**: Vigrid network integration for matchmaking
- **Configuration System**: JSON-based settings in profile directory

### Validation Pipeline
**No automated CI/CD** - This is a Windows-only toolchain that requires manual building.

To validate changes:
1. Build the mod successfully (`Deploy.bat`)
2. Test in single-player (`LaunchOffline.bat`) 
3. Test core functionality (zone system, spawning, UI)
4. Check for script errors in game logs
5. Test in multiplayer environment if possible

### Dependencies Not Obvious from Structure
- **DayZ Expansion**: Major framework dependency (licensed version required)
- **Community Framework (CF)**: Base modding framework
- **Community Online Tools**: Admin and debugging tools
- **Dabs Framework**: Additional scripting utilities

### Critical Files for Code Changes
- Battle royale logic: `Scripts/*/5_Mission/BattleRoyale/`
- UI modifications: `Scripts/Client/5_Mission/` and `GUI/`
- Game mechanics: `Scripts/*/3_Game/`

## Important Notes for Coding Agents

1. **Always validate P:\ drive mount** before attempting builds
2. **EnfusionScript syntax** is C-like but has DayZ-specific conventions:
   - **Does NOT support ternary operators** (`condition ? trueValue : falseValue`)
   - Use if-else statements instead of ternary operators
3. **Never modify working batch files** without understanding the build pipeline
4. **Client/Server separation** is critical - changes must go in correct directories
5. **Dependencies are strict** - respect the mod loading order in configs
6. **Test incrementally** - build and test small changes before major modifications
7. **UI changes** require both script and GUI asset modifications

### Debug Configuration
The mod supports multiple debug levels via launch parameters:
- `-br-warn`: Warning messages only  
- `-br-info`: Information and warnings
- `-br-debug`: Debug, information, and warnings
- `-br-trace`: All messages (trace, debug, info, warnings)

### Trust These Instructions
Only search for additional information if these instructions are incomplete or found to be incorrect. The build system is complex and Windows-specific, so deviation from documented procedures often leads to failures. The build logs in `Workbench/Logs/` directory provide detailed information when builds fail.