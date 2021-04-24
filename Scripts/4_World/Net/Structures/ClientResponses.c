//--- response obejcts 
class MatchMakingResponse {
    bool wait;
    ref ServerData server;
}
class StartResponse {
    ref PlayerData player;
    ref RegionData region;
}


typedef GenericResponse<StartResponse> ClientStartResponse;

typedef GenericResponse<MatchMakingResponse> ClientMatchMakeResponse;

typedef GenericResponse<PlayerData> GetPlayerResponse;

typedef GenericResponse<MatchData> GetMatchResponse;

typedef GenericResponse<ServerData> GetServerResponse;
