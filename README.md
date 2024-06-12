# KokkuRenderingEngineerTest
3D model loader and scene implemented using The Forge Framework for the Kokku Rendering Engineer Test

## Instructions:
1. Clone The-Forge lib in this repo by running  "git submodule update --init --recursive"
2. Run the PRE_BUILD.bat script in The-Forge root to download art assets
3. To build and run the project, open the VisualStudio solution in PCVisualStudio2022 and build & run.

## Obs:
- The Castle mesh has been converted to glTF with the usage of: https://github.com/facebookincubator/FBX2glTF

- The generated glTF mesh was then converted to the .bin file type by the AssetPipelineCMD tool included in The-Forge framework.
  This was done to allow the forge resource loading system to load the file.
