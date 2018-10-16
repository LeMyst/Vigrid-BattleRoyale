class BrChatMenu extends ChatInputMenu
{
	private EditBoxWidget new_m_edit_box;
	private TextWidget new_m_channel_text;
	private ref Timer new_m_close_timer;
	static ChatChannel m_current_channel = 0;
	
	
	void BrChatMenu()
	{
		new_m_close_timer = new Timer();
	}

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("gui/layouts/day_z_chat_input.layout");
		new_m_edit_box = EditBoxWidget.Cast( layoutRoot.FindAnyWidget("InputEditBoxWidget") );
		new_m_channel_text = TextWidget.Cast( layoutRoot.FindAnyWidget("ChannelText") );
		
		UpdateChannel();
		return layoutRoot;
	}
	
	override bool OnKeyDown(Widget w, int x, int y, int key)
	{
		if(key == KeyCode.KC_TAB)
		{
			switch(m_current_channel)
			{
				case 0:
					m_current_channel = 1;
					break;
				case 1:
					m_current_channel = 3;
					break;
				case 3:
					m_current_channel = 6;
					break;
				case 6:
					m_current_channel = 18;
					break;
				case 18:
					m_current_channel = 19;
					break;
				case 19:
					m_current_channel = 0;
					break;
			}	
			UpdateChannel();
			return true;
		}
		
		return super.OnKeyDown(w,x,y,key);
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
			GetGame().ChatPlayer(m_current_channel, text);
		}

		new_m_close_timer.Run(0.1, this, "Close");
		return true;
	}

	
	override void Refresh()
	{	
	}
	
	override void Update(float timeslice)
	{
		GetGame().GetInput().DisableKey(KeyCode.KC_RETURN);
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
				return "None";
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