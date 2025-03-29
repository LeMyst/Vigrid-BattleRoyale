#ifndef SERVER
class SpawnSelectionMenu extends UIScriptedMenu
{
	protected ref IngameHud m_Hud;
	protected ref BRMapHandler m_MapMenuHandler;
	protected ref MapWidget m_MapWidgetInstance;

	protected MapWidget m_MapWidget;
	protected TextWidget m_CountdownText;
	protected ButtonWidget m_ConfirmButton;

	protected vector m_SelectedSpawnPoint;

	void SpawnSelectionMenu()
	{
		GetGame().Chat("SpawnSelectionMenu::SpawnSelectionMenu", "colorFriendly");
		g_Game.SetKeyboardHandle(this);
	}

	void ~SpawnSelectionMenu()
	{
		g_Game.SetKeyboardHandle(NULL);
	}

	override void OnShow()
	{
		GetGame().Chat("SpawnSelectionMenu::OnShow", "colorFriendly");
		super.OnShow();

		PPEffects.SetBlurMenu(1);
		GetGame().GetInput().ChangeGameFocus(1);
		SetFocus(layoutRoot);
	}

	override void OnHide()
	{
		GetGame().Chat("SpawnSelectionMenu::OnHide", "colorFriendly");
		super.OnHide();

		PPEffects.SetBlurMenu(0);
		GetGame().GetInput().ResetGameFocus();
	}

	override Widget Init()
	{
		GetGame().Chat("SpawnSelectionMenu::Init", "colorFriendly");
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("Vigrid-BattleRoyale/GUI/layouts/spawn_selection.layout");
		m_Hud = IngameHud.Cast(GetGame().GetMission().GetHud());

		m_MapWidget = MapWidget.Cast(layoutRoot.FindAnyWidget("SpawnMap"));

		m_CountdownText = TextWidget.Cast(layoutRoot.FindAnyWidget("CountdownText"));

		m_ConfirmButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ConfirmButton"));

		if (m_MapWidget)
		{
			vector tempPosition = GetGame().ConfigGetVector(string.Format("CfgWorlds %1 centerPosition", GetGame().GetWorldName()));
			float scale = 0.33;
			vector mapPosition = Vector(tempPosition[0], tempPosition[1], tempPosition[2]);

			m_MapWidget.SetScale(scale);
			m_MapWidget.SetMapPos(mapPosition);

			m_MapMenuHandler = new BRMapHandler(m_MapWidget);

			if (m_Hud)
			{
				m_Hud.ShowHudUI(false);
				m_Hud.ShowQuickbarUI(false);
			}
		}

		layoutRoot.Update();

		return layoutRoot;
	}

	void SelectSpawnPoint(vector pos)
	{
		m_SelectedSpawnPoint = pos;

		m_MapWidget.ClearUserMarks();
		int color = ARGB(255, 255, 0, 255);
		m_MapWidget.AddUserMark(m_SelectedSpawnPoint, "", color, "\\DZ\\gear\\navigation\\data\\map_viewpoint_ca.paa");

		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SelectSpawnPoint", new Param1<vector>( m_SelectedSpawnPoint ), true );
	}

	vector GetSelectedSpawnPoint()
	{
		return m_SelectedSpawnPoint;
	}
}