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
                BattleRoyaleUtils.Trace("DayZGame::OnEvent StartupEventTypeID");
                break;
            }
            case MPSessionStartEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent MPSessionStartEventTypeID");
                break;
            }
            case MPSessionEndEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent MPSessionEndEventTypeID");
                break;
            }
            case MPSessionFailEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent MPSessionFailEventTypeID");
                break;
            }
            case MPSessionPlayerReadyEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent MPSessionPlayerReadyEventTypeID");
                break;
            }
            case MPConnectionLostEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent MPConnectionLostEventTypeID");
                break;
            }
            case WorldCleaupEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent WorldCleaupEventTypeID");
                break;
            }
            case DialogQueuedEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent DialogQueuedEventTypeID");
                break;
            }
            case ChatMessageEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent ChatMessageEventTypeID");
                break;
            }
            case ProgressEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent ProgressEventTypeID");
                break;
            }
            case LoginTimeEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent LoginTimeEventTypeID");
                break;
            }
            case RespawnEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent RespawnEventTypeID");
                break;
            }
            case PreloadEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent PreloadEventTypeID");
                break;
            }
            case LogoutEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent LogoutEventTypeID");
                break;
            }
            case SelectedUserChangedEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent SelectedUserChangedEventTypeID");
                break;
            }
            case LoginStatusEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent LoginStatusEventTypeID");
                break;
            }
            case ConnectingStartEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent ConnectingStartEventTypeID");
                break;
            }
            case ConnectingAbortEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent ConnectingAbortEventTypeID");
                break;
            }
            case DLCOwnerShipFailedEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent DLCOwnerShipFailedEventTypeID");
                break;
            }
            case ConnectivityStatsUpdatedEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent ConnectivityStatsUpdatedEventTypeID");
                break;
            }
            case ServerFpsStatsUpdatedEventTypeID:
            {
                BattleRoyaleUtils.Trace("DayZGame::OnEvent ServerFpsStatsUpdatedEventTypeID");
                break;
            }
        }

        super.OnEvent(eventTypeId, params);
    }
}
