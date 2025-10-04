#ifndef SERVER
class BRMapHandler : ScriptedWidgetEventHandler
{
	protected Widget m_Root;

	protected int last_click_on_map;
	protected vector v_StartClickPos;

	void BRMapHandler(Widget w)
	{
		m_Root = w;
		m_Root.SetHandler(this);
		last_click_on_map = 0;
	}

	// When the player clicks on the map
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		v_StartClickPos = Vector(x, y, 0);

		return super.OnMouseButtonDown(w, x, y, button);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if(GetGame().GetTime() - last_click_on_map < 250 || v_StartClickPos != Vector(x, y, 0))
		{
			return super.OnMouseButtonUp(w, x, y, button);
		}
		last_click_on_map = GetGame().GetTime();

		BattleRoyaleUtils.Trace("BRMapHandler::OnMouseButtonDown");

		SpawnSelectionMenu m = SpawnSelectionMenu.Cast(g_Game.GetUIManager().FindMenu(MENU_SPAWN_SELECTION));
		m.SelectSpawnPoint(Vector(x, y, 0));

		return super.OnMouseButtonUp(w, x, y, button);
	}
};