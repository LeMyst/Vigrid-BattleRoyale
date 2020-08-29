modded class MissionBase
{
    override UIScriptedMenu CreateScriptedMenu(int id)
	{
        UIScriptedMenu menu = NULL; 
        switch ( id )
		{
            case DAYZBR_SKIN_SELECTION_MENU:
                menu = new SkinSelectionMenu;
                break;
        }

        if ( menu )
		{
			menu.SetID( id );
		} else
		{
			menu = super.CreateScriptedMenu( id );
		}

        return menu;
    }
}