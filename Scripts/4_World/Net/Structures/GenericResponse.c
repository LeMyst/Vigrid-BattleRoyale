class GenericResponse<Class T> {
    bool success;
    string error;
    T data
}

class NoDataResponse {}