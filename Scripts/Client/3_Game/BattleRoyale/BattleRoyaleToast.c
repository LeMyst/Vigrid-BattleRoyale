#ifndef SERVER
/**
 *  One queued notification, waiting for the HUD to pick it up.
 *
 *  This exists in 3_Game rather than beside the renderer because BattleRoyaleRPC receives the
 *  NotificationMessage packet and BattleRoyaleRPC is 3_Game - a 3_Game class cannot name a 5_Mission
 *  type, so the queue's element type has to live down here. Same split, and for the same reason, as
 *  KillFeedEntry (3_Game) against KillFeedUI (5_Mission).
 *
 *  `text` is whatever should be handed to SetText, and the two producers hand over different things
 *  on purpose:
 *
 *    - the RPC path has already run StringLocaliser and BattleRoyaleKeyTokens over the server's bare
 *      key, because the %1..%5 substitution and the READY_KEY / UNSTUCK_KEY tokens both have to
 *      happen before anything can be displayed. What arrives here is finished text.
 *    - BattleRoyaleClient.NotifyLocal passes a '#key' untouched and lets the widget resolve it,
 *      because nothing crossed a stage boundary and there is nothing to substitute.
 *
 *  SetText is happy with either - it resolves every #token in the string it is given - so the
 *  renderer does not need to know which kind it has.
 */
class BattleRoyaleToast
{
    string text;

    //! How long the toast should be fully visible, in milliseconds. The fades are added on top.
    int duration_ms;

    void BattleRoyaleToast( string message, float seconds )
    {
        text = message;
        duration_ms = Math.Round( seconds * 1000 );
    }
}
#endif
