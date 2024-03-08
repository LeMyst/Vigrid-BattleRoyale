#ifndef SERVER
modded class MissionMainMenu
{
    override void PlayMusic()
    {
        if ( !m_MenuMusic )
        {
            SoundParams soundParams            = new SoundParams( "BattleRoyale_Music_SoundSet" );
            SoundObjectBuilder soundBuilder    = new SoundObjectBuilder( soundParams );
            SoundObject soundObject            = soundBuilder.BuildSoundObject();
            soundObject.SetKind( WaveKind.WAVEMUSIC );
            m_MenuMusic = GetGame().GetSoundScene().Play2D(soundObject, soundBuilder);
            m_MenuMusic.Loop( true );
            m_MenuMusic.Play();
        }

        GetGame().RequestExit(0);
    }
};
#endif