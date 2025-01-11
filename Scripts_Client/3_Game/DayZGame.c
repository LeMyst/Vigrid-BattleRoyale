modded class DayZGame
{
    string GetBattleRoyaleClientVersion()
    {
        return BATTLEROYALE_VERSION;
    }

    override void OnEvent(EventType eventTypeId, Param params)
    {
        switch (eventTypeId)
        {
            case StartupEventTypeID:
            {
                Print("DayZGame::OnEvent StartupEventTypeID");
                break;
            }
            case MPSessionStartEventTypeID:
            {
                Print("DayZGame::OnEvent MPSessionStartEventTypeID");
                break;
            }
            case MPSessionEndEventTypeID:
            {
                Print("DayZGame::OnEvent MPSessionEndEventTypeID");
                break;
            }
            case MPSessionFailEventTypeID:
            {
                Print("DayZGame::OnEvent MPSessionFailEventTypeID");
                break;
            }
            case MPSessionPlayerReadyEventTypeID:
            {
                Print("DayZGame::OnEvent MPSessionPlayerReadyEventTypeID");
                break;
            }
            case MPConnectionLostEventTypeID:
            {
                Print("DayZGame::OnEvent MPConnectionLostEventTypeID");
                break;
            }
            case WorldCleaupEventTypeID:
            {
                Print("DayZGame::OnEvent WorldCleaupEventTypeID");
                break;
            }
            case DialogQueuedEventTypeID:
            {
                Print("DayZGame::OnEvent DialogQueuedEventTypeID");
                break;
            }
            case ChatMessageEventTypeID:
            {
                Print("DayZGame::OnEvent ChatMessageEventTypeID");
                break;
            }
            case ProgressEventTypeID:
            {
                Print("DayZGame::OnEvent ProgressEventTypeID");
                break;
            }
            case LoginTimeEventTypeID:
            {
                Print("DayZGame::OnEvent LoginTimeEventTypeID");
                break;
            }
            case RespawnEventTypeID:
            {
                Print("DayZGame::OnEvent RespawnEventTypeID");
                break;
            }
            case PreloadEventTypeID:
            {
                Print("DayZGame::OnEvent PreloadEventTypeID");
                break;
            }
            case LogoutEventTypeID:
            {
                Print("DayZGame::OnEvent LogoutEventTypeID");
                break;
            }
            case SelectedUserChangedEventTypeID:
            {
                Print("DayZGame::OnEvent SelectedUserChangedEventTypeID");
                break;
            }
            case LoginStatusEventTypeID:
            {
                Print("DayZGame::OnEvent LoginStatusEventTypeID");
                break;
            }
            case ConnectingStartEventTypeID:
            {
                Print("DayZGame::OnEvent ConnectingStartEventTypeID");
                break;
            }
            case ConnectingAbortEventTypeID:
            {
                Print("DayZGame::OnEvent ConnectingAbortEventTypeID");
                break;
            }
            case DLCOwnerShipFailedEventTypeID:
            {
                Print("DayZGame::OnEvent DLCOwnerShipFailedEventTypeID");
                break;
            }
            case ConnectivityStatsUpdatedEventTypeID:
            {
                Print("DayZGame::OnEvent ConnectivityStatsUpdatedEventTypeID");
                break;
            }
            case ServerFpsStatsUpdatedEventTypeID:
            {
                Print("DayZGame::OnEvent ServerFpsStatsUpdatedEventTypeID");
                break;
            }
        }

        super.OnEvent(eventTypeId, params);
    }
}
