<#
.SYNOPSIS
  Checks that a folder runs without anything installed besides Windows and the GPU driver.

.DESCRIPTION
  Every executable and DLL in the folder (PE files) must find the libraries it imports inside the folder, or among
  the ones Windows provides. The C++ runtime (vcruntime140*.dll, msvcp140*.dll) is not one of them: it is shipped in
  the folder. The Vulkan loader (vulkan-1.dll) is installed with the GPU driver and is allowed.

  It reads the import tables of the files itself, so it needs no Visual Studio tool (dumpbin).

  Exits with 0 when the folder is self-contained, 1 when it is not (one offender per line on the error output), 2 on
  a usage error. The Linux and macOS counterpart is check-self-contained.sh, with the same output.

.PARAMETER Folder
  The folder to check.

.EXAMPLE
  .github/scripts/check-self-contained.ps1 build/package/runtime
#>
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Folder
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Folder -PathType Container)) {
    [Console]::Error.WriteLine("Usage: check-self-contained.ps1 <folder>")
    exit 2
}

$root = (Resolve-Path -LiteralPath $Folder).Path.TrimEnd('\')

# DLLs that are part of Windows or its drivers, by name. The C++ runtime is not in this list on purpose.
$allowPatterns = @(
    'api-ms-win-*.dll', 'ext-ms-win-*.dll',
    'kernel32.dll', 'kernelbase.dll', 'ntdll.dll', 'user32.dll', 'gdi32.dll', 'gdi32full.dll', 'advapi32.dll',
    'shell32.dll', 'shlwapi.dll', 'ole32.dll', 'oleaut32.dll', 'combase.dll', 'comdlg32.dll', 'comctl32.dll',
    'ws2_32.dll', 'wsock32.dll', 'mswsock.dll', 'iphlpapi.dll', 'bcrypt.dll', 'crypt32.dll', 'secur32.dll',
    'winmm.dll', 'imm32.dll', 'version.dll', 'setupapi.dll', 'cfgmgr32.dll', 'shcore.dll', 'dwmapi.dll',
    'dbghelp.dll', 'psapi.dll', 'userenv.dll', 'rpcrt4.dll', 'sechost.dll', 'msvcrt.dll', 'ucrtbase.dll',
    'opengl32.dll', 'hid.dll', 'xinput1_4.dll', 'xinput9_1_0.dll', 'dxgi.dll', 'd3d11.dll', 'd3d12.dll',
    'mmdevapi.dll', 'avrt.dll', 'uxtheme.dll', 'wtsapi32.dll', 'powrprof.dll', 'netapi32.dll', 'ncrypt.dll',
    # The Vulkan loader comes with the GPU driver.
    'vulkan-1.dll'
)
if ($env:SELF_CONTAINED_ALLOW) {
    $allowPatterns += ($env:SELF_CONTAINED_ALLOW -split '\s+' | Where-Object { $_ })
}

$script:offenders = 0
function Report([string]$message) {
    [Console]::Error.WriteLine($message)
    $script:offenders++
}

function Get-Relative([string]$path) { $path.Substring($root.Length).TrimStart('\') }

# The DLL names a PE file imports (normal and delay-load tables), or $null when the file is not a PE file.
function Get-PeImports([string]$path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) { return $null }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    if ($peOffset -le 0 -or $peOffset + 24 -gt $bytes.Length) { return $null }
    if ([BitConverter]::ToUInt32($bytes, $peOffset) -ne 0x00004550) { return $null }

    $sectionCount = [BitConverter]::ToUInt16($bytes, $peOffset + 6)
    $optionalSize = [BitConverter]::ToUInt16($bytes, $peOffset + 20)
    $optional = $peOffset + 24
    $magic = [BitConverter]::ToUInt16($bytes, $optional)
    # Data directories start at 96 (PE32) or 112 (PE32+) bytes into the optional header.
    $directories = if ($magic -eq 0x20B) { $optional + 112 } else { $optional + 96 }
    $sections = $optional + $optionalSize

    function ConvertTo-Offset([uint32]$rva) {
        for ($i = 0; $i -lt $sectionCount; $i++) {
            $s = $sections + 40 * $i
            $virtualSize = [BitConverter]::ToUInt32($bytes, $s + 8)
            $virtualAddress = [BitConverter]::ToUInt32($bytes, $s + 12)
            $rawSize = [BitConverter]::ToUInt32($bytes, $s + 16)
            $rawPointer = [BitConverter]::ToUInt32($bytes, $s + 20)
            $span = [Math]::Max($virtualSize, $rawSize)
            if ($rva -ge $virtualAddress -and $rva -lt $virtualAddress + $span) {
                return [int]($rva - $virtualAddress + $rawPointer)
            }
        }
        return -1
    }
    function Read-Name([int]$offset) {
        $end = $offset
        while ($end -lt $bytes.Length -and $bytes[$end] -ne 0) { $end++ }
        [System.Text.Encoding]::ASCII.GetString($bytes, $offset, $end - $offset)
    }

    $names = @()
    # Directory 1: imports (20-byte descriptors, name RVA at +12). Directory 13: delay imports (32 bytes, name at +4).
    foreach ($table in @(@{ Index = 1; Size = 20; Name = 12 }, @{ Index = 13; Size = 32; Name = 4 })) {
        $entry = $directories + 8 * $table.Index
        if ($entry + 8 -gt $bytes.Length) { continue }
        $rva = [BitConverter]::ToUInt32($bytes, $entry)
        if ($rva -eq 0) { continue }
        $offset = ConvertTo-Offset $rva
        while ($offset -ge 0 -and $offset + $table.Size -le $bytes.Length) {
            $nameRva = [BitConverter]::ToUInt32($bytes, $offset + $table.Name)
            if ($nameRva -eq 0) { break }
            $nameOffset = ConvertTo-Offset $nameRva
            if ($nameOffset -lt 0) { break }
            $names += Read-Name $nameOffset
            $offset += $table.Size
        }
    }
    return , $names
}

# Every file of the folder by name, for the lookup of an import.
$files = Get-ChildItem -LiteralPath $root -Recurse -File -Force
$inFolder = @{}
foreach ($file in $files) { $inFolder[$file.Name.ToLowerInvariant()] = $true }

# A link that leaves the folder.
foreach ($item in Get-ChildItem -LiteralPath $root -Recurse -Force | Where-Object { $_.LinkType }) {
    $target = ($item.Target | Select-Object -First 1)
    if ($target -and -not ([System.IO.Path]::GetFullPath($target)).StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
        Report "$(Get-Relative $item.FullName): link to $target, outside the folder"
    }
}

$count = 0
foreach ($file in $files) {
    if ($file.Extension -notin '.exe', '.dll', '.pyd', '.node') { continue }
    $imports = Get-PeImports $file.FullName
    if ($null -eq $imports) { continue }
    $count++
    foreach ($name in $imports) {
        $lower = $name.ToLowerInvariant()
        if ($inFolder.ContainsKey($lower)) { continue }
        $allowed = $false
        foreach ($pattern in $allowPatterns) {
            if ($lower -like $pattern.ToLowerInvariant()) { $allowed = $true; break }
        }
        if (-not $allowed) {
            Report "$(Get-Relative $file.FullName): imports $name, which is neither provided by Windows nor in the folder"
        }
    }
}

if ($script:offenders -gt 0) {
    [Console]::Error.WriteLine("$($script:offenders) offender(s) in $count binaries of $root.")
    exit 1
}
Write-Output "Self-contained: $count binaries of $root need nothing outside it."
# Explicit: a script that ends without `exit` leaves $LASTEXITCODE as the last native command set it, which the caller
# (smoke-test.ps1) would read as this check's result.
exit 0
