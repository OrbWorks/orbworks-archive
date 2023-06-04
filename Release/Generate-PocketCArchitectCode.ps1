param([string]$username)

$code = [uint]490155520
foreach ($c in $username.ToCharArray())
{
    # PowerShell promotes the multiplication to a 64-bit value, so mask the value to only the lower 32-bits
    $code = [uint32](($code * [byte]$c) % 0x100000000)
    $code %= 1000000000
}

echo $code