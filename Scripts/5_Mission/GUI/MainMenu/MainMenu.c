modded class MainMenu
{
	override Widget Init()
	{
		super.Init(); //--- TODO: does this call the DayZ expansion init function?
		
		
		
		
		Print("DAYZ BATTLE ROYALE INIT");
		//GetGame().OpenURL("https://dayzbr.dev/");
	
		BiosUserManager p_UserManager = GetGame().GetUserManager();
		if(p_UserManager)
		{	
			BiosUser p_User = p_UserManager.GetTitleInitiator();
			if(p_User)
			{
				Print("BIOS2");
				Print(p_User.GetName());
				Print(p_User.GetUid());
				
				RestApi p_Rest = GetRestApi();
				RestContext p_ClientContext = p_Rest.GetRestContext("https://dayzbr.dev/client/");
				string result = p_ClientContext.GET_now("start/" + p_User.GetUid() + "/" + p_User.GetName());
				
				
				
				//--- try to serialize it
				JsonSerializer m_Serializer = new JsonSerializer;
				PlayerData p_PlayerWebData;
				string error;
				if( m_Serializer.ReadFromString( p_PlayerWebData, result, error ) )
				{
					Print(p_PlayerWebData.name);
					Print(p_PlayerWebData.steam_id);
					Print(p_PlayerWebData.match_ids);
					Print(p_PlayerWebData.rating);
					Print(p_PlayerWebData.ips);
					Print(p_PlayerWebData._id);
				}
				else 
				{
					Print(result);
					Error( error );
				}
			}
			else
			{
				Print("p_User = NULL [2]");
			}
		}
		else
		{
			Print("p_UserManager = NULL");
		}
		
		Print("You are running a developer build of DayZBR");
		Print("DAYZ BATTLE ROYALE END");
		
		return layoutRoot;
	}
}

class PlayerData 
{
	//{"name":"Lystic","steam_id":"76561198277370562","match_ids":[],"rating":1000,"ips":["173.69.172.153"],"_id":"5f024bbee9e65b9c7f824cea"}
	
	string name;
	string steam_id;
	array<string> match_ids;
	int rating;
	array<string> ips;
	string _id;
}