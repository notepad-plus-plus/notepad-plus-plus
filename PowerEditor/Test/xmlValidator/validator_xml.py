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


def parse_xml_file(filename_xml):

    # open and read schema file
    #with open(filename_xsd, 'r') as schema_file:
        #schema_to_check = schema_file.read()

    # open and read xml file
    #with open(filename_xml, 'r') as xml_file:
    #    xml_to_check = xml_file.read()

    # parse xml
    try:
        doc = etree.parse(filename_xml)
        #print(f'{filename_xml} XML well formed, syntax ok.')

    # check for file IO error
    except IOError:
        #print('Invalid File')
        post_error(f'{filename_xml}: IOError Invalid File')


    # check for XML syntax errors
    except etree.XMLSyntaxError as err:
        #print('XML Syntax Error, see error_syntax.log')
        post_error(f'{filename_xml}: {str(err.error_log)}: XMLSyntaxError Invalid File')

    # check for general XML errors
    except etree.LxmlError as err:
        #print('XML Error, see error_syntax.log')
        post_error(f'{filename_xml}: {str(err.error_log)}: LxmlError Invalid File')

    except:
        #print('Unknown error.')
        post_error(f'{filename_xml}: Unknown error. Maybe check that no xml version is in the first line.')

def parse_xml_files_from_APIs_dir():

    for file in os.listdir("PowerEditor/installer/APIs"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/APIs", file))
            parse_xml_file(os.path.join("PowerEditor/installer/APIs", file))

def parse_xml_files_from_functionList_dir():

    for file in os.listdir("PowerEditor/installer/functionList"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/functionList", file))
            parse_xml_file(os.path.join("PowerEditor/installer/functionList", file))

def parse_xml_files_from_nativeLang_dir():

    for file in os.listdir("PowerEditor/installer/nativeLang"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/nativeLang", file))
            parse_xml_file(os.path.join("PowerEditor/installer/nativeLang", file))

def parse_xml_files_from_themes_dir():

    for file in os.listdir("PowerEditor/installer/themes"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/installer/themes", file))
            parse_xml_file(os.path.join("PowerEditor/installer/themes", file))

def parse_xml_files_from_src_dir():

    for file in os.listdir("PowerEditor/src"):
        if file.endswith(".xml"):
            #print(os.path.join("PowerEditor/src", file))
            parse_xml_file(os.path.join("PowerEditor/src", file))

print('Start syntax check for xml files.')
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
