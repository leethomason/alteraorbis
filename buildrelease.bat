del /Q /S Altera\*.*
del /Q Altera\res\*.*

mkdir Altera
mkdir Altera\res
mkdir Altera\cache

copy .\res\*.* Altera\res\*.*

copy .\SDL2.dll Altera
copy .\SDL2_mixer.dll Altera

copy .\visstudio\Release\lumos.exe Altera\Altera.exe
copy README.txt Altera



