class LoadingScreenBackground
{
	string MapName;
	ref array<string> Path;

	//--- Built on first use and kept for the session, so the no-repeat run survives across loads.
	//--- [NonSerialized()] because Path is what comes out of LoadingScreens.json; this is derived.
	[NonSerialized()]
	ref BattleRoyaleShuffleBag m_Path;

	void LoadingScreenBackground(string map_name, array<string> texture_path)
	{
		MapName = map_name;
		Path = texture_path;
	}

	string GetRandomPath()
	{
        if (!m_Path)
            m_Path = new BattleRoyaleShuffleBag(Path);

        return m_Path.Draw();
	}
};

class LoadingScreenBackgrounds
{
	static ref array<ref LoadingScreenBackground> s_Backgrounds = new array<ref LoadingScreenBackground>;

	static array<ref LoadingScreenBackground> Get()
	{
		if (!s_Backgrounds.Count())
		{
    		string errorMessage;
			if (!JsonFileLoader<array<ref LoadingScreenBackground>>.LoadFile( DAYZBR_LOADING_SCREENS_PATH, s_Backgrounds, errorMessage))
				ErrorEx(errorMessage);
		}

		return s_Backgrounds;
	}
}
