modded class ConnectErrorServerModule
{
	override void FillErrorDataMap()
	{
		InsertDialogueErrorProperties(EConnectErrorServer.SERVER_LOCKED,		"#STR_MP_SESSION_LOCKED"+"\n"+"#STR_BR_SERVER_LOCKED");

		super.FillErrorDataMap();
	}
}