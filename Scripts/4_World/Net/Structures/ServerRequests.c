
class OnStartRequest {
    string query_port;
    string server_version;
    string server_ip;
}

class OnFinishRequest {
    string winner;
    string query_port;
    string server_ip;
}

class SetLockRequest {
    bool lock;
    string query_port;
    string server_ip;
}