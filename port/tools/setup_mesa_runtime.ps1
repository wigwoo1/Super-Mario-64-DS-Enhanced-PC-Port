param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repoRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$cache=Join-Path $repoRoot 'build\mesa-runtime'
if(-not $OutputDirectory){$OutputDirectory=Join-Path $repoRoot 'build\port\renderers\mesa'}
New-Item -ItemType Directory -Path $cache,$OutputDirectory -Force | Out-Null
function Fetch-Checked([string]$Url,[string]$Name,[string]$Hash) {
    $path=Join-Path $cache $Name
    if(-not(Test-Path -LiteralPath $path)){Invoke-WebRequest $Url -OutFile $path}
    if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $Hash){throw "Checksum mismatch for $Name"}
    return $path
}
$runtime=Fetch-Checked 'https://github.com/pal1000/mesa-dist-win/releases/download/26.2.4/mesa3d-26.2.4-release-msvc.7z' 'mesa3d-26.2.4-release-msvc.7z' '351FC8C8B695878FFB3EAA044B3EAD08672A48B1A045E3C3E3975811DF0F6695'
$source=Fetch-Checked 'https://archive.mesa3d.org/mesa-26.2.4.tar.xz' 'mesa-26.2.4.tar.xz' 'BCE5F7FBEBB934373B86C999A064D52FB5065878DC57F287F95346648EC832E9'
& tar -xf $runtime -C $cache x86/opengl32.dll x86/libgallium_wgl.dll
if($LASTEXITCODE -ne 0){throw 'Mesa runtime extraction failed'}
& tar -xf $source -C $cache mesa-26.2.4/licenses mesa-26.2.4/docs/license.rst
if($LASTEXITCODE -ne 0){throw 'Mesa notices extraction failed'}
$llvm=Join-Path $cache 'LLVM-LICENSE.txt'
$distro=Join-Path $cache 'mesa-dist-win-LICENSE.txt'
if(-not(Test-Path -LiteralPath $llvm)){Invoke-WebRequest 'https://raw.githubusercontent.com/llvm/llvm-project/llvmorg-23.1.2/LICENSE.TXT' -OutFile $llvm}
if(-not(Test-Path -LiteralPath $distro)){Invoke-WebRequest 'https://raw.githubusercontent.com/pal1000/mesa-dist-win/f02a557/LICENSE' -OutFile $distro}
Copy-Item -LiteralPath (Join-Path $cache 'x86\opengl32.dll'),(Join-Path $cache 'x86\libgallium_wgl.dll') -Destination $OutputDirectory -Force
$notices=Join-Path $OutputDirectory 'licenses'
New-Item -ItemType Directory -Path $notices -Force | Out-Null
$sourceNotices=Join-Path $cache 'mesa-26.2.4\licenses'
foreach($file in Get-ChildItem -LiteralPath $sourceNotices -Recurse -File) {
    $relative=$file.FullName.Substring($sourceNotices.Length).TrimStart('\')
    $target=Join-Path (Join-Path $notices 'Mesa') $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force
}
Copy-Item -LiteralPath (Join-Path $cache 'mesa-26.2.4\docs\license.rst'),$llvm,$distro -Destination $notices -Force
@'
Optional CPU OpenGL runtime: Mesa 26.2.4, x86 MSVC, LLVM 23.1.2.
Binary distributor: https://github.com/pal1000/mesa-dist-win/releases/tag/26.2.4
Mesa source: https://archive.mesa3d.org/mesa-26.2.4.tar.xz
LLVM source/license: https://github.com/llvm/llvm-project/tree/llvmorg-23.1.2
License texts and component notices are in licenses/.
The original Mesa source archive, including per-file copyrights, is retained by
the development setup in build/mesa-runtime and available at the source URL.
Only this application's OpenGL CPU renderer loads these libraries. No system
driver installation, registry change, or GPU-driver replacement is performed.
The application requires GL_RENDERER to report llvmpipe before enabling it.
'@ | Set-Content -LiteralPath (Join-Path $OutputDirectory 'README.txt') -Encoding UTF8
Get-FileHash -LiteralPath (Join-Path $OutputDirectory 'opengl32.dll'),(Join-Path $OutputDirectory 'libgallium_wgl.dll') -Algorithm SHA256

