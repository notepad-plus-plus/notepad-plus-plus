# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
import unittest
import hmac
import hashlib
from migrate_shortcuts import migrate, migrate_validation

class MigrationTests(unittest.TestCase):
    def test_preserves_unknown_fields_and_custom_commands(self):
        first = b'<Command name="Get PHP help" Alt="yes" Key="112">https://www.php.net/$(CURRENT_WORD)</Command>'
        second = b'<Command name="Wikipedia Search" Alt="yes" Key="114">https://en.wikipedia.org/wiki/Special:Search?search=$(CURRENT_WORD)</Command>'
        data = b'<?xml version="1.0" encoding="utf-8"?>\r\n<NotepadPlus custom="keep"><!-- untouched -->\r\n<Macros><Macro name="Mine" future="keep"/></Macros><UserDefinedCommands>' + first + b'<Command name="Get PHP help" Key="70">my-custom-tool</Command>' + second + b'<Future>keep</Future></UserDefinedCommands><InternalCommands><Shortcut id="1" Key="65"/></InternalCommands><Unknown value="keep"/></NotepadPlus>'
        result, count = migrate(data)
        self.assertEqual(count, 2)
        self.assertEqual(result, data.replace(first, b'').replace(second, b''))
        self.assertEqual(migrate(result), (result, 0))
    def test_comments_are_untouched(self):
        data = b'<NotepadPlus><!-- <UserDefinedCommands><Command name="Get PHP help">https://www.php.net/$(CURRENT_WORD)</Command></UserDefinedCommands> --></NotepadPlus>'
        self.assertEqual(migrate(data), (data, 0))

    def test_preserves_existing_trust_without_validating_changed_commands(self):
        old, new, key = b'original fixture', b'migrated fixture', b'synthetic machine key'
        digest = hmac.new(key, old, hashlib.sha256).hexdigest().encode()
        replacement = hmac.new(key, new, hashlib.sha256).hexdigest().encode()
        config = b'<NotepadPlus unknown="keep"><GUIConfigs><GUIConfig name="shortcutsXmlHMAC" future="keep" value="'+digest+b'"/><GUIConfig name="Other" custom="keep"/></GUIConfigs></NotepadPlus>'
        self.assertEqual(migrate_validation(config, old, new, key), config.replace(digest,replacement))
        self.assertEqual(migrate_validation(config, b'untrusted modification', new, key), config)

if __name__ == '__main__':
    unittest.main()
