#!/usr/local/bin/python3

import os
import io
import sys

import requests
from hashlib import sha256
from lxml import etree

api_url = os.environ.get('APPVEYOR_API_URL')
has_error = False


def post_error(message):
    global has_error

    has_error = True

    message = {
        "message": message,
        "category": "error",
        "details": ""
    }

    if api_url:
        requests.post(api_url + "api/build/messages", json=message)
    else:
        from pprint import pprint
        pprint(message)


def parse_xml_file(filename_xml, filename_xsd = None):

    # parse xml
    try:
        doc = etree.parse(filename_xml)
        #print(f'{filename_xml} XML well formed, syntax ok.')

    # check for file IO error
    except IOError:
        #print('Invalid File')
        post_error(f'{filename_xml}: IOError Invalid File')
        return

    # check for XML syntax errors
    except etree.XMLSyntaxError as err:
        #print('XML Syntax Error, see error_syntax.log')
        post_error(f'{filename_xml}: {str(err.error_log)}: XMLSyntaxError Invalid File')
        return

    # check for general XML errors
    except etree.LxmlError as err:
        #print('XML Error, see error_syntax.log')
        post_error(f'{filename_xml}: {str(err.error_log)}: LxmlError Invalid File')
        return

    # unknown error
    except:
        #print('Unknown error.')
        post_error(f'{filename_xml}: Unknown error. Maybe check that no xml version is in the first line.')
        return



    # now validate against correct schema
    if doc and filename_xsd is not None:
        try:
            xmlschema_doc = etree.parse(filename_xsd)

        # error reading XSD
        except IOError:
            post_error(f'{filename_xml} | {filename_xsd}: IOError Invalid File')
            return

        # error parsing XSD
        except etree.XMLSyntaxError as err:
            post_error(f'{filename_xml} | {filename_xsd}: {str(err.error_log)}: XMLSyntaxError Invalid File')
            return

        # other error
        except Exception as err:
            post_error(f'{filename_xml} | {filename_xsd}: Unknown error {str(err.error_log)} reading Schema .xsd file.')
            return

        # Next, extract the schema object from the schema_doc
        try:
            xmlschema = etree.XMLSchema(xmlschema_doc)
            #print(f'{filename_xml} | {filename_xsd}: SCHEMA OBJECT OK')

        # error with Schema
        except etree.XMLSchemaError as err:
            post_error(f'{filename_xml} | {filename_xsd}: {str(err.error_log)}: XMLSchemaError')
            return

        # other error
        except Exception as err:
            post_error(f'{filename_xml} | {filename_xsd}: Unknown error {str(err.error_log)} obtaining schema object')
            return

        # finally, validate the XML against the schema
        if not xmlschema.validate(doc):
            post_error(f'{filename_xml} | {filename_xsd}: Validation error {str(xmlschema.error_log)}')
            return
        else:
            #print(f'{filename_xml} vs schema: VALIDATION OK')
            pass


def parse_xml_files_from_APIs_dir():

    for file in os.listdir("PowerEditor/installer/APIs"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/APIs", file))
            parse_xml_file(os.path.join("PowerEditor/installer/APIs", file), os.path.join("PowerEditor", "Test", "xmlValidator", "autoCompletion.xsd"))

def parse_xml_files_from_functionList_dir():

    for file in os.listdir("PowerEditor/installer/functionList"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/functionList", file))
            xsd_filename = None
            if not file.endswith("overrideMap.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "functionList.xsd")

            parse_xml_file(os.path.join("PowerEditor/installer/functionList", file), xsd_filename)

def parse_xml_files_from_nativeLang_dir():

    for file in os.listdir("PowerEditor/installer/nativeLang"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/nativeLang", file))
            parse_xml_file(os.path.join("PowerEditor/installer/nativeLang", file), os.path.join("PowerEditor", "Test", "xmlValidator", "nativeLang.xsd"))

def parse_xml_files_from_themes_dir():

    for file in os.listdir("PowerEditor/installer/themes"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/themes", file))
            parse_xml_file(os.path.join("PowerEditor/installer/themes", file), os.path.join("PowerEditor", "Test", "xmlValidator", "theme.xsd"))


def parse_xml_files_from_src_dir():

    for file in os.listdir("PowerEditor/src"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/src", file))
            xsd_filename = None
            if file.endswith("stylers.model.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "theme.xsd")
            elif file.endswith("langs.model.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "langs.xsd")
            elif file.endswith("toolbarButtonsConf_example.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "toolbarButtons.xsd")
            elif file.endswith("tabContextMenu_example.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "tabContext.xsd")
            elif file.endswith("contextMenu.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "contextMenu.xsd")
            elif file.endswith("shortcuts.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "shortcuts.xsd")
            elif file.endswith("config.4zipPackage.xml"):
                xsd_filename = os.path.join("PowerEditor", "Test", "xmlValidator", "config.xsd")
            parse_xml_file(os.path.join("PowerEditor/src", file), xsd_filename)

print('Start syntax check and validation for XML files.')
parse_xml_files_from_APIs_dir()
parse_xml_files_from_functionList_dir()
parse_xml_files_from_nativeLang_dir()
parse_xml_files_from_themes_dir()
parse_xml_files_from_src_dir()
print('Done.')

if has_error:
    sys.exit(-2)
else:
    sys.exit()
