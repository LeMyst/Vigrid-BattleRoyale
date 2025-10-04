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
#endif
		return super.CreateScriptedMenu(id);
	}
}
