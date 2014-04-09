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

copy .\SDL2.dll Altera
copy .\SDL2_mixer.dll Altera

copy .\visstudio\Release\lumos.exe Altera\Altera.exe
copy README.txt Altera



