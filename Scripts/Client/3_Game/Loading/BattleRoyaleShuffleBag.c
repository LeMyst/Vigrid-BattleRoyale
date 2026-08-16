/**
 *  A shuffle bag over a list of strings: keep handing out elements at random, but never repeat one
 *  until every other has had a turn.
 *
 *  ⚠️ DELIBERATELY UNGUARDED, AND IT HAS TO BE. Its only consumer, LoadingScreenBackground, carries
 *  no guard either, so it compiles on the SERVER as well - where a `#ifndef SERVER` type simply does
 *  not exist and the whole Game module fails with "Bad type 'BattleRoyaleShuffleBag'". The
 *  ExpansionArray this replaced never hit that because Expansion Core is unguarded too.
 *
 *  An offline client cannot catch this by construction: SERVER is undefined there, so the guarded
 *  type is always present and the module always compiles. It takes a real server launch.
 *
 *  This replaced ExpansionArray<string>.GetQuasiRandomElementAvoidRepetition(), which was the only
 *  thing the loading screen wanted from DayZ-Expansion and the last unguarded Expansion symbol
 *  outside the notification path. A plain GetRandomElement() is not a substitute: with twelve
 *  loading screens an independent draw shows the same picture twice in a row about one load in
 *  twelve, which is exactly the case a player notices.
 *
 *  The bag holds INDICES rather than copies of the elements, so refilling it costs nothing and the
 *  caller's array stays the single source of truth.
 *
 *  ONE BEHAVIOURAL DIFFERENCE FROM THE ORIGINAL, AND IT IS A FIX. Expansion's version stored the
 *  bag POSITION it had drawn from and then compared that against ELEMENT indices when refilling, so
 *  the element it excluded was rarely the one it had just returned; its guard was also written
 *  `> 0`, which let element 0 repeat across a refill regardless. Here `m_Last` is the element index,
 *  and the test is `>= 0`, so a back-to-back repeat across the refill boundary genuinely cannot
 *  happen. The single-element case is the one exception and has to be: with one picture there is
 *  nothing else to show, so the guard is skipped rather than emptying the bag forever.
 */
class BattleRoyaleShuffleBag
{
    private ref array<string> m_Elements;
    private ref array<int> m_Bag;

    //! Element index handed out last, or -1 before the first draw. NOT a position in m_Bag.
    private int m_Last;

    void BattleRoyaleShuffleBag( array<string> elements )
    {
        if ( elements )
            m_Elements = elements;
        else
            m_Elements = new array<string>();

        m_Bag = new array<int>();
        m_Last = -1;
    }

    int Count()
    {
        return m_Elements.Count();
    }

    string Draw()
    {
        int count = m_Elements.Count();
        if ( count == 0 )
            return "";

        if ( m_Bag.Count() == 0 )
            Refill( count );

        //--- Pick a slot in the bag, take the element index it holds, and retire the slot.
        int slot = m_Bag.GetRandomIndex();
        int index = m_Bag.Get( slot );
        m_Bag.Remove( slot );

        m_Last = index;

        return m_Elements.Get( index );
    }

    private void Refill( int count )
    {
        for ( int i = 0; i < count; i++ )
        {
            //--- Hold back whatever was drawn last, so the first pick of the new bag cannot repeat
            //--- the final pick of the old one. Skipped when there is only one element, or the bag
            //--- would refill empty and Draw() would have nothing to take.
            if ( count > 1 && m_Last >= 0 && i == m_Last )
                continue;

            m_Bag.Insert( i );
        }
    }
}
