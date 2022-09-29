//--- response obejcts 
class MatchMakingResponse {
    bool wait;
    ref ServerData server;
}
class StartResponse {
    ref BRPlayerData player;
    ref RegionData region;
}


typedef GenericResponse<ref StartResponse> ClientStartResponse;

typedef GenericResponse<ref MatchMakingResponse> ClientMatchMakeResponse;

typedef GenericResponse<ref BRPlayerData> GetPlayerResponse;

typedef GenericResponse<ref MatchData> GetMatchResponse;

typedef GenericResponse<ref ServerData> GetServerResponse;
