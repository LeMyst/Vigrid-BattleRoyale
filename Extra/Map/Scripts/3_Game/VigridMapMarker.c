/**
 *  Vigrid Map - one placed marker. No guard: the server owns the authoritative set and the client
 *  holds a mirror of what it is allowed to see, and both want the same shape.
 *
 *  Deliberately NOT a VigridPartyPing:
 *
 *    - a ping is a transient combat callout - 3 per player, FIFO, 30 s by default, placed by
 *      raycasting where you are looking, and it requires a party;
 *    - a marker is a standing plan - one per player, permanent, placed by clicking anywhere on the
 *      map at any range, and it works solo.
 *
 *  There is therefore no expiry field and no sweep tick anywhere in this addon. That absence is the
 *  whole reason this class is not simply the ping class reused, and it is why the marker store is
 *  genuinely simpler than the ping store rather than a copy of it.
 */
class VigridMapMarker
{
    string owner_uid;  //!< SteamID64. Server-side this is minted from the RPC sender, never read
                       //!< off the payload - a client must not be able to place as someone else.
    int owner_slot;    //!< Party slot at placement time, -1 when solo. Drives the colour only.
    vector pos;        //!< World position, y forced to 0.
    string label;      //!< Player text, truncated and sanitised server-side.
    int created_ms;    //!< Diagnostics only. Markers never expire, so nothing reads this to decide
                       //!< visibility - it exists so a stuck marker can be aged in a log.

    void VigridMapMarker(string _owner_uid, int _owner_slot, vector _pos, string _label, int _created_ms)
    {
        owner_uid = _owner_uid;
        owner_slot = _owner_slot;
        pos = _pos;
        label = _label;
        created_ms = _created_ms;
    }
}
