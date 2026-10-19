# test_enlngf_http_methods.ps1
$ErrorActionPreference = "Stop"

$testFile = Join-Path $PSScriptRoot "test_style2_and_html.enlgf"
$enlngfExe = Join-Path $PSScriptRoot "..\enlngf.exe"
$port = 38922

Write-Host "Starting enlngf serve on port $port..."
$proc = Start-Process -FilePath $enlngfExe -ArgumentList "serve `"$testFile`" --port $port" -PassThru
Start-Sleep -Milliseconds 700

try {
    # 1. Test POST -> 405 Method Not Allowed
    Write-Host "Testing POST..."
    $rawPost = curl.exe -s -i -X POST "http://127.0.0.1:$port/"
    $strPost = $rawPost -join "`n"
    if ($strPost -notmatch "HTTP/1\.1 405 Method Not Allowed") {
        throw "[FAIL] POST did not return HTTP/1.1 405 Method Not Allowed"
    }
    if ($strPost -notmatch "Allow: GET, HEAD") {
        throw "[FAIL] POST did not return Allow: GET, HEAD"
    }
    if ($strPost -notmatch "Content-Length: 22") {
        throw "[FAIL] POST did not return Content-Length: 22"
    }
    if ($strPost -notmatch "405 Method Not Allowed") {
        throw "[FAIL] POST did not return body '405 Method Not Allowed'"
    }
    Write-Host "[PASS] POST returned 405 Method Not Allowed with Allow: GET, HEAD"

    # 2. Test PUT -> 405 Method Not Allowed
    Write-Host "Testing PUT..."
    $rawPut = curl.exe -s -i -X PUT "http://127.0.0.1:$port/"
    $strPut = $rawPut -join "`n"
    if ($strPut -notmatch "HTTP/1\.1 405 Method Not Allowed") {
        throw "[FAIL] PUT did not return HTTP/1.1 405 Method Not Allowed"
    }
    Write-Host "[PASS] PUT returned 405 Method Not Allowed"

    # 3. Test OPTIONS -> 405 Method Not Allowed
    Write-Host "Testing OPTIONS..."
    $rawOpt = curl.exe -s -i -X OPTIONS "http://127.0.0.1:$port/"
    $strOpt = $rawOpt -join "`n"
    if ($strOpt -notmatch "HTTP/1\.1 405 Method Not Allowed") {
        throw "[FAIL] OPTIONS did not return HTTP/1.1 405 Method Not Allowed"
    }
    Write-Host "[PASS] OPTIONS returned 405 Method Not Allowed"

    # 4. Test HEAD -> 200 OK without body bytes
    Write-Host "Testing HEAD..."
    $rawHead = curl.exe -s -i -I "http://127.0.0.1:$port/"
    $strHead = $rawHead -join "`n"
    if ($strHead -notmatch "HTTP/1\.1 200 OK") {
        throw "[FAIL] HEAD did not return HTTP/1.1 200 OK"
    }
    if ($strHead -match "<!DOCTYPE html>") {
        throw "[FAIL] HEAD returned HTML body bytes!"
    }
    Write-Host "[PASS] HEAD returned 200 OK without body bytes"

    # 5. Test GET -> 200 OK with body
    Write-Host "Testing GET..."
    $rawGet = curl.exe -s -i "http://127.0.0.1:$port/"
    $strGet = $rawGet -join "`n"
    if ($strGet -notmatch "HTTP/1\.1 200 OK") {
        throw "[FAIL] GET did not return HTTP/1.1 200 OK"
    }
    if ($strGet -notmatch "<!-- Compiled by Enlangg Sovereign Web Engine -->") {
        throw "[FAIL] GET body did not contain sovereign compile comment"
    }
    Write-Host "[PASS] GET returned 200 OK with full body and sovereign comment"

    Write-Host "`nAll HTTP method hardening tests passed successfully!"
} finally {
    Stop-Process -Id $proc.Id -Force
}
