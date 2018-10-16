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
				case CCNone:
					m_current_channel = ChatChannel.CCGlobal;
				case CCGlobal:
					m_current_channel = ChatChannel.CCItemTransmitter;
				case CCItemTransmitter:
					m_current_channel = ChatChannel.CCDirect;
				case CCDirect:
					m_current_channel = ChatChannel.CCStatus;
				case CCStatus:
					m_current_channel = ChatChannel.CCSystem;
				case CCSystem:
					m_current_channel = ChatChannel.CCNone;
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
			case CCNone:
				return "None";
			case CCGlobal:
				return "Global";
			case CCItemTransmitter:
				return "Radio"; 
			case CCDirect:
				return "Direct";      
			case CCStatus:
				return "Status";   
			case CCSystem:
				return "System";   
		}	
		
		return "";
	}
}