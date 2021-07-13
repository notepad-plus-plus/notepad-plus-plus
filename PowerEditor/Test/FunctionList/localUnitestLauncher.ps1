$testRoot = ".\"
$PowerEditor = "$testRoot\..\..\"

Copy-Item "$PowerEditor\installer\functionList" -Destination "$PowerEditor\bin" -Recurse -Force
Copy-Item "$PowerEditor\installer\filesForTesting\regexGlobalTest.xml" -Destination "$PowerEditor\bin\functionList" -Force
Copy-Item "$PowerEditor\installer\filesForTesting\overrideMap.xml" -Destination "$PowerEditor\bin\functionList" -Force

& ".\unitTestLauncher.ps1"