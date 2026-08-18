/**
 *  One colour per PARTY, for the admin spectate overlay. THE one source of truth for them.
 *
 *  No side guard, and 3_Game for the same reason VigridPartyPalette is: it has consumers in two
 *  different stages on the client - BattleRoyaleSpectatorTags (5_Mission) draws the name, and
 *  BattleRoyaleClient (5_Mission) resolves the same colour again to push it into the map addon,
 *  which must never learn what a party is. 3_Game is the lowest stage both can reach, and a server
 *  that one day wants to name a team in a log line can reach it too.
 *
 *  ⚠ THIS IS NOT VigridPartyPalette, AND THE TWO ANSWER DIFFERENT QUESTIONS. The palette is one
 *  colour per party SLOT - "which of my teammates called that out" - and wraps at eight, which is
 *  right when the party is at most sixteen and you are in it. This is one colour per PARTY - "are
 *  those two on the same side" - asked by somebody outside every party in a match that can hold
 *  thirty of them. Feeding a slot index in here, or a party index into the palette, produces a
 *  plausible-looking colour that answers the other question; that was the bug (#276), where the
 *  server sent GetMemberIndex and every party's first member came out amber.
 *
 *  See BR_TEAM_COLOUR_* in BattleRoyaleConstants.c for why the walk is the golden angle rather than
 *  a random draw, and for the honest limit on how many teams can really be told apart.
 */
class BattleRoyaleTeamColour
{
    /**
     *  Colour for the party at match-local index `party_index`, at opacity `alpha` (0..1).
     *
     *  A NEGATIVE INDEX MEANS SOLO and reads plain white - deliberately, because colouring a player
     *  with no team implies a team that does not exist. The server sends -1 both for a genuinely
     *  unpartied player and for every player on a server whose party manager is switched off, so
     *  this one branch covers both without the caller testing anything.
     *
     *  The opacity is baked into the ARGB rather than left to the widget, matching VigridPartyPalette
     *  and for the same two reasons: a TextWidget's SetColor carries its own alpha, and the map layer
     *  hands the value to a CanvasWidget, which has no widget in the chain to apply an alpha to.
     */
    static int ForParty(int party_index, float alpha)
    {
        int a = Math.Round(Math.Clamp(alpha, 0, 1) * 255);

        if (party_index < 0)
            return BR_SPECTATE_TAG_SOLO_COLOUR;

        //--- Unbounded on purpose: the walk wraps the hue circle by itself and never runs out, so
        //--- there is no modulo on the index and no "ran out of colours" case to handle. Kept as a
        //--- float multiply rather than a running total so it cannot drift or depend on call order.
        float hue = Math.ModFloat(party_index * BR_TEAM_COLOUR_GOLDEN_ANGLE, 360.0);

        //--- One declaration per name per scope, so the tier picks into variables declared up here
        //--- rather than inside the branches. Written as an if-chain rather than a lookup table
        //--- because a global `static const ref array` has no precedent anywhere in this repo and is
        //--- not the construct to prove out in a colour helper.
        float sat = BR_TEAM_COLOUR_SAT_A;
        float val = BR_TEAM_COLOUR_VAL_A;

        int tier = party_index % BR_TEAM_COLOUR_TIERS;

        if (tier == 1)
        {
            sat = BR_TEAM_COLOUR_SAT_B;
            val = BR_TEAM_COLOUR_VAL_B;
        }
        else if (tier == 2)
        {
            sat = BR_TEAM_COLOUR_SAT_C;
            val = BR_TEAM_COLOUR_VAL_C;
        }
        else if (tier == 3)
        {
            sat = BR_TEAM_COLOUR_SAT_D;
            val = BR_TEAM_COLOUR_VAL_D;
        }
        else if (tier == 4)
        {
            sat = BR_TEAM_COLOUR_SAT_E;
            val = BR_TEAM_COLOUR_VAL_E;
        }
        else if (tier == 5)
        {
            sat = BR_TEAM_COLOUR_SAT_F;
            val = BR_TEAM_COLOUR_VAL_F;
        }

        return HsvToArgb(hue, sat, val, a);
    }

    /**
     *  Standard HSV -> RGB, hue in degrees, s and v in 0..1, alpha already in 0..255.
     *
     *  Written out rather than reached for: the engine exposes no colour-space helper, and the two
     *  vanilla tint paths (ARGB and Widget.SetColor) both want a packed int at the end anyway.
     */
    private static int HsvToArgb(float hue, float sat, float val, int a)
    {
        float c = val * sat;
        float h = hue / 60.0;

        //--- The classic second chord. Split off its own line because the whole expression in one
        //--- go is exactly the shape this engine rejects as "Formula too complex".
        float h_mod = Math.ModFloat(h, 2.0);
        float x = c * (1.0 - Math.AbsFloat(h_mod - 1.0));
        float m = val - c;

        float r = 0;
        float g = 0;
        float b = 0;

        if (h < 1.0)
        {
            r = c;
            g = x;
        }
        else if (h < 2.0)
        {
            r = x;
            g = c;
        }
        else if (h < 3.0)
        {
            g = c;
            b = x;
        }
        else if (h < 4.0)
        {
            g = x;
            b = c;
        }
        else if (h < 5.0)
        {
            r = x;
            b = c;
        }
        else
        {
            r = c;
            b = x;
        }

        int red = Math.Round(Math.Clamp(r + m, 0, 1) * 255);
        int green = Math.Round(Math.Clamp(g + m, 0, 1) * 255);
        int blue = Math.Round(Math.Clamp(b + m, 0, 1) * 255);

        return ARGB(a, red, green, blue);
    }
}
