#ifndef SERVER
modded class DynamicMusicPlayerRegistry
{
	override protected void RegisterTracksMenu()
	{
		m_TracksMenu = new array<ref DynamicMusicTrackData>();

		RegisterTrackMenu("BattleRoyale_Music_SoundSet", true);
		RegisterTrackMenu("Music_Menu_SoundSet");
		RegisterTrackMenu("Music_Menu_2_SoundSet");
		RegisterTrackMenu("Music_Menu_3_SoundSet");
		RegisterTrackMenu("Music_Menu_4_SoundSet");
	}
}