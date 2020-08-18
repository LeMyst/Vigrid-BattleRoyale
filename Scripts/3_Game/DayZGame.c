modded class DayZGame {
	private string m_DummyMissionPath = "$saves:DummyMissions/Dummy.ChernarusPlus";

	void DayZGame()
	{
		if (FileExist("$saves:DummyMissions/Dummy.ChernarusPlus/storage_-1"))
			DeleteFile("$saves:DummyMissions/Dummy.ChernarusPlus/storage_-1");

		//Create the amazing dummy mission
		if ( !FileExist(m_DummyMissionPath) )
		{
			MakeDirectory("$saves:DummyMissions");
			MakeDirectory("$saves:DummyMissions/Dummy.ChernarusPlus");
			MakeDirectory("$saves:DummyMissions/Dummy.ChernarusPlus/db");

			FileHandle file = OpenFile(m_DummyMissionPath + "/init.c", FileMode.WRITE);
			FPrintln(file, "class ClientDummyMission: MissionMainMenu");
			FPrintln(file, "{");
			FPrintln(file, "override void OnInit()");
			FPrintln(file, "{");
			FPrintln(file, "super.OnInit();");
			FPrintln(file, "Print(\"[ClientDummyMission] --Hive--\");");
			FPrintln(file, "Hive ce = CreateHive();");
			FPrintln(file, "if ( ce )");
			FPrintln(file, "ce.InitSandbox();");
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
	override void MainMenuLaunch()
	{
		SetGameState( DayZGameState.MAIN_MENU );
		SetLoadState( DayZLoadState.MAIN_MENU_START );
	
		Print("[ DayZGame ] Start Dummy Mission for client...");
		switch(GetMainMenuWorld())
		{
			case "Enoch":
			PlayMission(m_DummyMissionPath); //Need to setup a dummy mission for enoch aka livonia
			break;

			case "ChernarusPlus":
			PlayMission(m_DummyMissionPath);
			break;
		}
		DeleteTitleScreen();
	}
	//disables CLI -connect
	override void ConnectFromCLI()
	{
	  g_Game.MainMenuLaunch();
	}

	string GetBattleRoyaleClientVersion() 
	{
		return "0.1.b";
	}
}


