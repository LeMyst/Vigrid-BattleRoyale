//! Offline single-player test mission. Deployed into %GameDirectory%Missions\ by SetupMod.bat and
//! pointed at by SPMission in Workbench/user.cfg (`.\Missions\empty.ChernarusPlus`, relative because
//! LaunchOffline.bat starts the game with cwd = GameDirectory).
//!
//! The point of this file is what it does NOT do: it never returns a MissionServer for an offline
//! session. Vanilla's own factory (P:\scripts\5_mission\somemission.c:7) returns MissionServer only
//! for a real multiplayer server and MissionGameplay otherwise, and MissionGameplay is what
//! Community-Online-Tools hooks to create and select an offline character -
//! JM/COT/Scripts/5_Mission/CommunityOnlineTools/MissionGameplay.c:189 OnMissionStart ->
//! IsMissionOffline() -> OfflineMissionStart(). That gives a random spawn at a CfgWorlds
//! ChernarusPlus Names location plus a fixed loadout with the knife, Magnum, Shovel and Hatchet
//! already bound to quickbar slots 0-3.
//!
//! An unconditional `return new CustomMission()` - which is what every dayzOffline.* mission and
//! every BR-MissionFiles mission does, CustomMission being a MissionServer - means MissionGameplay
//! is never constructed, nobody is ever selected, and the session parks on the "IDLE MODE ACTIVE"
//! banner with no character to control. The tells are no MissionGameplay::MissionGameplay in
//! script_*.log, and `[IdleMode] Entering IN` in the .RPT with no matching `Leaving OUT`.
//!
//! No CreateHive() here, so no loot and no infected spawn - which keeps the load fast for
//! client-side work. If a test needs the economy, copy dayzOffline.chernarusplus into
//! Workbench\Missions\ instead and change only its CreateCustomMission to the form below.
//!
//! If $profile:ExpansionMod\Loadouts\AdminLoadout.json exists, COT applies that loadout instead of
//! the one described above (MissionGameplay.c:87).
//!
//! The world comes from the folder suffix, so a second map needs only a copy of this folder renamed
//! (empty.Enoch, empty.Sakhal, ...) - SetupMod.bat deploys every folder it finds here.

void main()
{
}

Mission CreateCustomMission(string path)
{
	if (GetGame().IsMultiplayer() && GetGame().IsServer())
		return new MissionServer();

	return new MissionGameplay();
}
