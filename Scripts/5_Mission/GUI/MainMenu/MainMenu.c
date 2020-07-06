modded class MainMenu
{
	override Widget Init()
	{
		super.Init(); //--- TODO: does this call the DayZ expansion init function?
		
		//this will fail if we are not initializing a DayZExpansion main menu (as m_Logo only exists there)
		m_Logo.LoadImageFile( 0, "set:dayzbr_gui image:DayZBRLogo_White" );
		
		
		if(OnStart())
		{
			
			Print("You are running a developer build of DayZBR");
		}
		else
		{
			Error("Something Went Wrong! BR Failed To Start?!");
		}
		
		return layoutRoot;
	}
	
	bool OnStart()
	{
		Print("DAYZ BATTLE ROYALE INIT");
		
		BiosUserManager p_UserManager = GetGame().GetUserManager();
		if(!p_UserManager)
		{
			Print("DBR ERROR: p_UserManager = NULL");
			return false;
		}
		BiosUser p_User = p_UserManager.GetTitleInitiator();
		if(!p_User)
		{
			Print("DBR ERROR: p_User = NULL");
			return false;
		}
		RestApi p_Rest = GetRestApi();
		RestContext p_ClientContext = p_Rest.GetRestContext("https://dayzbr.dev/client/");
		string web_result = p_ClientContext.GET_now("start/" + p_User.GetUid() + "/" + p_User.GetName());
		
		JsonSerializer m_Serializer = new JsonSerializer;
		PlayerData p_PlayerWebData;
		string error;
		if(!m_Serializer.ReadFromString( p_PlayerWebData, result, error ))
		{
			Print("DBR ERROR: JSON Failed To Parse!");
			Error(error);
			return false;
		}
		Print("DAYZ BATTLE ROYALE END");
		
		//TODO: manage p_PlayerWebData and possibly our webclient through GetGame() (by having a custom game object get instantiated)
		return true;
	}
}

class PlayerData 
{
	//here is an example of the json data we expect to parse into this class
	//{"name":"Lystic","steam_id":"76561198277370562","match_ids":[],"rating":1000,"ips":["173.69.172.153"],"_id":"5f024bbee9e65b9c7f824cea"}
	string name;
	string steam_id;
	array<string> match_ids;
	int rating;
	array<string> ips;
	string _id;
}