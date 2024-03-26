#ifdef SERVER
modded class ExpansionAirdropSettings
{
	void SettingsOverride()
	{
        Height = 250.0;
        Radius = 1.0;
        Speed = 400;
	}

	override void OnSave()
	{
		SettingsOverride();

		super.OnSave();
    }

	override bool OnRead()
	{
		if ( !super.OnRead() )
			return false;

		SettingsOverride();

		return true;
    }
}
#endif
