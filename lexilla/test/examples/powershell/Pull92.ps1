<# Tests for PowerShell #>

<# Backticks should escape in double quoted strings #>
$double_quote_str_esc_1 = "`"XXX`""
$double_quote_str_esc_2 = "This `"string`" `$useses `r`n Backticks '``'"

<# Backticks should be ignored in quoted strings #>
$single_quote_str_esc_1 = 'XXX`'
$single_quote_str_esc_2 = 'XXX```'
