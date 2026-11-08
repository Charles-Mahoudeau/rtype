# Smoke test of a release zip, on Windows: checks it the way a user would, by unzipping it into an empty directory and
# running it. The same checks as smoke-test.sh.
#
#   .github/scripts/smoke-test.ps1 <aylo-engine-...zip>
#
# Run it on a machine with no toolchain for the answer that counts. Exits 1 if a check fails.
param([Parameter(Mandatory = $true)][string]$Zip)

$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$failures = 0

function Ok($message) { Write-Host "ok:   $message" }
function Warn($message) { Write-Host "warn: $message" }
function Fail($message) {
    [Console]::Error.WriteLine("FAIL: $message")
    $script:failures++
}

if (-not (Test-Path -LiteralPath $Zip -PathType Leaf)) {
    [Console]::Error.WriteLine("usage: smoke-test.ps1 <release zip>")
    exit 2
}
$Zip = (Resolve-Path -LiteralPath $Zip).Path
$work = Join-Path ([System.IO.Path]::GetTempPath()) ("aylo-smoke-" + [System.Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
$processes = @()

try {
    # --- Unzip into an empty directory ------------------------------------------------------------------------------
    $unzipped = Join-Path $work 'unzipped'
    Expand-Archive -LiteralPath $Zip -DestinationPath $unzipped
    $entries = @(Get-ChildItem -LiteralPath $unzipped)
    if ($entries.Count -ne 1 -or -not $entries[0].PSIsContainer) {
        [Console]::Error.WriteLine("FAIL: the zip must hold exactly one folder, it holds $($entries.Count) entries")
        exit 1
    }
    $root = $entries[0].FullName
    Ok "unzipped to one folder: $($entries[0].Name)"

    # --- Layout -------------------------------------------------------------------------------------------------------
    $runtime = Join-Path $root 'runtime\aylo-runtime.exe'
    foreach ($file in 'manifest.json', 'README.md', 'VERSION', 'templates\game\project.luau', 'runtime\licenses\THIRD_PARTY_NOTICES.md',
                      'runtime\aylo-runtime.exe', 'tools\aylo-sign.exe') {
        if (Test-Path -LiteralPath (Join-Path $root $file) -PathType Leaf) { Ok "has $file" } else { Fail "missing $file" }
    }
    foreach ($folder in 'runtime\plugins', 'runtime\plugins\backends', 'templates\game\scripts', 'templates\game\assets') {
        if (Test-Path -LiteralPath (Join-Path $root $folder) -PathType Container) { Ok "has $folder\" } else { Fail "missing $folder\" }
    }

    # --- Version ------------------------------------------------------------------------------------------------------
    if (Test-Path -LiteralPath $runtime) {
        $reported = (& $runtime --version | Select-Object -First 1)
        $expected = "aylo-engine " + (Get-Content -LiteralPath (Join-Path $root 'VERSION') -TotalCount 1).Trim()
        if ($reported -eq $expected) { Ok "--version says '$reported'" } else { Fail "--version says '$reported', VERSION says '$expected'" }
    }

    # --- Manifest: the hash of every file ----------------------------------------------------------------------------
    $manifest = Get-Content -LiteralPath (Join-Path $root 'manifest.json') -Raw | ConvertFrom-Json
    $bad = 0
    foreach ($entry in $manifest.files) {
        $file = Join-Path $root ($entry.path -replace '/', '\')
        $have = if (Test-Path -LiteralPath $file) { (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLower() } else { '' }
        if ($have -ne $entry.sha256) {
            Fail "manifest: $($entry.path) has hash '$have', the manifest says '$($entry.sha256)'"
            $bad++
        }
    }
    $present = @(Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object { $_.Name -ne 'manifest.json' }).Count
    if ($manifest.files.Count -ne $present) {
        Fail "manifest lists $($manifest.files.Count) files, the zip holds $present"
    } elseif ($bad -eq 0) {
        Ok "manifest: $present files match their hash"
    }

    # --- Nothing outside the runtime folder ---------------------------------------------------------------------------
    $check = Join-Path $here 'check-self-contained.ps1'
    if (Test-Path -LiteralPath $check) {
        & $check (Join-Path $root 'runtime')
        $runtimeStatus = $LASTEXITCODE
        & $check (Join-Path $root 'tools')
        if ($runtimeStatus -eq 0 -and $LASTEXITCODE -eq 0) {
            Ok 'runtime\ and tools\ depend on nothing outside themselves'
        } else {
            Fail 'something resolves outside the zip (listed above)'
        }
    } else {
        Warn "$check does not exist: the dependencies of the runtime are not checked"
    }

    # --- A server and a headless client ---------------------------------------------------------------------------------
    # The output of a Windows program redirected to a file is buffered by its C runtime: a player joining is not seen
    # until the server stops. Both programs are only checked to stay up.
    Warn 'the output of the server is buffered on Windows: the client connecting is not checked, only that both stay up'
    if (Test-Path -LiteralPath $runtime) {
        $port = 20000 + (Get-Random -Maximum 20000)
        $game = Join-Path $root 'templates\game'
        $serverLog = Join-Path $work 'server.log'
        $clientLog = Join-Path $work 'client.log'
        $server = Start-Process -FilePath $runtime -ArgumentList '--server', "`"$game`"", '--port', $port `
            -RedirectStandardOutput $serverLog -RedirectStandardError (Join-Path $work 'server.err') -PassThru -WindowStyle Hidden
        $processes += $server
        Start-Sleep -Seconds 2
        if (-not $server.HasExited) {
            Ok 'the server on templates\game is running'
            $client = Start-Process -FilePath $runtime -ArgumentList "`"$game`"", '--port', $port, '--headless' `
                -RedirectStandardOutput $clientLog -RedirectStandardError (Join-Path $work 'client.err') -PassThru -WindowStyle Hidden
            $processes += $client
            Start-Sleep -Seconds 4
            if ($client.HasExited) {
                Fail "the headless client exited with code $($client.ExitCode): $(Get-Content -Raw -LiteralPath (Join-Path $work 'client.err'))"
            } else {
                Ok 'the headless client is running'
            }
            if ($server.HasExited) { Fail "the server stopped with code $($server.ExitCode)" } else { Ok 'the server is still running' }
        } else {
            Fail "the server exited with code $($server.ExitCode): $(Get-Content -Raw -LiteralPath (Join-Path $work 'server.err'))"
        }
    }

    # --- aylo-sign ----------------------------------------------------------------------------------------------------
    $keys = Join-Path $work 'keys'
    New-Item -ItemType Directory -Path $keys | Out-Null
    Push-Location $keys
    try { & (Join-Path $root 'tools\aylo-sign.exe') keygen smoke | Out-Null } finally { Pop-Location }
    if ((Test-Path (Join-Path $keys 'smoke.pub')) -and (Test-Path (Join-Path $keys 'smoke.key'))) {
        Ok 'aylo-sign keygen makes a key pair'
    } else {
        Fail 'aylo-sign keygen did not make smoke.pub and smoke.key'
    }
} finally {
    foreach ($process in $processes) {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
    }
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ''
if ($failures -eq 0) {
    Write-Host "smoke test passed: $(Split-Path -Leaf $Zip)"
    exit 0
}
[Console]::Error.WriteLine("smoke test FAILED: $failures checks")
exit 1
