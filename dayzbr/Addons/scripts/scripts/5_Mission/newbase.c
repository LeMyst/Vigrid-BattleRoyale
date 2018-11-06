

modded class MissionBase
{
	bool autowalk = false;
	
	
	
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = NULL;
		//check br menus
		switch (id)
		{
			case MENU_CHAT_INPUT:
				menu = new BrChatMenu;
				menu.SetID(id);
				return menu;
				BRLOG("CHAT UI BR");
				break;
			case MENU_SKINS:
				menu = new SkinMenu(this);
				menu.SetID(id);
				BRLOG("SKIN UI BR");
				return menu;
				break;
			case MENU_GESTURES:
				menu = new NewGesturesMenu(this);
				menu.SetID(id);
				BRLOG("GESTURE UI BR");
				return menu;
				break;	
		}
		
		return super.CreateScriptedMenu(id);
	}
}