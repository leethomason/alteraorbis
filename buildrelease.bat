del /Q /S Altera\*.*
del /Q Altera\res\*.*

mkdir Altera
mkdir Altera\res
mkdir Altera\cache
mkdir Altera\save

copy .\res\*.* Altera\res\*.*

copy .\save\game_def.dat Altera\save\game_def.dat
copy .\save\map_def.dat Altera\save\map_def.dat
copy .\save\map_def.png Altera\save\map_def.png

copy c:\bin\SDL.dll Altera
copy c:\bin\SDL_image.dll Altera

copy .\visstudio\Release\lumos.exe Altera\Altera.exe
copy .\visstudio\Release\builder.exe Altera\builder.exe
copy README.txt Altera


