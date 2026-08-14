# After Build: 16-bit additive checksum of Project.bin -> Project.chk (XXXX + CRLF)
$ErrorActionPreference = 'Stop'
try {
    $root = Split-Path -Parent $PSScriptRoot
    $binPath = Join-Path $root 'bin\Project.bin'
    $chkPath = Join-Path $root 'bin\Project.chk'
    if (-not (Test-Path -LiteralPath $binPath)) {
        throw "missing $binPath"
    }
    $bytes = [System.IO.File]::ReadAllBytes($binPath)
    $s = 0
    foreach ($x in $bytes) {
        $s = ($s + $x) -band 0xFFFF
    }
    $h = '{0:X4}' -f $s
    [System.IO.File]::WriteAllText($chkPath, $h + "`r`n")
    [Console]::Out.WriteLine("BIN_SUM16=$h")
    exit 0
}
catch {
    $errPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'bin\Project.chk.err'
    $msg = $_.Exception.Message
    [System.IO.File]::WriteAllText($errPath, $msg)
    [Console]::Error.WriteLine("gen_bin_sum16: $msg")
    exit 1
}
