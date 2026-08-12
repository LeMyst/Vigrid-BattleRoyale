#ifndef SERVER
modded class MissionBase
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
#ifndef NO_GUI
		if (id == MENU_SPAWN_SELECTION)
		{
			BattleRoyaleUtils.Trace("CreateScriptedMenu: MENU_SPAWN_SELECTION");
			UIScriptedMenu menu = new SpawnSelectionMenu;
			menu.SetID(id);
			return menu;
		}

		if (id == MENU_BR_LEADERBOARD)
		{
			BattleRoyaleUtils.Trace("CreateScriptedMenu: MENU_BR_LEADERBOARD");
			UIScriptedMenu leaderboard_menu = new LeaderboardMenu;
			leaderboard_menu.SetID(id);
			return leaderboard_menu;
		}
#endif
		return super.CreateScriptedMenu(id);
	}
}
