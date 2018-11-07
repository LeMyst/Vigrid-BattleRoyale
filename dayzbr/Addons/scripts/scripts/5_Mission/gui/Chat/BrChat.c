modded class Chat
{
	//custom BR method for adding text to chat
	void Add(string text)
	{
		ChatMessageEventParams msgParams = new ChatMessageEventParams(0,text,"","");
		Add(msgParams);
	}
	void Add(string name, string text)
	{
		ChatMessageEventParams msgParams = new ChatMessageEventParams(0,name,text,"");
		Add(msgParams);
	}
}