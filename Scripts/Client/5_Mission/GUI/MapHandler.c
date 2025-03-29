#ifndef SERVER
class BRMapHandler : ScriptedWidgetEventHandler
{
	protected Widget m_Root;

	void BRMapHandler(Widget w)
	{
		m_Root = w;
		m_Root.SetHandler(this);
	}

	override bool OnKeyDown(Widget w, int x, int y, int key)
	{
		if (!super.OnKeyDown(w, x, y, key))
			return false;

		GetGame().Chat("BRMapHandler::OnKeyDown", "colorFriendly");
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);

		GetGame().Chat("BRMapHandler::OnClick", "colorFriendly");

		SpawnSelectionMenu m = SpawnSelectionMenu.Cast(g_Game.GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
		m.SelectSpawnPoint(MapWidget.Cast(w).ScreenToMap(Vector(x, y, 0)));

		return true;
	}

	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		super.OnDoubleClick(w, x, y, button);

		GetGame().Chat("BRMapHandler::OnDoubleClick", "colorFriendly");

		SpawnSelectionMenu m = SpawnSelectionMenu.Cast(g_Game.GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
		m.SelectSpawnPoint(MapWidget.Cast(w).ScreenToMap(Vector(x, y, 0)));

		return true;
	}
};