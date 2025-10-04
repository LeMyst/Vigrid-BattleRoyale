#ifndef SERVER
#ifdef DIAG_DEVELOPER
modded class PluginDiagMenu
{
	protected int m_BRDiagRootMenuID;
	protected int m_BRDiagDummyID;
	protected int m_BRDiagSpectateID;

	override protected void RegisterModdedDiagsIDs()
	{
		super.RegisterModdedDiagsIDs();

		m_BRDiagRootMenuID = GetModdedDiagID();
		m_BRDiagDummyID = GetModdedDiagID();
		m_BRDiagSpectateID = GetModdedDiagID();
	}

	override protected void RegisterModdedDiags()
	{
		super.RegisterModdedDiags();

		DiagMenu.RegisterMenu(m_BRDiagRootMenuID, "BattleRoyale - Myst", GetModdedRootMenu());
		DiagMenu.RegisterBool(m_BRDiagDummyID, "", "Dummy Option", m_BRDiagRootMenuID);
		DiagMenu.RegisterBool(m_BRDiagSpectateID, "", "Start spectating", m_BRDiagRootMenuID);
	}
}