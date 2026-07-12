# ota_flash.ps1 — flash PanPilot over Wi-Fi (ElegantOTA v3 protocol), so a
# stove-mounted device needs no USB cable. Usage:
#   pio run -e crowpanel35_advance
#   .\tools\ota_flash.ps1 -Ip 192.168.86.95 [-Env crowpanel35_advance]
# The device's OTA rollback still applies: a build that fails to run 8 s
# healthy reverts to the previous image on next boot (hal ota_mark_stable).
param(
    [Parameter(Mandatory = $true)][string]$Ip,
    [string]$Env = "crowpanel35_advance"
)

$bin = ".pio\build\$Env\firmware.bin"
if (-not (Test-Path $bin)) { throw "no $bin - run: pio run -e $Env" }

$md5 = (Get-FileHash $bin -Algorithm MD5).Hash.ToLower()
$size = (Get-Item $bin).Length
Write-Host "firmware: $bin ($size bytes, md5 $md5)"

# 1) announce (mode=fr -> firmware, as the /update page does)
$start = Invoke-RestMethod -Uri "http://$Ip/ota/start?mode=fr&hash=$md5" -TimeoutSec 10
Write-Host "start: $start"

# 2) upload (multipart field name 'file'); curl.exe handles multipart cleanly
$resp = & curl.exe -s -X POST -F "file=@$bin" "http://$Ip/ota/upload"
Write-Host "upload: $resp"
if ($LASTEXITCODE -ne 0) { throw "upload failed" }
Write-Host "device is rebooting into the new image; check http://$Ip/api/health for the new fw version"
