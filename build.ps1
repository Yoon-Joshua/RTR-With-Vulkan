$vert = ".vert"
$frag = "frag"

cd shader
$folders = Get-ChildItem
foreach ($folder in $folders){
    $shaders = Get-ChildItem $folder -Filter *.vert
    foreach($shader in $shaders){
        $output=$shader.FullName+".spv"
        glslc $shader.FullName -o $output
    }

    $shaders = Get-ChildItem $folder -Filter *.frag
    foreach($shader in $shaders){
        $output=$shader.FullName+".spv"
        glslc $shader.FullName -o $output
    }
}
cd ..
