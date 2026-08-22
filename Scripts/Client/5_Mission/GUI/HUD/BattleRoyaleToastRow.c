#ifndef SERVER
/**
 *  A toast that is actually on screen: the payload plus when it goes away.
 *
 *  Separate from BattleRoyaleToast because that one is 3_Game - it is the queue's element type and
 *  BattleRoyaleRPC is 3_Game - and knows nothing about being displayed.
 *
 *  THE CLOCKS ARE MINTED WHEN THE RENDERER TAKES THE TOAST, not when the packet arrived. A toast
 *  queued while the HUD was still being built would otherwise have spent part of its life expiring
 *  off screen, and the first notification of a session is exactly the one most likely to land in
 *  that window.
 */
class BattleRoyaleToastRow
{
    ref BattleRoyaleToast toast;

    int shown_at;
    int expires_at;

    /**
     *  Unique per toast, for the lifetime of the process. This is what the renderer matches against
     *  to decide whether a pooled widget is already showing THIS toast.
     *
     *  ⚠️ IT REPLACED A `bound` FLAG ON THIS CLASS, WHICH WAS EXACTLY BACKWARDS. Binding is
     *  model-to-widget-INDEX, and a new toast inserts at index 0 and shifts every existing model
     *  down a slot - so a model that was already flagged bound arrives on a DIFFERENT widget, the
     *  renderer skips SetText, and that widget keeps whatever text it had before. Raising two
     *  bursts of three showed it: the fourth plate had never been bound to anything, so it drew
     *  empty. An id compared against what the WIDGET currently holds survives the shift; a flag on
     *  the model cannot, because the thing that changed is not the model.
     */
    int id;

    private static int s_NextId = 1;

    void BattleRoyaleToastRow( BattleRoyaleToast entry )
    {
        toast = entry;

        id = s_NextId;
        s_NextId = s_NextId + 1;

        shown_at = GetGame().GetTime();
        expires_at = shown_at + entry.duration_ms + BR_TOAST_FADE_IN_MS + BR_TOAST_FADE_OUT_MS;
    }

    //! Is this row inside one of its fades, and therefore in need of a repaint every frame?
    bool IsFading( int now )
    {
        if ( now < shown_at + BR_TOAST_FADE_IN_MS )
            return true;

        return now > expires_at - BR_TOAST_FADE_OUT_MS;
    }

    /**
     *  0..1, ramping up over the first BR_TOAST_FADE_IN_MS and back down over the last
     *  BR_TOAST_FADE_OUT_MS.
     *
     *  Clamped at both ends because neither edge is sampled exactly: a long frame can put `age` past
     *  the fade-in or `left` below zero, and an unclamped ratio there would flash the row to full
     *  brightness on the frame it was supposed to disappear.
     */
    float AlphaAt( int now )
    {
        int age = now - shown_at;
        if ( age < BR_TOAST_FADE_IN_MS )
            return Math.Clamp( age / Math.Max( 1.0, BR_TOAST_FADE_IN_MS ), 0.0, 1.0 );

        int left = expires_at - now;
        if ( left < BR_TOAST_FADE_OUT_MS )
            return Math.Clamp( left / Math.Max( 1.0, BR_TOAST_FADE_OUT_MS ), 0.0, 1.0 );

        return 1.0;
    }
}
#endif
