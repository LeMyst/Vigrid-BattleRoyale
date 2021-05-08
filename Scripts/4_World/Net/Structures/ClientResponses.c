//--- response obejcts 
class MatchMakingResponse {
    bool wait;
    ref ServerData server;
}
class StartResponse {
    ref PlayerData player;
    ref RegionData region;
}


typedef GenericResponse<ref StartResponse> ClientStartResponse;

typedef GenericResponse<ref MatchMakingResponse> ClientMatchMakeResponse;

typedef GenericResponse<ref PlayerData> GetPlayerResponse;

typedef GenericResponse<ref MatchData> GetMatchResponse;

typedef GenericResponse<ref ServerData> GetServerResponse;
