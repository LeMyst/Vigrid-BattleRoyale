#ifdef SERVER
/**
 *  Put the resolved name in the ADM log too.
 *
 *  Overwriting PlayerBase.m_CachedPlayerName is not enough on its own: vanilla's GetPlayerPrefix
 *  only falls back to GetCachedName() when the identity is already gone (pluginadminlog.c:82-92),
 *  so a live player is still logged as "Survivor" without this.
 *
 *  Patching super's answer rather than reimplementing it: the vanilla prefix carries the name as the
 *  one quoted span in the line, so a single Replace is enough, and the position/id formatting stays
 *  vanilla's problem across game updates.
 */
modded class PluginAdminLog
{
	override string GetPlayerPrefix(PlayerBase player, PlayerIdentity identity)
	{
		string prefix = super.GetPlayerPrefix(player, identity);

		if (!identity)
			return prefix;

		string original = identity.GetName();
		if (original == "")
			return prefix;

		string resolved = BattleRoyaleNameService.ResolveIdentity(identity);
		if (resolved == "")
			return prefix;
		if (resolved == original)
			return prefix;

		prefix.Replace("\"" + original + "\"", "\"" + resolved + "\"");

		return prefix;
	}
};
#endif
