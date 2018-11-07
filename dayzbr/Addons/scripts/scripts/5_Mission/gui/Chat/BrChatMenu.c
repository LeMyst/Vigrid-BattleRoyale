class BrChatMenu extends ChatInputMenu
{
	protected EditBoxWidget new_m_edit_box;
	protected TextWidget new_m_channel_text;
	protected ref Timer new_m_close_timer;
	static int m_current_channel = 0;
	
	protected MissionGameplay new_m_mission;
	
	void BrChatMenu(MissionBase m_mission)
	{
		new_m_mission = MissionGameplay.Cast(m_mission);
		new_m_close_timer = new Timer();
	}

	override Widget Init()
	{
		layoutRoot = super.Init();
		new_m_edit_box = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("InputEditBoxWidget") );
		new_m_channel_text = TextWidget.Cast( layoutRoot.FindAnyWidget("ChannelText") );
		
		
		
		UpdateChannel();
		return layoutRoot;
	}
	
	override bool UseKeyboard() 
	{ 
		return true; 
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if(!finished)
		{
			super.OnChange(w, x, y, finished);
		}
		
		if (!finished) return false;
		
		string text = new_m_edit_box.GetText();

		BRLOG("Chat message: " + text);
		if (text != "")
		{
			if(BrChatMenu.m_current_channel == 1)
			{
				//Global Chat RPC
				BRLOG("GLOBAL CHAT");
				ref Param1<string> value_string = new Param1<string>(text);
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "GlobalChat", value_string, false, null, GetGame().GetPlayer() );
			}
			else
			{
				//Local chat
				BRLOG("LOCAL CHAT");
				GetGame().ChatPlayer(0, text);
			}
		}

		new_m_close_timer.Run(0.1, this, "Close");
		
		GetGame().GetMission().HideChat();
		
		return true;
	}

	
	override void Refresh()
	{	
	}
	
	override void Update(float timeslice)
	{
		
		
		
		if(GetGame().GetInput().GetAction(UAZeroingUp,false) > 0)
		{
			switch(BrChatMenu.m_current_channel)
			{
				case 0:
					BrChatMenu.m_current_channel = 1;
					break;
				case 1:
					BrChatMenu.m_current_channel = 0;
					break;
			}	
			UpdateChannel();
		}
		if(GetGame().GetInput().GetAction(UAZeroingDown,false) > 0)
		{
			switch(BrChatMenu.m_current_channel)
			{
				case 0:
					BrChatMenu.m_current_channel = 1;
					break;
				case 1:
					BrChatMenu.m_current_channel = 0;
					break;
			}	
			UpdateChannel();
		}
		
		GetGame().GetInput().DisableKey(KeyCode.KC_RETURN);
	}
	
	override void UpdateChannel()
	{
		if(new_m_mission)
		{
			new_m_mission.m_ChatChannelText.SetText(BrChatMenu.GetChannelNameInt(BrChatMenu.m_current_channel));
			new_m_mission.m_ChatChannelFadeTimer.FadeIn(new_m_mission.m_ChatChannelArea, 0.5, true);
			new_m_mission.m_ChatChannelHideTimer.Run(2, new_m_mission.m_ChatChannelFadeTimer, "FadeOut", new Param3<Widget, float, bool>(new_m_mission.m_ChatChannelArea, 0.5, true));
		}
	}
	
	static string GetChannelNameInt(int channel)
	{
		switch(channel)
		{
			case 0:
				return "Direct"; //This is actually "None" the default value for dayz
			case 1:
				return "Global";
			case 3:
				return "Radio"; 
			case 6:
				return "Direct";      
			case 18:
				return "Status";   
			case 19:
				return "System";   
		}	
		
		return "";
	}
	override static string GetChannelName(ChatChannel channel)
	{
		switch(channel)
		{
			case 0:
				return "Direct"; //This is actually "None" the default value for dayz
			case 1:
				return "Global";
			case 3:
				return "Radio"; 
			case 6:
				return "Direct";      
			case 18:
				return "Status";   
			case 19:
				return "System";   
		}	
		
		return "";
	}
}