modded class Chat
{
	override void Add(ChatMessageEventParams params)
	{
		BRLOG("Chat message received");
		super.Add(params);
	}
	//custom BR method for adding text to chat
	void Add(string text)
	{
		BRLOG("Add #1: " + text);
		ChatMessageEventParams msgParams = new ChatMessageEventParams(0,text,"","");
		Add(msgParams);
	}
	void Add(string name, string text)
	{
		BRLOG("Add #2: " + name + ": " + text);
		ChatMessageEventParams msgParams = new ChatMessageEventParams(0,name,text,"");
		Add(msgParams);
	}
}