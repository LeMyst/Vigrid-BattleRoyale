modded class MissionBase
{
    override UIScriptedMenu CreateScriptedMenu(int id)
	{
        UIScriptedMenu menu = NULL; 

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
