modded class ExpansionClientSettings
{
	void SettingsOverride()
	{
        UseHelicopterMouseControl = true;
        TurnOffAutoHoverDuringFlight = true;
        EnableGPSBasedOnVehicleSpeed = true;

		Show3DClientMarkers = true;
		Show3DPlayerMarkers = true;
		Show3DPartyMarkers = true;
		Show3DGlobalMarkers = true;
		Show3DPartyMemberIcon = true;
		Show2DClientMarkers = true;
		Show2DPlayerMarkers = true;
		Show2DPartyMarkers = true;
		Show2DGlobalMarkers = true;

		MarkerSize = ExpansionClientUIMarkerSize.SMALL;
		PlayerArrowColor = ExpansionClientUIPlayerArrowColor.YELLOW;
	}

	override void OnSettingsUpdated(typename type, ExpansionSettingSerializationBase setting)
	{
		SettingsOverride();

		super.OnSettingsUpdated(type, setting);
	}

	override private void OnSave( ParamsWriteContext ctx )
	{
		SettingsOverride();

		super.OnSave(ctx);
    }

	override private bool OnRead( ParamsReadContext ctx, int version, out bool settingsRepaired = false )
	{
		if ( !super.OnRead(ctx, version, settingsRepaired) )
			return false;

		SettingsOverride();
		return true;
    }

	override void Init()
	{
		super.Init();
		
		SettingsOverride();
    }
};