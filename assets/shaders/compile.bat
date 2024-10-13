@echo off
rem Create the spv directory if it doesn't exist
if not exist spv (
    mkdir spv
)

rem Delete all .spv files in the spv directory
del spv\*.spv /Q

rem Compile all .vert files to spv directory with .vert.spv extension
for %%f in (*.vert) do (
    echo [Compiling] [VERT]: %%f
    glslc %%f -o spv\%%~nf.vert.spv
)

rem Compile all .frag files to spv directory with .frag.spv extension
for %%f in (*.frag) do (
    echo [Compiling] [FRAG]: %%f
    glslc %%f -o spv\%%~nf.frag.spv
)

echo Finished compiling all .vert and .frag files.
