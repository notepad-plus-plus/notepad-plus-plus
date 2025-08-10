#!/usr/bin/env python
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Introduces a styles check utility. This can be used to help check for
# possible updates needed to apply for a given theme.

from enum import Enum
from enum import auto
from pathlib import Path
import xml.etree.ElementTree as ET
import argparse

# version of this script
__version__ = '0.0.0'

# default filename for the stylers model
DEFAULT_STYLERS_MODEL = 'stylers.model.xml'

# expected tag at the root of the xml model/style document
XML_ROOT_TAG = 'NotepadPlus'

# xml query for finding lexer types
XML_MATCH_LEXER_TYPE = './LexerStyles/LexerType'

# xml query for finding word styles
XML_MATCH_WORD_STYLES = './WordsStyle'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('themes', nargs='*',
        help='Theme(s) to check against')
    parser.add_argument('--all', '-A', action='store_true',
        help='Perform all checks')
    parser.add_argument('--model',
        help=f'The stylers model definition (default: {DEFAULT_STYLERS_MODEL})')
    parser.add_argument('--version', '-v', action='version',
        version='%(prog)s ' + __version__)
    parser.add_argument('--ws', action='store_true',
        help='Include checks for word styles')
    args = parser.parse_args()

    # find default paths
    pe_dir = Path(__file__).parent.parent
    src_dir = pe_dir / 'src'
    installer_dir = pe_dir / 'installer'
    default_themes_dir = installer_dir / 'themes'

    # determine stylers model to use
    if args.model:
        stylers_model = Path(args.model)
    else:
        stylers_model = src_dir / DEFAULT_STYLERS_MODEL

    if not stylers_model.is_file():
        raise SystemExit(f'missing theme model: {stylers_model}')

    # compile a list of themes to check against
    theme_search_list = args.themes
    if not args.themes:
        theme_search_list = [ default_themes_dir ]

    theme_files = []
    unknown_themes = []
    for theme_arg in theme_search_list:
        theme_path = Path(theme_arg)
        if theme_path.is_file():
            theme_files.append(theme_path)
        elif theme_path.is_dir():
            found_themes = theme_path.glob('*.xml')
            for found_theme in found_themes:
                if found_theme.is_file():
                    theme_files.append(found_theme)
        else:
            unknown_themes.append(theme_path)

    for unknown_theme in unknown_themes:
        possible_theme = default_themes_dir / f'{unknown_theme}.xml'
        if possible_theme.is_file():
            theme_files.append(possible_theme)
        else:
            raise SystemExit(f'unknown theme provided: {theme_path}')

    if not theme_files:
        raise SystemExit('no themes to process')

    # build options and perform a check
    opts = {
        'model': stylers_model,
        'themes': theme_files,
        'word-styles': args.all or args.ws,
    }

    issue_count = check(opts)

    if issue_count == 0:
        print('\nLooks good!')
    else:
        raise SystemExit(f'\nDetected an issue (total: {issue_count})!')


def check(opts):
    issue_count = 0

    print(f'Loading styles model: {opts['model']}')
    tree = ET.parse(opts['model'])
    root = tree.getroot()
    if root.tag != XML_ROOT_TAG:
        raise SystemExit(f'unknown model definition (no "{XML_ROOT_TAG}" tag)')

    default_db = extract_style_lexers(root)

    print(f'Styles model defines {len(default_db)} lexers types.')
    theme_count = len(opts['themes'])
    if theme_count > 1:
        print(f'Processing {theme_count} themes.')
    print()

    # sort lexer styles to give a nicer presentation
    lexer_types = {}
    for k, v in default_db.items():
        lexer_types[k] = v[DbKey.Description]
    lexer_types = dict(sorted(lexer_types.items(), key=lambda item: item[1]))

    # process each provided theme
    for xmlfile in opts['themes']:
        tree = ET.parse(xmlfile)
        root = tree.getroot()
        if root.tag != 'NotepadPlus':
            continue

        print(xmlfile.stem)
        db = extract_style_lexers(root)

        missing_lexers = default_db.keys() - db.keys()
        unknown_lexers = db.keys() - default_db.keys()
        matching_lexers = set(default_db.keys()).intersection(db.keys())

        for lexer_type, lexer_name in lexer_types.items():
            if lexer_type in missing_lexers:
                print(f'  MISSING: {lexer_name} ({lexer_type})')
                issue_count += 1
                continue

        for unknown_lexer in unknown_lexers:
            ule_desc = db[unknown_lexer].get(DbKey.Description, unknown_lexer)
            print(f'  UNKNOWN: {ule_desc} ({unknown_lexer})')
            issue_count += 1

        # check for any word style inconsistencies
        if opts['word-styles']:
            for lexer_type in matching_lexers:
                default_ws = default_db[lexer_type][DbKey.WordsStyle]
                db_ws = db[lexer_type][DbKey.WordsStyle]

                missing_ws = default_ws.keys() - db_ws.keys()
                unknown_ws = db_ws.keys() - default_ws.keys()

                if missing_ws or unknown_ws:
                    lexer_name = lexer_types[lexer_type]
                    print(f'  PARTIAL: {lexer_name} ({lexer_type})')

                for missing in missing_ws:
                    ws_name = default_ws[missing]
                    print(f'    MISSING: {ws_name} ({missing})')
                    issue_count += 1

                for unknown in unknown_ws:
                    ws_name = db_ws[unknown]
                    print(f'    UNKNOWN: {ws_name} ({unknown})')
                    issue_count += 1

    return issue_count


class DbKey(Enum):
    Description = auto()
    WordsStyle = auto()


def extract_style_lexers(root):
    lexer_db = {}

    for lexer_entry in root.findall(XML_MATCH_LEXER_TYPE):
        lexer_name = lexer_entry.attrib.get('name')
        lexer_desc = lexer_entry.attrib.get('desc')

        lexer_db[lexer_name] = {
            DbKey.Description: lexer_desc,
            DbKey.WordsStyle: {},
        }

        for lexer_ws in lexer_entry.findall(XML_MATCH_WORD_STYLES):
            ws_id = lexer_ws.attrib.get('styleID')
            if not ws_id:
                continue

            ws_name = lexer_ws.attrib.get('name')
            lexer_db[lexer_name][DbKey.WordsStyle][ws_id] = ws_name

    return lexer_db


if __name__ == '__main__':
    main()
