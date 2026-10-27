"""Give the recovered player music helpers their complete native signature."""


def transform(text):
    # EndMusic's second retail argument is unused. On x86, MSVC may still
    # reuse its stack slot while tail-calling the music restart function.
    # Omitting that argument overwrites a caller's saved nonvolatile register.
    declaration = 'extern int _ZN5Sound8EndMusicEjj(unsigned char);'
    call = '_ZN5Sound8EndMusicEjj(*(unsigned char*)(c+0x6d8));'
    assert text.count(declaration) == 4, 'player music declaration inventory changed'
    assert text.count(call) == 6, 'player music call inventory changed'
    text = text.replace(declaration,
                        'extern void _ZN5Sound8EndMusicEjj(unsigned int, unsigned int);')
    return text.replace(call, '_ZN5Sound8EndMusicEjj(*(unsigned char*)(c+0x6d8), 0u);')
