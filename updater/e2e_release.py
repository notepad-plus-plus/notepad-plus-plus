# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Exercise the native updater against two real public GitHub releases in an isolated profile."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
import time
import urllib.request
import zipfile
from verify_release import verify

REPO = Path(__file__).resolve().parents[1]
BASE = 'https://github.com/berkkivircik/NoteQu/releases/download/'
SAMPLE = 'NoteQu synthetic update — café, Türkçe, 日本語.\nUnsaved recovery document.\n'

def download(url, target):
    with urllib.request.urlopen(urllib.request.Request(url, headers={'User-Agent':'NoteQu-Release-Verification'}), timeout=60) as response:
        target.write_bytes(response.read())

def payload(version, system, arch, work):
    folder = work / version; folder.mkdir()
    base = BASE + 'v' + version + '/'
    name = f'NoteQu-update-{system}-{arch}.json'
    for file in [name, name+'.sig', f'NoteQu-{version}-{system}-{arch}-update.zip']:
        download(base+file, folder/file)
    archive = folder/f'NoteQu-{version}-{system}-{arch}-update.zip'
    manifest = verify((folder/name).read_bytes(), (folder/(name+'.sig')).read_bytes(), archive, system, arch, version)
    return archive, manifest

def wait_for(profile, state, timeout=240):
    deadline = time.monotonic()+timeout
    while time.monotonic()<deadline:
        events = profile/'update-events.jsonl'
        text = events.read_text() if events.exists() else ''
        if state in text: return
        if 'message:' in text: raise RuntimeError('Updater reported an error: '+text)
        time.sleep(.25)
    raise TimeoutError('Updater did not reach '+state)

def install_payload(archive, app, manifest):
    # Bootstrap only: verified application-owned files, never a complete portable overlay.
    with zipfile.ZipFile(archive) as z:
        for name, spec in manifest['files'].items():
            target = app/name; target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(z.read(name))
            if spec.get('executable'): target.chmod(0o755)

def hashes(paths):
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}

def windows_test(work, profile, archive, manifest, target_version):
    from windows_driver import wait_window, windows, text, close, command, menus
    previous = work/'previous.zip'
    download(BASE+'v1.3.3/NoteX-1.3.3-portable-x64.zip', previous)
    assert hashlib.sha256(previous.read_bytes()).hexdigest() == '3ebafcaf25ed9739db795cbe1af051cfe780d95fef7d2a20b4b1fc01767299d4'
    with zipfile.ZipFile(previous) as z: z.extractall(work/'bootstrap')
    app = next((work/'bootstrap').rglob('NoteX.exe')).parent; exe=app/'NoteX.exe'
    shutil.copy2(app/'config.xml',profile/'config.xml')
    protected=[]
    for name in ['themes/My Theme.xml','userDefineLangs/my-language.xml','plugins/config/Test.ini','my-personal-note.txt']:
        p=app/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('<NotepadPlus><LexerStyles/><GlobalStyles/></NotepadPlus>' if name.endswith('.xml') else 'Synthetic preserved data: '+name);protected.append(p)
    shortcuts=b'<NotepadPlus future="preserve"><Macros><Macro name="Custom Macro" Ctrl="yes" Alt="no" Shift="yes" Key="90"/></Macros><UserDefinedCommands><Command name="Custom Command" Ctrl="no" Alt="yes" Shift="no" Key="115">cmd /c echo fixture</Command></UserDefinedCommands><Unknown keep="yes"/></NotepadPlus>'
    (profile/'shortcuts.xml').write_bytes(shortcuts);protected.append(profile/'shortcuts.xml')
    # Compile a genuine, minimal plugin and exercise its command before and after updating.
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    buildtools=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    plugin=app/'plugins/UpdateFixture/UpdateFixture.dll';plugin.parent.mkdir(parents=True)
    script=work/'build-fixture.cmd'
    script.write_text('@echo off\ncall "'+buildtools+'\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /std:c++20 /EHsc /MT /LD /DUNICODE /D_UNICODE /I"'+str(REPO/'scintilla/include')+'" /I"'+str(REPO/'PowerEditor/src/MISC/PluginsManager')+'" "'+str(REPO/'updater/fixtures/plugin.cpp')+'" /Fe:"'+str(plugin)+'"\nexit /b %errorlevel%\n',encoding='utf-8')
    subprocess.run(['cmd','/c',str(script)],cwd=work,check=True);protected.append(plugin)
    before=hashes(protected)
    proc=subprocess.Popen([str(exe),'-multiInst',f'-settingsDir={profile}'],cwd=app)
    pid,hwnd=wait_window(exe);assert text(pid,hwnd,SAMPLE)==SAMPLE;time.sleep(3)
    command(hwnd,'Update Fixture Check');time.sleep(.5);assert (plugin.parent/'fixture-ran.txt').exists()
    close(exe);proc.wait(timeout=20)
    install_payload(archive,app,manifest)
    exe = app/'NoteQu.exe'
    assert hashes(protected)==before
    env=dict(os.environ,NOTEX_TEST_UPDATE_PROFILE=str(profile))
    proc=subprocess.Popen([str(exe),'-multiInst',f'-settingsDir={profile}'],cwd=app,env=env)
    pid,hwnd=wait_window(exe);assert text(pid,hwnd)==SAMPLE
    assert 'Custom Command' in menus(hwnd) and 'Custom Macro' in menus(hwnd)
    command(hwnd,'Check for NoteQu Updates...')
    wait_for(profile,'waiting for normal exit')
    assert proc.poll() is None, 'Updater closed the application without consent.'
    close(exe);proc.wait(timeout=20)
    wait_for(profile,'committed: '+target_version)
    new_pid,new_window=wait_window(exe);assert new_pid!=pid and text(new_pid,new_window)==SAMPLE
    (plugin.parent/'fixture-ran.txt').unlink();command(new_window,'Update Fixture Check');time.sleep(.5)
    assert (plugin.parent/'fixture-ran.txt').exists(), 'Preserved plugin did not execute after the update.'
    assert hashes(protected)==before
    close(exe)
    return {'bootstrap':'1.3.3 -> '+manifest['version'],'unsaved_session_restored':True,'custom_commands_and_macro_preserved':True,'protected_files':len(protected),'plugin_loaded_and_command_executed_before_and_after':True,'application':str(app)}

def macos_test(work,profile,archive,manifest,target_version):
    previous=work/'previous.zip'
    download(BASE+'v1.5.0-macos.5/NoteX-1.5.0-macos-arm64.zip',previous)
    subprocess.run(['ditto','-x','-k',str(previous),str(work/'previous')],check=True)
    old=work/'previous/NoteX.app/Contents/MacOS/NoteX'
    env=dict(os.environ,NOTEX_PROFILE=str(profile),NOTEX_TEST_UPDATE_PROFILE=str(profile))
    subprocess.run([str(old),'--self-test','seed'],env=env,check=True,timeout=60)
    app=work/'NoteQu.app';app.mkdir();install_payload(archive,app,manifest)
    subprocess.run(['codesign','--verify','--strict',str(app)],check=True)
    protected=[]
    for name in ['my-personal-note.txt','plugins/custom-extension.dat','themes/custom-theme.json','unknown-future-setting.json']:
        p=profile/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('Synthetic preserved data: '+name);protected.append(p)
    before=hashes(protected)
    # Prove the separately installed first updater version reads the previous real application's session.
    subprocess.run([str(app/'Contents/MacOS/NoteQu'),'--self-test','restore-legacy'],env=env,check=True,timeout=60)
    proc=subprocess.Popen([str(app/'Contents/MacOS/NoteQu'),'--self-test','update-release'],env=env)
    wait_for(profile,'waiting for normal exit');proc.wait(timeout=30)
    assert proc.returncode==0
    wait_for(profile,'committed: '+target_version)
    subprocess.run(['codesign','--verify','--strict',str(app)],check=True)
    # The restarted app belongs to this unique bundle. Quit it through its ordinary AppKit path.
    subprocess.run(['osascript','-e','tell application "'+str(app)+'" to quit'],check=True,timeout=30)
    time.sleep(2)
    subprocess.run([str(app/'Contents/MacOS/NoteQu'),'--self-test','update-verify'],env=env,check=True,timeout=60)
    assert hashes(protected)==before
    return {'bootstrap':'1.5.0-macos.5 -> '+manifest['version'],'unsaved_session_and_format_restored':True,'custom_preferences_preserved':True,'protected_files':len(protected),'application':str(app)}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--from-version',required=True);p.add_argument('--to-version',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    system='windows' if os.name=='nt' else 'darwin';arch='amd64' if system=='windows' else 'arm64'
    work=Path(tempfile.mkdtemp(prefix='notequ-real-release-e2e-'));profile=work/'test-update-profile';profile.mkdir()
    (profile/'allow-update-test').touch();(profile/'notex-updates.json').write_text('{"disabled":true,"futureSetting":"preserve"}')
    archive,manifest=payload(a.from_version,system,arch,work)
    _,target=payload(a.to_version,system,arch,work)
    result=(windows_test if system=='windows' else macos_test)(work,profile,archive,manifest,a.to_version)
    assert json.loads((profile/'notex-updates.json').read_text())['futureSetting']=='preserve'
    installed=Path(result.pop('application'))
    for name,spec in target['files'].items(): assert hashlib.sha256((installed/name).read_bytes()).hexdigest()==spec['sha256']
    result.update(passed=True,platform=system,architecture=arch,from_version=a.from_version,to_version=a.to_version,
                  source=target['source'],real_public_release=True,version_and_signature_checks_enabled=True,
                  consent_dialogs_automated=True,normal_shutdown=True,new_application_started=True)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result))
    # Keep synthetic evidence and transaction backups until independently reviewed.

if __name__=='__main__':main()
