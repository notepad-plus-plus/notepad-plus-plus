$languages = @("asm","autoit","bash","batch","c","cpp","cs","ini","inno","java","javascript","nsis","perl","php","powershell","python","ruby","xml")

foreach ($language in $languages) {

	echo $language

	Get-ChildItem .\$language\ -exclude *result* | Foreach-Object {

		..\..\bin\notepad++.exe -export=functionList "-l$language" $_ | Out-Null

		$expectedRes = Get-Content "$_.expected.result"
		$generatedRes = Get-Content "$_.result.json"

		if ($generatedRes -eq $expectedRes)
		{
			Remove-Item "$_.result.json"
			echo "OK"
		}
		else
		{
			"$generatedRes"
			exit -1
		}
	}
}