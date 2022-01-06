#!/bin/bash
set -e

currentDir=$(dirname "${BASH_SOURCE[0]}")
glslDir="$currentDir"/glsl460
spvDir="$currentDir"/spv
spvHexDir="$currentDir"/spv/.hex

rm -rf "$spvDir"
mkdir "$spvDir"
mkdir "$spvHexDir"

for i in "${glslDir}"/*.glsl; do
  name=$(basename "$i" .glsl)
  spvFile="$spvDir"/"$name".spv
  spvHexFile="$spvHexDir"/"$name".spv
  glslc --target-spv=spv1.0 -g "$i" -o "$spvFile"
  spirv-opt -O "$spvFile" -o "$spvFile"
  xxd -i <"$spvFile" >"$spvHexFile"
done
