class BrChatMenu extends UIScriptedMenu
{
	EditBoxWidget new_m_edit_box;
	TextWidget new_m_channel_text;
	ref Timer new_m_close_timer;
	static int m_current_channel = 0;
	
	
	void BrChatMenu()
	{
		new_m_close_timer = new Timer();
	}

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("gui/layouts/day_z_chat_input.layout");
		new_m_edit_box = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("InputEditBoxWidget") );
		new_m_channel_text = TextWidget.Cast( layoutRoot.FindAnyWidget("ChannelText") );
		
		new_m_channel_text.Show(true);
		
		
		UpdateChannel();
		return layoutRoot;
	}
	
	override bool UseKeyboard() 
	{ 
		return true; 
	}
	
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		super.OnChange(w, x, y, finished);
		
		if (!finished) return false;
		
		string text = new_m_edit_box.GetText();

		if (text != "")
		{
			bool test_global_chat = false; //TODO: flip this to true if you want to test global chat through ChatPlayer
			if(m_current_channel == 1 && !test_global_chat)
			{
				//Global Chat RPC
				ref Param1<string> value_string = new Param1<string>(text);
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "GlobalChat", value_string, false, null, GetGame().GetPlayer() );
			}
			else
			{
				//Local chat
				GetGame().ChatPlayer(m_current_channel, text);
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
		GetGame().GetInput().DisableKey(KeyCode.KC_RETURN);
		//actions for changing chat channel
		if(GetGame().GetInput().GetAction(UANextAction,false) > 0)
		{
			switch(m_current_channel)
			{
				case 0:
					m_current_channel = 1;
					break;
				case 1:
					m_current_channel = 0;
					break;
			}	
			UpdateChannel();
		}
		if(GetGame().GetInput().GetAction(UAPrevAction,false) > 0)
		{
			switch(m_current_channel)
			{
				case 0:
					m_current_channel = 1;
					break;
				case 1:
					m_current_channel = 0;
					break;
			}	
			UpdateChannel();
		}
	}
	
	override void UpdateChannel()
	{
		new_m_channel_text.SetText(GetChannelName(m_current_channel));	
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