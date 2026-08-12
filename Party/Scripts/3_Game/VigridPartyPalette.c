/**
 *  Vigrid Party - the per-slot marker colours. THE one source of truth for them.
 *
 *  No side guard: nothing here is client-only, and the server records a slot on a placed marker
 *  that it may one day want to describe in a log line.
 *
 *  It lives in 3_Game rather than beside its original caller because it now has three consumers in
 *  three different stages - VigridPartyPings (5_Mission), VigridPartyAPI's client block (4_World),
 *  and through that API the map addon - and 3_Game is the lowest stage all of them can reach. It is
 *  also the stage VIGRID_PARTY_PING_PALETTE_SIZE already lives in, and a palette whose size and
 *  values sit in the same file cannot drift apart.
 *
 *  Colours are one per party slot rather than Carim's own-vs-teammate split: with a single shade for
 *  every teammate you can see that somebody called something out but not who, which is the thing
 *  worth knowing when two people are marking at once. The roster is join-ordered and never
 *  reshuffled (see VigridPartyAPI), so a member keeps one colour for the life of the party.
 */
class VigridPartyPalette
{
    /**
     *  Colour for the member in party slot `slot`, at opacity `alpha` (0..1).
     *
     *  Nothing is special-cased for the local player: your markers are drawn in your own slot
     *  colour, so what you see is what your team sees. A negative slot means the owner is not on the
     *  roster - which happens for a frame or two when a ping set arrives just before the roster that
     *  explains it - and reads off-white.
     *
     *  The opacity is baked into the colour rather than left to the widget hierarchy, and there are
     *  now two independent reasons for that:
     *
     *    - an ImageWidget takes its alpha from its own colour and ignores the parent's, so setting
     *      the icon to a fully opaque tint is exactly what pinned it at full brightness while the
     *      text around it faded. PingIcon carries `inheritalpha 0` to match;
     *    - CanvasWidget.DrawLine takes a plain ARGB int and there is no widget in the chain to apply
     *      an alpha to at all, so on the map baking it in is the only way to get a faded stroke.
     */
    static int ColourForSlot(int slot, float alpha)
    {
        int a = Math.Round(Math.Clamp(alpha, 0, 1) * 255);

        if (slot < 0)
            return ARGB(a, 232, 228, 220); // off-white

        //--- Wraps rather than running out: max_party_size goes to 16, and two members sharing a
        //--- colour in a party that large is better than a marker with no colour at all.
        int index = slot % VIGRID_PARTY_PING_PALETTE_SIZE;

        if (index == 0)
            return ARGB(a, 242, 199, 68);  // amber
        if (index == 1)
            return ARGB(a, 79, 195, 232);  // cyan
        if (index == 2)
            return ARGB(a, 181, 123, 232); // violet
        if (index == 3)
            return ARGB(a, 123, 216, 123); // green
        if (index == 4)
            return ARGB(a, 232, 130, 46);  // orange
        if (index == 5)
            return ARGB(a, 232, 123, 181); // pink
        if (index == 6)
            return ARGB(a, 123, 155, 232); // blue

        return ARGB(a, 199, 232, 123);     // lime
    }
}
