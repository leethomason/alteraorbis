del /Q /S Altera\*.*
del /Q Altera\res\*.*

mkdir Altera
mkdir Altera\res

copy .\res\*.* Altera\res\*.*

copy c:\bin\SDL.dll Altera
copy c:\bin\SDL_image.dll Altera

copy .\visstudio\Release\lumos.exe Altera\Altera.exe
copy .\visstudio\Release\builder.exe Altera\builder.exe
copy README.txt Altera


