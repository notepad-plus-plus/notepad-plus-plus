# -*- coding: utf-8 -*-
# Show problem with character value truncation causing U+0121 'ġ' (LATIN SMALL LETTER G WITH DOT ABOVE)
# to be styled as an operator as static_cast<char>(0x121) = 0x21 == '!' which is an operator

# Isolate
ġ

# Continuing from operator
(ġ)
