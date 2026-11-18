try {
	$binDir = '..\..\Bin'
	$pluginsDir = $binDir + '\plugins'
	$pluginsSaveDir = $binDir + '\plugins_save'

	if (Test-Path -Path $pluginsDir -PathType Container)
	{
		if (Test-Path -Path $pluginsSaveDir -PathType Container)
		{
			"Backup for plugins directory already exists"
			exit -1
		}
		"Backing up plugin directory ..."
		Move-Item $pluginsDir $pluginsSaveDir
	}
	"Installing Lua plugin for testing ..."
	Copy-Item -Path .\plugins -Destination $binDir -Recurse

	"Testing ..."
	Invoke-Expression ($binDir + "\notepad++.exe | Out-Null")

	if (Test-Path -Path $pluginsSaveDir -PathType Container)
	{
		"Removing Lua plugin ..."
		Remove-Item -Path $pluginsDir -Recurse -Force
		"Restoring plugin directory ..."
		Move-Item $pluginsSaveDir $pluginsDir
	}

	$expectedRes = Get-Content .\verifyUrlDetection_1a.expected.result
	$generatedRes = Get-Content .\verifyUrlDetection_1a.result

	if (Compare-Object -ReferenceObject $expectedRes -DifferenceObject $generatedRes)
	{
		"Unexpected test results for verifyUrlDetection_1a"
		exit -1
	}
	else
	{
		Remove-Item .\verifyUrlDetection_1a.result
		$expectedRes = Get-Content .\verifyUrlDetection_1b.expected.result
		$generatedRes = Get-Content .\verifyUrlDetection_1b.result
		if (Compare-Object -ReferenceObject $expectedRes -DifferenceObject $generatedRes)
		{
			"Unexpected test results for verifyUrlDetection_1b"
			exit -1
		}
		else
		{
			Remove-Item .\verifyUrlDetection_1b.result
			"URL detection test OK"
			exit 0
		}
	}
}
catch
{
	"Unexpected behavior while URL detection test"
	exit -1
}

