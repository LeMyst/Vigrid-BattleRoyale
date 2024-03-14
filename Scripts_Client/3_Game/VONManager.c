#ifndef SERVER
modded class VONManagerImplementation
{
	override void HandleInput(Input inp)
	{
		super.HandleInput( inp );

		if ( GetGame().GetVoiceLevel() > VoiceLevelWhisper )
		{
			GetGame().SetVoiceLevel( VoiceLevelWhisper ); // Force level to whisper
			UpdateVoiceIcon();
		}
	}
}
