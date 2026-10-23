param([string]$InstallationRoot = '', [switch]$CheckOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-Version([string]$Value) {
    if ($Value -cnotmatch '^minotaur-[A-Za-z0-9](?:[A-Za-z0-9.-]*[A-Za-z0-9])?$') {
        throw 'Invalid Minotaur version name.'
    }
}

function Assert-Relative([string]$Value) {
    if ([string]::IsNullOrWhiteSpace($Value) -or $Value.Contains('\') -or $Value.Contains(':') -or
        $Value.Contains([char]0) -or [IO.Path]::IsPathRooted($Value)) {
        throw 'Unsafe update path.'
    }
    foreach ($part in $Value.Split('/')) {
        if ($part -eq '' -or $part -eq '.' -or $part -eq '..' -or $part.EndsWith('.') -or
            $part.EndsWith(' ') -or $part -match '^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)') {
            throw 'Unsafe update path component.'
        }
    }
}

function Safe-Path([string]$Root, [string]$Relative) {
    Assert-Relative $Relative
    $rootPath = [IO.Path]::GetFullPath($Root)
    $prefix = $rootPath.TrimEnd([char[]]'\/') + [IO.Path]::DirectorySeparatorChar
    $path = [IO.Path]::GetFullPath((Join-Path $rootPath $Relative))
    if (-not $path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Path leaves installation.' }
    $check = $path
    while ($check) {
        if (Test-Path -LiteralPath $check) {
            $item = Get-Item -LiteralPath $check -Force
            if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Linked update paths are unsupported.' }
        }
        if ($check.TrimEnd([char[]]'\/') -ieq $rootPath.TrimEnd([char[]]'\/')) { break }
        $check = [IO.Path]::GetDirectoryName($check)
    }
    return $path
}

function Load-Json([string]$Path) {
    return Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
}

function New-Map {
    return ,([Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal))
}

function Assert-Record($Record, [bool]$Pack) {
    if ($Record.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
        (-not ($Record.size -is [int] -or $Record.size -is [long])) -or $Record.size -lt 0) {
        throw 'Invalid file digest or size.'
    }
    if ($Pack -and ($Record.size -eq 0 -or $Record.content_id -cnotmatch '^[0-9a-f]{64}$' -or
        $Record.file -cnotmatch '^[A-Za-z0-9][A-Za-z0-9._-]*\.zip$')) { throw 'Invalid content pack.' }
}

function Assert-Index($Index) {
    if ($Index.schema -ne 1 -or $Index.profile -cne 'desktop' -or $Index.content_format -ne 1) {
        throw 'Unsupported content index.'
    }
    $map = New-Map
    $names = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $packages = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($pack in @($Index.packs)) {
        Assert-Record $pack $true
        Assert-Relative $pack.file
        Assert-Relative $pack.id
        if (@('engine','mm6','mm7','mm8') -cnotcontains $pack.package) { throw 'Unknown world package.' }
        $mount = '/worlds/' + $pack.package
        if ($pack.package -ceq 'engine') { $mount = '/engine' }
        if ($pack.mount -cne $mount -and -not $pack.mount.StartsWith($mount + '/', [StringComparison]::Ordinal)) {
            throw 'Invalid content mount.'
        }
        Assert-Relative $pack.mount.TrimStart('/')
        if ($map.ContainsKey($pack.id) -or -not $names.Add($pack.file)) { throw 'Duplicate content pack.' }
        $map.Add($pack.id, $pack)
        [void]$packages.Add($pack.package)
    }
    if ($packages.Count -ne 4) { throw 'Missing engine or MM6/MM7/MM8 content.' }
    return ,$map
}

function Same-Content($Left, $Right) {
    foreach ($key in @('id','package','mount','content_id')) {
        if ($Left.$key -cne $Right.$key) { return $false }
    }
    return $true
}

function Assert-SameIndex($Expected, $Actual) {
    $expectedPacks = Assert-Index $Expected
    $actualPacks = Assert-Index $Actual
    if ($expectedPacks.Count -ne $actualPacks.Count) { throw 'Installed resource set differs from patch base.' }
    foreach ($id in $expectedPacks.Keys) {
        if (-not $actualPacks.ContainsKey($id) -or -not (Same-Content $expectedPacks[$id] $actualPacks[$id])) {
            throw 'Installed resource set differs from patch base.'
        }
    }
}

function File-Matches([string]$Path, $Record) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    if ((Get-Item -LiteralPath $Path).Length -ne $Record.size) { return $false }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ceq $Record.sha256
}

function Assert-File([string]$Path, $Record) {
    if (-not (File-Matches $Path $Record)) { throw ('Missing or damaged file: ' + $Path) }
}

function Atomic-Text([string]$Path, [string]$Text) {
    $temporary = $Path + '.new-' + [guid]::NewGuid().ToString('N')
    try {
        [IO.File]::WriteAllText($temporary, $Text, [Text.UTF8Encoding]::new($false))
        if (Test-Path -LiteralPath $Path) { [IO.File]::Replace($temporary, $Path, $null) }
        else { [IO.File]::Move($temporary, $Path) }
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
    }
}

function Assert-App([string]$Folder, $Files, $Index) {
    foreach ($file in $Files) { Assert-File (Safe-Path $Folder $file.path) $file }
    $allowed = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in $Files) { [void]$allowed.Add($file.path) }
    foreach ($name in @('index.json','healthy','starts')) { [void]$allowed.Add($name) }
    $prefix = [IO.Path]::GetFullPath($Folder).TrimEnd([char[]]'\/') + [IO.Path]::DirectorySeparatorChar
    foreach ($item in Get-ChildItem -LiteralPath $Folder -Recurse -File -Force) {
        $name = $item.FullName.Substring($prefix.Length).Replace('\','/')
        if (-not $allowed.Contains($name)) { throw 'Unexpected file in the new application directory.' }
        [void](Safe-Path $Folder $name)
    }
    Assert-SameIndex $Index (Load-Json (Safe-Path $Folder 'index.json'))
}

$lock = $null
$staging = $null
try {
    $patch = Load-Json (Join-Path $PSScriptRoot 'minotaur-update.json')
    if ($patch.schema -ne 1 -or $patch.channel -cne 'minotaur') { throw 'Not a Minotaur update.' }
    foreach ($release in @($patch.base, $patch.target)) {
        Assert-Version $release.version
        if ($release.schema -ne 1 -or $release.channel -cne 'minotaur' -or
            $release.source_sha -cnotmatch '^[0-9a-f]{40}$') { throw 'Invalid Minotaur release metadata.' }
        [void](Assert-Index $release.index)
    }
    if ($patch.base.version -ieq $patch.target.version) { throw 'Patch must use a new version directory.' }
    $appNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($file in @($patch.app_files)) {
        Assert-Relative $file.path
        Assert-Record $file $false
        if (@('index.json','healthy','starts') -contains $file.path -or -not $appNames.Add($file.path)) {
            throw 'Duplicate or reserved app payload path.'
        }
    }
    if (-not $appNames.Contains('openyamm.exe') -or -not $appNames.Contains('OpenYAMM-launcher.exe')) {
        throw 'Patch lacks the game or launcher.'
    }
    if (-not $InstallationRoot) {
        $InstallationRoot = Split-Path -Parent $PSScriptRoot
        if (-not (Test-Path -LiteralPath (Join-Path $InstallationRoot 'app/current') -PathType Leaf)) {
            Add-Type -AssemblyName System.Windows.Forms
            $picker = New-Object Windows.Forms.FolderBrowserDialog
            $picker.Description = 'Выберите папку игры с OpenYAMM.exe'
            if ($picker.ShowDialog() -ne [Windows.Forms.DialogResult]::OK) { throw 'Установка отменена.' }
            $InstallationRoot = $picker.SelectedPath
        }
    }
    $InstallationRoot = [IO.Path]::GetFullPath($InstallationRoot)
    $currentPath = Safe-Path $InstallationRoot 'app/current'
    if (-not (Test-Path -LiteralPath $currentPath -PathType Leaf)) { throw 'Установите полную базовую сборку мода.' }
    $current = (Get-Content -LiteralPath $currentPath -Raw -Encoding UTF8).Trim()
    Assert-Version $current
    $installed = Load-Json (Safe-Path $InstallationRoot ('app/' + $current + '/index.json'))
    $installedPacks = Assert-Index $installed
    if ($installed.store -cne '../../assets/packs') { throw 'Unsupported content-store layout.' }
    $targetPath = Safe-Path $InstallationRoot ('app/' + $patch.target.version)
    if ($current -ceq $patch.target.version) {
        Assert-App $targetPath $patch.app_files $patch.target.index
        foreach ($pack in @($installed.packs)) {
            Assert-File (Safe-Path $InstallationRoot ('assets/packs/' + $pack.file)) $pack
        }
        Write-Host 'Это обновление уже установлено.'
        exit 0
    }
    if ($current -cne $patch.base.version) {
        throw ('Патч предназначен для ' + $patch.base.version + '. Установлена ' + $current + '.')
    }
    Assert-SameIndex $patch.base.index $installed
    foreach ($process in @(Get-Process -Name openyamm -ErrorAction SilentlyContinue)) {
        if (-not $process.Path -or $process.Path.StartsWith(
            $InstallationRoot.TrimEnd([char[]]'\/') + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) { throw 'Закройте игру перед обновлением.' }
    }
    $basePacks = Assert-Index $patch.base.index
    $targetPacks = Assert-Index $patch.target.index
    $reused = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    $changed = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
    foreach ($id in @($patch.reused_packs)) {
        if (-not $reused.Add($id) -or -not $targetPacks.ContainsKey($id)) { throw 'Invalid reused-pack list.' }
    }
    foreach ($id in @($patch.changed_packs)) {
        if (-not $changed.Add($id) -or $reused.Contains($id) -or -not $targetPacks.ContainsKey($id)) {
            throw 'Invalid changed-pack list.'
        }
    }
    if ($reused.Count + $changed.Count -ne $targetPacks.Count) { throw 'Incomplete patch pack list.' }
    $resolved = ($patch.target.index | ConvertTo-Json -Depth 32 | ConvertFrom-Json)
    $resolved.store = '../../assets/packs'
    $number = 0
    foreach ($pack in @($resolved.packs)) {
        $number++
        Write-Progress -Activity 'Проверка ресурсов' -Status ('{0} из {1}' -f $number,$targetPacks.Count) -PercentComplete (
            [int](100 * $number / $targetPacks.Count))
        $same = $basePacks.ContainsKey($pack.id) -and (Same-Content $basePacks[$pack.id] $pack)
        if ($reused.Contains($pack.id)) {
            if (-not $same) { throw 'Reused pack differs from the base.' }
            $old = $installedPacks[$pack.id]
            Assert-File (Safe-Path $InstallationRoot ('assets/packs/' + $old.file)) $old
            $pack.file = $old.file
            $pack.size = $old.size
            $pack.sha256 = $old.sha256
        } else {
            if ($same) { throw 'Unchanged logical content must be reused.' }
            Assert-File (Safe-Path $PSScriptRoot ('payload/packs/' + $pack.file)) $pack
            $destination = Safe-Path $InstallationRoot ('assets/packs/' + $pack.file)
            if ((Test-Path -LiteralPath $destination) -and -not (File-Matches $destination $pack)) {
                $pack.file = [IO.Path]::GetFileNameWithoutExtension($pack.file) + '-' +
                    $pack.sha256.Substring(0,16) + '.zip'
                $destination = Safe-Path $InstallationRoot ('assets/packs/' + $pack.file)
                if (Test-Path -LiteralPath $destination) { Assert-File $destination $pack }
            }
        }
    }
    Write-Progress -Activity 'Проверка ресурсов' -Completed
    foreach ($file in @($patch.app_files)) {
        Assert-File (Safe-Path $PSScriptRoot ('payload/app/' + $file.path)) $file
    }
    if ($CheckOnly) { Write-Host 'Патч и установленная база проверены. Файлы игры не изменены.'; exit 0 }
    $updatePath = Safe-Path $InstallationRoot 'update'
    [void][IO.Directory]::CreateDirectory($updatePath)
    $lock = [IO.File]::Open((Join-Path $updatePath 'minotaur.lock'), [IO.FileMode]::OpenOrCreate,
        [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    if ((Get-Content -LiteralPath $currentPath -Raw -Encoding UTF8).Trim() -cne $current) {
        throw 'Версия игры изменилась. Запустите патч повторно.'
    }
    $staging = Join-Path $updatePath ('minotaur-' + [guid]::NewGuid().ToString('N'))
    $stagedApp = Join-Path $staging 'app'
    [void][IO.Directory]::CreateDirectory($stagedApp)
    Write-Host 'Установка обновления...'
    foreach ($file in @($patch.app_files)) {
        $destination = Safe-Path $stagedApp $file.path
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
        Copy-Item -LiteralPath (Safe-Path $PSScriptRoot ('payload/app/' + $file.path)) -Destination $destination
        Assert-File $destination $file
    }
    $indexPath = Safe-Path $stagedApp 'index.json'
    [IO.File]::WriteAllText($indexPath, ($resolved | ConvertTo-Json -Depth 32), [Text.UTF8Encoding]::new($false))
    foreach ($pack in @($resolved.packs)) {
        if (-not $changed.Contains($pack.id)) { continue }
        $destination = Safe-Path $InstallationRoot ('assets/packs/' + $pack.file)
        if (Test-Path -LiteralPath $destination) { Assert-File $destination $pack; continue }
        [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destination))
        $temporary = Join-Path $staging 'pack.tmp'
        $original = $targetPacks[$pack.id]
        Copy-Item -LiteralPath (Safe-Path $PSScriptRoot ('payload/packs/' + $original.file)) -Destination $temporary
        Assert-File $temporary $pack
        [IO.File]::Move($temporary, $destination)
    }
    if (Test-Path -LiteralPath $targetPath) {
        Assert-App $targetPath $patch.app_files $resolved
        $resolved = Load-Json (Safe-Path $targetPath 'index.json')
    } else { [IO.Directory]::Move($stagedApp, $targetPath) }
    foreach ($pack in @($resolved.packs)) {
        Assert-File (Safe-Path $InstallationRoot ('assets/packs/' + $pack.file)) $pack
    }
    foreach ($name in @('starts','healthy')) {
        $marker = Safe-Path $targetPath $name
        if (Test-Path -LiteralPath $marker) { Remove-Item -LiteralPath $marker -Force }
    }
    Atomic-Text (Safe-Path $InstallationRoot 'app/previous') ($current + "`n")
    Atomic-Text $currentPath ($patch.target.version + "`n")
    try {
        Atomic-Text (Safe-Path $InstallationRoot 'BUILD-REVISION.txt') (
            'Commit SHA: ' + $patch.target.source_sha + "`nVersion: " + $patch.target.version +
            "`nPrevious version: " + $current + "`n")
    } catch { Write-Warning 'Версия переключена. Сведения о версии находятся в app/current.' }
    Write-Host ('Готово: ' + $patch.target.version + '. Запускайте обычный OpenYAMM.exe.')
} catch {
    Write-Host ('Обновление не выполнено: ' + $_.Exception.Message) -ForegroundColor Red
    exit 1
} finally {
    if ($lock) { $lock.Dispose() }
    if ($staging -and (Test-Path -LiteralPath $staging)) {
        Remove-Item -LiteralPath $staging -Recurse -Force -ErrorAction SilentlyContinue
    }
}
