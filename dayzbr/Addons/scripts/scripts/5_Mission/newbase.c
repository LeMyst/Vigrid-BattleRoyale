

modded class MissionBase
{
	bool autowalk = false;
	
	
	
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = super.CreateScriptedMenu(id);
		if(menu)
		{
			return menu;
		}
		//no menu was found (br menu?)
		
		switch (id)
		{
			case MENU_CHAT_INPUT:
				menu = new BrChatMenu;
				break;
			case MENU_SKINS:
				menu = new SkinMenu(this);
				break;
			case MENU_GESTURES:
				menu = new NewGesturesMenu(this);
				break;	
		}

		if (menu)
		{
			menu.SetID(id);
		}

		return menu;
	}
}