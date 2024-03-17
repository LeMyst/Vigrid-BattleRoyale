class ESPBox extends UIScriptedMenu
{
	private Widget           m_MarkerRootWidget;
	private GridSpacerWidget m_SpacerGrid;

	private TextWidget       m_NameInput;

	private bool             isMarkerVisible;
	private Object           m_TargetVehicle;

	private string m_Name;

	void ESPBox( Object TargetVehicle, string playerName )
	{
		Print("new object ESPBox()");
		GetGame().Chat( "new object ESPBox()", "colorImportant" );
		m_MarkerRootWidget = GetGame().GetWorkspace().CreateWidgets( "Vigrid-BattleRoyale/GUI/layouts/name.layout" );

		m_SpacerGrid       = GridSpacerWidget.Cast( m_MarkerRootWidget.FindAnyWidget( "SpacerGrid" ) );
		m_NameInput        = TextWidget.Cast( m_SpacerGrid.FindAnyWidget( "NameInput" ) );

		m_TargetVehicle    = TargetVehicle;
		m_Name             = playerName;

		m_NameInput.SetText(m_Name);

		m_SpacerGrid.Update();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(this.DoUpdate, 1, true);
	}

	void ~ESPBox()
	{
		GetGame().Chat( "~ESPBox()", "colorImportant" );
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(this.DoUpdate);
		m_MarkerRootWidget.Show( false );
	}

	void DoUpdate()
	{
		GetGame().Chat( "DoUpdate()", "colorImportant" );
		if (m_MarkerRootWidget != NULL)
		{
			vector TargetVehiclePosition = m_TargetVehicle.GetPosition();
			vector ScreenPosRelative = GetGame().GetScreenPosRelative(Vector(TargetVehiclePosition[0] + 0.2, TargetVehiclePosition[1] + 1.6,TargetVehiclePosition[2]));
			if( ScreenPosRelative[0] >= 1 || ScreenPosRelative[0] == 0 || ScreenPosRelative[1] >= 1 || ScreenPosRelative[1] == 0 )
			{
				GetGame().Chat( "dont show m_MarkerRootWidget a", "colorImportant" );
				m_MarkerRootWidget.Show( false );
				return;
			}
			else if( ScreenPosRelative[2] < 0 )
			{
				GetGame().Chat( "dont show m_MarkerRootWidget b", "colorImportant" );
				m_MarkerRootWidget.Show( false );
				return;
			}
			else
			{
				GetGame().Chat( "show m_MarkerRootWidget", "colorImportant" );
				m_MarkerRootWidget.Show( true );
			}

			float pos_x, pos_y;

			vector ScreenPos = GetGame().GetScreenPos(Vector(TargetVehiclePosition[0] + 0.2, TargetVehiclePosition[1] + 1.6,TargetVehiclePosition[2]));
			pos_x = ScreenPos[0];
			pos_y = ScreenPos[1];

			pos_x = Math.Ceil(pos_x);
			pos_y = Math.Ceil(pos_y);

			m_MarkerRootWidget.SetPos(pos_x,pos_y);

			m_SpacerGrid.Update();
		} else {
			GetGame().Chat( "m_MarkerRootWidget is NULL", "colorImportant" );
		}
	}
}
