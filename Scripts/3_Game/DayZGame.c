modded class DayZGame {
	//dummy mission foldername should be based on the mods we have loaded (dynamic)
	private string m_DummyMissionPath = "$saves:DayZBR/Dummy." + BATTLEROYALE_DUMMY_MISSION_WORLD;

	void DayZGame()
	{
		Print("DayZGame constructor called");
		Print(m_DummyMissionPath);
		//Create the amazing dummy mission
		if ( !FileExist(m_DummyMissionPath) )
		{
            if (FileExist(m_DummyMissionPath+"/storage_-1"))
                DeleteFile(m_DummyMissionPath+"/storage_-1");

			MakeDirectory("$saves:DayZBR");
			MakeDirectory(m_DummyMissionPath);
			MakeDirectory(m_DummyMissionPath+"/db");

			FileHandle file = OpenFile(m_DummyMissionPath + "/init.c", FileMode.WRITE);
			FPrintln(file, "class ClientDummyMission: MissionMainMenu");
			FPrintln(file, "{");
			FPrintln(file, "override void OnInit()");
			FPrintln(file, "{");
			FPrintln(file, "super.OnInit();");
			FPrintln(file, "Print(\"[ClientDummyMission] --Hive--\");");
			FPrintln(file, "Hive ce = CreateHive();");
			FPrintln(file, "if ( ce ) {");
			FPrintln(file, "Print(\"Hive created\");");
			FPrintln(file, "ce.InitSandbox();");
			FPrintln(file, "Print(\"Sandbox initialized\");");
			FPrintln(file, "} else {");
			FPrintln(file, "Print(\"Hive create Failed\");");
			FPrintln(file, "}");
			FPrintln(file, "}");
			FPrintln(file, "};");
			FPrintln(file, "Mission CreateCustomMission(string path)");
			FPrintln(file, "{");
			FPrintln(file, "return new ClientDummyMission();");
			FPrintln(file, "}");
			FPrintln(file, "void main(){}");
			CloseFile(file);

			file = OpenFile(m_DummyMissionPath + "/db/types.xml", FileMode.WRITE);
			FPrintln(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
			FPrintln(file, "<types>");
			FPrintln(file, "");
			FPrintln(file, "</types>");
			CloseFile(file);

			file = OpenFile(m_DummyMissionPath + "/db/globals.xml", FileMode.WRITE);
			FPrintln(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
			FPrintln(file, "<variables>");
			FPrintln(file, "<var name=\"CleanupAvoidance\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"CleanupLifetimeLimit\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"QueueTimeSameServer\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"RespawnAttempt\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"RespawnLimit\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"RespawnTypes\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"SpawnInitial\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"ZombieMaxCount\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"CleanupLifetimeDefault\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"ZoneSpawnDist\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"RestartSpawn\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"CleanupLifetimeRuined\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "<var name=\"AnimalMaxCount\" type=\"0\" value=\"1\"/>");
			FPrintln(file, "</variables>");
			CloseFile(file);

			file = OpenFile(m_DummyMissionPath + "/db/events.xml", FileMode.WRITE);
			FPrintln(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
			FPrintln(file, "<events>");
			FPrintln(file, "</events>");
			CloseFile(file);

			file = OpenFile(m_DummyMissionPath + "/db/economy.xml", FileMode.WRITE);
			FPrintln(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
			FPrintln(file, "<economy>");
			FPrintln(file, "<dynamic init=\"1\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "<animals init=\"1\" load=\"0\" respawn=\"1\" save=\"0\"/>");
			FPrintln(file, "<zombies init=\"1\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "<vehicles init=\"1\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "<randoms init=\"0\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "<custom init=\"0\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "<building init=\"0\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "<player init=\"1\" load=\"0\" respawn=\"0\" save=\"0\"/>");
			FPrintln(file, "</economy>");
			CloseFile(file);
		}
	}

	//This will start the dummy mission ONLY when the game is launched into main menu so using "-connect=x.x.x.x" will not hit this function
	//override void MainMenuLaunch()
	//{
		//SetGameState( DayZGameState.MAIN_MENU );
		//SetLoadState( DayZLoadState.MAIN_MENU_START );

		//Print("[ DayZGame ] Start Dummy Mission for client...");
		//PlayMission(m_DummyMissionPath);
		//DeleteTitleScreen();
	//}

	//disables CLI -connect
	//override void ConnectFromCLI()
	//{
		//if we're using -connect, check if it is an official server. if it is not, allow direct connect

	 // g_Game.MainMenuLaunch();
	//}

	//--- above this is a patch for GetRestApi

	string GetBattleRoyaleClientVersion()
	{
		return BATTLEROYALE_VERSION;
	}

    void ConnectToBR()
    {
        Print("ConnectToBR()");
        if (BATTLEROYALE_SOLO_GAME)
        {
            m_ConnectAddress	= "127.0.0.1";
        } else {
            m_ConnectAddress	= "br.nimih.fr";
        }
        m_ConnectPort		= 2302;
        m_ConnectPassword	= "";
        m_ConnectFromJoin	= false;
        Connect();
    }
}
