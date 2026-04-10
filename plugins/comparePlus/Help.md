**Some Notepad++ Features**

*Single-View mode:*
![image](https://cloud.githubusercontent.com/assets/10229320/23313224/7a845b98-fac5-11e6-8c03-51e0497c139e.png)

*Double-View mode:*
![image](https://cloud.githubusercontent.com/assets/10229320/23313393/29c7a4ac-fac6-11e6-880a-10c5f783ca9e.png)


You can set the Double-View mode to horizontal or vertical by right-clicking the gripper and using the "Rotate" commands:
![image](https://cloud.githubusercontent.com/assets/10229320/23313809/e78d5f08-fac7-11e6-9207-95edb3a09582.png)


It's recommended to restart Notepad++ after shifting from horizontal to vertical split or vice-versa.

**ComparePlus Concept**

ComparePlus (CP) assumes that you would normally want to compare an old version of your work vs. its new version.
CP regards the new file as the (more) important one. Your **focal** file.

**Compare**

You can initiate a Compare in three ways:

**1.** Open multiple files (A, B, C, D, - D is active), press "Set as First to Compare", activate file B and press "Compare".
- File B - *active when you pressed "Compare"* - is regarded as the new file and compared to the old file D (default - see **Settings**).
- The new file is positioned at the right view (default).

**2.** Open two files (A, B) and press "Compare".
- File B (new) is compared to file A.

**3.** Open multiple files (A, B, C, D), right-click file D's tab and press "Move to Other View" (Notepad++ is now in Double-View mode).
Press "Compare".
- File D (new, default) is compared to file C.

**Symbols**

*Added "+":* The line only exists in **this new file**. IOW: the line has been added to the new file, and does not exist in the old one.

*Removed "-":* The line does not exist in **the other new file**. IOW: the line has been removed in the new file, and only exists in this old file.

- The "+" symbol will only appear in the new file, and the "-" symbol will only appear in the old file.

![image](https://cloud.githubusercontent.com/assets/10229320/23319986/8870fe70-fae1-11e6-922b-3557f0619cdb.png)

**
*NOTE:*
If you uncheck the "Detect Moves" option, the meaning of the "+" and "-" symbols would be as follows:

*Added "+":* The line exists **in this location only in this new file**. It may be in the old file too but in a different location.

*Removed "-":* The line does not exist **in this location in the other new file**. It may be in the new file too but in a different location.
**

*Moved:* The line appears *once* in the other file and in a different location.
![image](https://cloud.githubusercontent.com/assets/10229320/23321414/1d565642-fae8-11e6-8aad-f7e3d5921f00.png)

*Moved-Multiple:* The line appears *multiple times* in the other file and in different locations.

*Changed:* Most of the line is identical in both files. Some changes have been made.
![image](https://cloud.githubusercontent.com/assets/10229320/23321717/449eb086-fae9-11e6-9801-de449da22981.png)

**Menu**

*Compare:* Compare all the lines in both files.

*Compare Selected Lines:* Compare only the selected blocks in both files.

*Diff since last Save:* Compare the active *modified and currently unsaved* file to its contents when it was last saved to the disk.

*SVN/Git Diff:*

**Settings**

*First is:* Determines whether the file "Set as First to Compare" should be regarded as the old or new file.

*Old file position:* Determines whether the old file should be positioned at the left or right view (top/bottom in vertical split).

*Single-view default compare to:* Determines whether the active file in Single-View mode should be compared to its previous or next file (on "Compare" without "Set First").

*Warn about encodings mismatch:* Check to display a warning message on trying to compare two files with different encodings (the results might be inaccurate).

*Prompt to close files on match:* Check to display an option in the "Files Match" message to close the compared files.

*Align replacements:* If checked, an added (or moved) line in the new file would be aligned with its counterpart removed line in the old file.

If unchecked, blank lines would be inserted for added/removed/moved lines in both files (*old CP implementation*).
![image](https://cloud.githubusercontent.com/assets/10229320/23320254/9f1621cc-fae2-11e6-866a-23cd4347ca9d.png)

*Wrap around diffs:* Determines whether the "Next" command should be enabled on reaching the last diff and go to the first diff.

*Re-Compare active on Save:* Check to automatically re-Compare the active Compare on saving its files.

*Go to first diff after re-Compare:* If unchecked, the caret position would not change on re-Compare.

*Compact Navigation Bar:*

**Shortcuts**

The old `Alt+D` has been removed in order to avoid conflict with Windows convention of Alt+LETTER opening the menu.

You can set your own shortcuts to all CP commands via Notepad++ Settings -> Shortcut Mapper -> Plugin Commands.

**Icons**

You can use the excellent **Customize Toolbar** plugin (by Dave) to add, move and remove any toolbar buttons.
***
**[Requests and Issues-Report](https://github.com/pnedev/comparePlus/issues)**
**Thank you for [Donating](https://www.paypal.com/paypalme/pnedev).**