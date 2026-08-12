/**
 *  KillFeed - why a player died. No guard: the server classifies, the client presents.
 *
 *  Travels as an int inside the entry RPC. WEAPON and MELEE are the only causes that carry a
 *  weapon to render; everything else falls back to an icon and a phrase.
 */
enum KillFeedCause
{
    INVALID = -1, //Do not move this

    WEAPON,      //!< shot by a player - renders the gun with its accessories, shows distance
    MELEE,       //!< struck by a player holding a melee weapon - renders it, no distance
    BAREHANDS,   //!< beaten to death by a player with nothing in hand
    EXPLOSIVE,   //!< grenade or mine; the killer name is only known if something recorded it
    ZONE,        //!< pushed in by the host game through KillFeedAPI.NoteEnvironmentalDamage
    INFECTED,
    ANIMAL,
    ENVIRONMENT, //!< falls, starvation, drowning, and anything that could not be identified

    COUNT //Do not move this
}
