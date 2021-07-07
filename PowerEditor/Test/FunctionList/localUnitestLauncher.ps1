$testRoot = ".\"
$PowerEditor = "$testRoot\..\..\"

Copy-Item "$PowerEditor\installer\functionList" -Destination "$PowerEditor\bin" -Recurse
Copy-Item "$PowerEditor\installer\filesForTesting\regexGlobalTest.xml" -Destination "$PowerEditor\bin\functionList"
Copy-Item "$PowerEditor\installer\filesForTesting\overrideMap.xml" -Destination "$PowerEditor\bin\functionList"

& ".\unitTestLauncher.ps1"