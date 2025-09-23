#ifndef SERVER
modded class MissionBase
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
#ifndef NO_GUI
		if (id == MENU_SPAWN_SELECTION)
		{
			GetGame().Chat("CreateScriptedMenu: MENU_SPAWN_SELECTION", "colorFriendly");
			UIScriptedMenu menu = new SpawnSelectionMenu;
			menu.SetID(id);
			return menu;
		}
#endif
		return super.CreateScriptedMenu(id);
	}
}
