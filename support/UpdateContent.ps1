$ScriptDir = Split-Path $MyInvocation.MyCommand.Path -Parent
$ScriptDir = Split-Path $ScriptDir -Parent
Push-Location $ScriptDir
if (-not (Test-Path "content"))
{
    git clone --depth 1 "https://github.com/CelestiaProject/CelestiaContent.git" -b master "content"
}
elseif (Test-Path "content" -PathType Container)
{
    Set-Location content
    git fetch --depth 1 origin master
    git reset --hard origin/master
}
else
{
    Write-Error "content exists and is not a directory"
}

Pop-Location
