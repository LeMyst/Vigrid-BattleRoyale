modded class UiHintPanel {

    //load BR hints instead of native engines
    private override void LoadContentList()
	{
		JsonFileLoader<array<ref HintPage>>.JsonLoadFile( "DayZBR-Mod/Scripts/5_Mission/GUI/Hints/hints.json", m_ContentList );
	}
}
