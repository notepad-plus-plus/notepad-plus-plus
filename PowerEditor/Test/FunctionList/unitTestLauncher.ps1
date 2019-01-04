$languages = @("asm","autoit","bash","batch","c","cpp","cs","ini","inno","java","javascript","nsis","perl","php","powershell","python","ruby","xml")

foreach ($language in $languages) {

	echo $language

	Get-ChildItem .\$language -Filter *unitTest | Foreach-Object {

		..\..\bin\notepad++.exe -export=functionList "-l$language" .\$language\$_ | Out-Null

		$expectedRes = Get-Content .\$language\$_.expected.result
		$generatedRes = Get-Content .\$language\$_.result.json

		if ($generatedRes -eq $expectedRes)
		{
			Remove-Item .\$language\$_.result.json
			echo "OK"
		}
		else
		{
			"$generatedRes"
			exit -1
		}
	}
}