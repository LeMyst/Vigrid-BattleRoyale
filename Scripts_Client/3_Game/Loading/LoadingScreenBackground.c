class LoadingScreenBackground
{
	string MapName;
	ref array<string> Path;

	[NonSerialized()]
	ref ExpansionArray<string> m_Path;

	void LoadingScreenBackground(string map_name, array<string> texture_path)
	{
		MapName = map_name;
		Path = texture_path;
	}

	string GetRandomPath()
	{
        if (!m_Path)
            m_Path = new ExpansionArray<string>(Path);

        return m_Path.GetQuasiRandomElementAvoidRepetition();
	}
};

class LoadingScreenBackgrounds
{
	static ref array<ref LoadingScreenBackground> s_Backgrounds = new array<ref LoadingScreenBackground>;

	static array<ref LoadingScreenBackground> Get()
	{
		if (!s_Backgrounds.Count())
			JsonFileLoader<array<ref LoadingScreenBackground>>.JsonLoadFile( "Vigrid-BattleRoyale/Data/LoadingScreens.json", s_Backgrounds );

		return s_Backgrounds;
	}
}
