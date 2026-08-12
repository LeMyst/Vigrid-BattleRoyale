/**
 *  SafeZone - shared constants. No guard: this file compiles on both client and server.
 *
 *  Short by design. The addon has no assets, no settings file and no RPC namespace: state reaches
 *  the client on a netsync bool riding the player entity, so there is nothing else to name here.
 */

static const string VIGRID_SAFEZONE_VERSION = "0.1.0";

#ifdef DIAG
#define VIGRID_SAFEZONE_TRACE_ENABLED
#endif

//--- Log verbosity. Overridden at runtime by -safezone-trace/-safezone-debug/-safezone-info/
//--- -safezone-warn/-safezone-none on the command line, or by SafeZoneLogLevel in serverDZ.cfg.
#ifdef VIGRID_SAFEZONE_TRACE_ENABLED
    static const int VIGRID_SAFEZONE_LOG_LEVEL = 4; // Trace
#else
    static const int VIGRID_SAFEZONE_LOG_LEVEL = 0; // Error only
#endif
