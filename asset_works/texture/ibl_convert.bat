@setlocal

set EXR2PNG=..\..\Tools\exr2png-1_2\exr2png.exe
set IMAGE_MAGICK=c:\usr\local\bin\ImageMagick-6.9.3-7-portable-Q16-x64\convert.exe
set SRC_DIR=%~dp0
set TMP_DIR=.\converted\
set OUT_DIR_FOR_OTHERS=.\to_others\IBL\
set OUT_DIR_FOR_ADRENO=.\to_adreno\IBL\
set CROP_SIZE=128x128
set CROPx0=0
set CROPx1=128
set CROPx2=256
set CROP_SIZE_L=256x256
set CROP_Lx0=0
set CROP_Lx1=256
set CROP_Lx2=512

set REGION_PX=+x 0 0 256 256 t
set REGION_NX=-x 512 256 768 512 l
set REGION_PY=+y 0 256 256 512 l
set REGION_NY=-y 512 0 768 256 b
set REGION_PZ=+z 256 256 512 512 l
set REGION_NZ=-z 256 0 512 256 t
set REGION=%REGION_PX% %REGION_NX% %REGION_PY% %REGION_NY% %REGION_PZ% %REGION_NZ% 

set CUBEMAP_SIZE_HIGH=256x256
set CUBEMAP_SIZE_IRR=8x8

set GEN_CUBEMAP=c:/usr/local/projects/gen_cubemap/gen_cubemap.exe
set CONV_FOR_OTHERS=C:\Imagination\PowerVR_Graphics\PowerVR_Tools\PVRTexTool\CLI\Windows_x86_64\PVRTexToolCLI.exe
set CONV_FOR_ADRENO=C:\usr\local\bin\ATCConv-0_2-x64\ATCConv.exe
set FMT=R8G8B8A8,UBN
set OPT=

@rem blender environment map layout in camera axis (0, 0, 0).
@rem -X|-Z|+X
@rem +Y|-Y|+Z

call :convert noon 0.333
call :convert sunset 1.1
call :convert night 0.25
goto :end

:convert
%EXR2PNG% -s %2 -f tga -c irradiance %REGION% "%1HighRez.exr" "irradiance"
if errorlevel 0 goto :success_irr
pause
exit 1
:success_irr
%IMAGE_MAGICK% "irradiance_px.tga" -flip -resize %CUBEMAP_SIZE_IRR% %TMP_DIR%tmp_px.png
%IMAGE_MAGICK% "irradiance_nx.tga" -flip -resize %CUBEMAP_SIZE_IRR% %TMP_DIR%tmp_nx.png
%IMAGE_MAGICK% "irradiance_py.tga" -flip -resize %CUBEMAP_SIZE_IRR% %TMP_DIR%tmp_py.png
%IMAGE_MAGICK% "irradiance_ny.tga" -flip -resize %CUBEMAP_SIZE_IRR% %TMP_DIR%tmp_ny.png
%IMAGE_MAGICK% "irradiance_pz.tga" -flip -resize %CUBEMAP_SIZE_IRR% %TMP_DIR%tmp_pz.png
%IMAGE_MAGICK% "irradiance_nz.tga" -flip -resize %CUBEMAP_SIZE_IRR% %TMP_DIR%tmp_nz.png
del "irradiance_px.tga"
del "irradiance_nx.tga"
del "irradiance_py.tga"
del "irradiance_ny.tga"
del "irradiance_pz.tga"
del "irradiance_nz.tga"
call :cubemap_for_others ibl_%1Irr.ktx
call :cubemap_for_adreno ibl_%1Irr.ktx

:convert_low
set /a "CUBEMAP_SIZE = 128"
set /a "ANGLE = 1"
set /a "BASE_SCALE = 1"
for /L %%i in (2, 1, 7) do (call :convert_cubemap %1 %2 ibl_%1_%%i.ktx %%i)
goto :convert_high

:convert_cubemap
%EXR2PNG% -s %2 -f tga -c filtered -a %ANGLE% %REGION% -m %BASE_SCALE% "%1HighRez.exr" "filtered"
if errorlevel 0 goto :success_low
pause
exit 1
:success_low
%IMAGE_MAGICK% "filtered_px.tga" -flip -resize %CUBEMAP_SIZE%x%CUBEMAP_SIZE% %TMP_DIR%tmp_nx.png
%IMAGE_MAGICK% "filtered_nx.tga" -flip -resize %CUBEMAP_SIZE%x%CUBEMAP_SIZE% %TMP_DIR%tmp_nz.png
%IMAGE_MAGICK% "filtered_py.tga" -flip -resize %CUBEMAP_SIZE%x%CUBEMAP_SIZE% %TMP_DIR%tmp_py.png
%IMAGE_MAGICK% "filtered_ny.tga" -flip -resize %CUBEMAP_SIZE%x%CUBEMAP_SIZE% %TMP_DIR%tmp_ny.png
%IMAGE_MAGICK% "filtered_pz.tga" -flip -resize %CUBEMAP_SIZE%x%CUBEMAP_SIZE% %TMP_DIR%tmp_px.png
%IMAGE_MAGICK% "filtered_nz.tga" -flip -resize %CUBEMAP_SIZE%x%CUBEMAP_SIZE% %TMP_DIR%tmp_pz.png
del "filtered_px.tga"
del "filtered_nx.tga"
del "filtered_py.tga"
del "filtered_ny.tga"
del "filtered_pz.tga"
del "filtered_nz.tga"
call :cubemap_for_others %3
call :cubemap_for_adreno %3
set /a "CUBEMAP_SIZE = CUBEMAP_SIZE / 2"
set /a "ANGLE = ANGLE * 2"
set /a "BASE_SCALE = BASE_SCALE * 2"
exit /b

:convert_high
%EXR2PNG% -s %2 -f tga -c none %REGION% "%1HighRez.exr" "skybox"
if errorlevel 0 goto :success_high
pause
exit 1
:success_high
%IMAGE_MAGICK% "skybox_px.tga" -flip -resize %CUBEMAP_SIZE_HIGH% %TMP_DIR%tmp_nx.png
%IMAGE_MAGICK% "skybox_nx.tga" -flip -resize %CUBEMAP_SIZE_HIGH% %TMP_DIR%tmp_nz.png
%IMAGE_MAGICK% "skybox_py.tga" -flip -resize %CUBEMAP_SIZE_HIGH% %TMP_DIR%tmp_py.png
%IMAGE_MAGICK% "skybox_ny.tga" -flip -resize %CUBEMAP_SIZE_HIGH% %TMP_DIR%tmp_ny.png
%IMAGE_MAGICK% "skybox_pz.tga" -flip -resize %CUBEMAP_SIZE_HIGH% %TMP_DIR%tmp_px.png
%IMAGE_MAGICK% "skybox_nz.tga" -flip -resize %CUBEMAP_SIZE_HIGH% %TMP_DIR%tmp_pz.png
del "skybox_px.tga"
del "skybox_nx.tga"
del "skybox_py.tga"
del "skybox_ny.tga"
del "skybox_pz.tga"
del "skybox_nz.tga"
call :cubemap_for_others ibl_%1_1.ktx
call :cubemap_for_adreno ibl_%1_1.ktx
exit /b

:cubemap_for_others
%CONV_FOR_OTHERS% -f %FMT% %OPT% -i %TMP_DIR%tmp_px.png -o tmp_px.ktx
%CONV_FOR_OTHERS% -f %FMT% %OPT% -i %TMP_DIR%tmp_nx.png -o tmp_nx.ktx
%CONV_FOR_OTHERS% -f %FMT% %OPT% -i %TMP_DIR%tmp_py.png -o tmp_py.ktx
%CONV_FOR_OTHERS% -f %FMT% %OPT% -i %TMP_DIR%tmp_ny.png -o tmp_ny.ktx
%CONV_FOR_OTHERS% -f %FMT% %OPT% -i %TMP_DIR%tmp_pz.png -o tmp_pz.ktx
%CONV_FOR_OTHERS% -f %FMT% %OPT% -i %TMP_DIR%tmp_nz.png -o tmp_nz.ktx
%GEN_CUBEMAP% -ftmp_pz.ktx -btmp_nz.ktx -ltmp_nx.ktx -rtmp_px.ktx -utmp_ny.ktx -dtmp_py.ktx -o%OUT_DIR_FOR_OTHERS%%1
del tmp_px.ktx
del tmp_nx.ktx
del tmp_py.ktx
del tmp_ny.ktx
del tmp_pz.ktx
del tmp_nz.ktx
exit /b

:cubemap_for_adreno
%CONV_FOR_ADRENO% -f atci %TMP_DIR%tmp_px.png tmp_px.ktx
%CONV_FOR_ADRENO% -f atci %TMP_DIR%tmp_nx.png tmp_nx.ktx
%CONV_FOR_ADRENO% -f atci %TMP_DIR%tmp_py.png tmp_py.ktx
%CONV_FOR_ADRENO% -f atci %TMP_DIR%tmp_ny.png tmp_ny.ktx
%CONV_FOR_ADRENO% -f atci %TMP_DIR%tmp_pz.png tmp_pz.ktx
%CONV_FOR_ADRENO% -f atci %TMP_DIR%tmp_nz.png tmp_nz.ktx
%GEN_CUBEMAP% -ftmp_pz.ktx -btmp_nz.ktx -ltmp_nx.ktx -rtmp_px.ktx -utmp_ny.ktx -dtmp_py.ktx -o%OUT_DIR_FOR_ADRENO%%1
del tmp_px.ktx
del tmp_nx.ktx
del tmp_py.ktx
del tmp_ny.ktx
del tmp_pz.ktx
del tmp_nz.ktx
exit /b

:end
@endlocal
