cd shader

$shaders = Get-ChildItem -Recurse -File -Filter "*.spv"
echo $shaders.FullName

foreach ($shader in $shaders){
    Remove-Item $shader.FullName
}

cd ..