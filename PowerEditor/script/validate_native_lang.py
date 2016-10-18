#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os
import subprocess
import xml.etree.ElementTree as ET

def get_item_id(item):
    # item type can be :
    # {'id': '2204', 'name': 'Bold'}
    # {'subMenuId': 'file-openFolder', 'name': 'Open Containing Folder'}
    # {'message': 'Editing contextMenu.xml allows you to modify your Notepad++ popup context menu.\\rYou have to restart your Notepad++ to take effect after modifying contextMenu.xml.', 'title': 'Editing contextMenu'}
    # {'menuId': 'file', 'name': '&File'}
    # {'name': 'Doc Switcher'}
    # {'CMID': '2204', 'name': 'Close'}
    # {}
    res = None
    if 'tag' in item and item['tag']=='Item':
        keys = ['id', 'subMenuId', 'menuId', 'CMID' ]
        for key in keys:
            if key in item:
                res = item[key]
    elif 'tag' in item:
        res = item['tag']
    return res

def read_child(obj, log, parsedTree):
    if len(obj):
        for child in obj:
            newLog = log
            if log:
                newLog += " - "
            newLog += "{}".format(obj.tag)
            read_child(child, newLog, parsedTree)
    else:
        if not log in parsedTree:
            parsedTree[log] = list()

        res = obj.attrib
        res['tag'] = obj.tag
        parsedTree[log].append(obj.attrib)

def read_xml(path):
    tree = ET.parse(path)
    root = tree.getroot()
    parsedTree = dict()
    read_child(root[0], "", parsedTree)
    return parsedTree

def compare_langs(english, otherObj):
    errors = list()
    for key, items in english.items():
        if not key in otherObj:
            errors.append("key {} is missing in other language".format(key))
        else:
            # check each english elements has a traduction in the other language
            for item in items:
                id = get_item_id(item)
                traduction = None
                for elt in otherObj[key]:
                    other_id = get_item_id(elt)
                    if other_id and other_id == id:
                        traduction = elt
                if not traduction:
                    errors.append("traduction not found on '{}' - '{}'".format(key, item))
    return errors


def main():
    # read the english file that is the reference
    try:
        englishLang = read_xml('../installer/nativeLang/english.xml')
    except Exception as inst:
        print "can't read english.xml : {}".format(inst)
        
    for file in os.listdir("../installer/nativeLang/"):
        if file.endswith(".xml") and file != "english.xml":
            lang = None
            errors = list()
            try:
                lang = read_xml( "../installer/nativeLang/" + file)
                errors = compare_langs(englishLang, lang)
            except Exception as inst:
                errorMsg = "Exception while reading or comparing {} : {}".format(file, inst)
                print errorMsg
                if 'APPVEYOR' in os.environ and os.environ['APPVEYOR']=="True":
                    # we are on AppVeyor environement, we can report an error on the xml file.
                    # appveyor AddMessage <message> [options]
                    subprocess.call(["appveyor", "AddMessage", errorMsg, "-Category", "Error"])

            if len(errors):
                if 'APPVEYOR' in os.environ and os.environ['APPVEYOR']=="True":
                    # we are on AppVeyor environement, we can report an error on the xml file.
                    # appveyor AddMessage <message> [options]
                    errorMsg = "native lang file {} contains errors. See Details in build log.".format(file)
                    subprocess.call(["appveyor", "AddMessage", errorMsg, "-Category", "Warning"])

                for error in errors:
                    print "{} - {}".format(file, error)

if __name__ == "__main__":
    main()