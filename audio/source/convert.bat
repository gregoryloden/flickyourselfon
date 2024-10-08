@echo off
set ffmpeg=C:\path\to\ffmpeg.exe

"%ffmpeg%" -i "slow walking.m4a" -filter:a "asetrate=66075" "slow walking sped up.wav"
"%ffmpeg%" -i "slow walking sped up.wav" -ss 13.95 -t 0.5 -af volume=1.4 ..\step1.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 18.5 -t 0.5 -af volume=1.8 ..\step2.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 22.5 -t 0.5 -af volume=2.3 ..\step3.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 35.15 -t 0.5 -af volume=1.1 ..\step4.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 16.5 -t 0.5 -af volume=1.5 ..\step5.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 17 -t 0.5 -af volume=1.4 ..\step6.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 3.7 -t 0.25 -af volume=3 ..\land.wav -y
del "slow walking sped up.wav"

"%ffmpeg%" -i "stomp dragging.m4a" -filter_complex "[0:a]volume=1.2,atrim=start=5.62:end=5.77[a0];[a0]asetrate=29400[a0a];[a0a]aresample=44100[a0b];[0:a]volume=0.5,atrim=start=10.9:end=11.3[a1];[a0b][a1]concat=n=2:v=0:a=1[outa]" -map "[outa]" ..\climb.wav -y

"%ffmpeg%" -i walking.m4a -filter_complex "[0:a]volume=1.35,atrim=start=40.08:end=40.28[a0];[0:a]volume=4.05,atrim=start=2.31:end=2.51[a1];[a0][a1]amix=inputs=2[outa]" -map "[outa]" "..\step off rail.wav" -y

"%ffmpeg%" -i whoo.m4a -i walking.m4a -filter_complex "[0:a]atrim=start=11.68:end=12.10,volume=3,asetrate=52444[a0];[a0]aresample=44100[a0a];[1:a]atrim=start=35.72:end=35.97,volume=4[a1];[a0a][a1]amix=inputs=2[outa]" -map "[outa]" ..\jump.wav -y
"%ffmpeg%" -i ..\jump.wav -t 0.25 ..\jump.wav -y
"%ffmpeg%" -i whoo.m4a -filter_complex "[0:a]atrim=start=5.56:end=5.81,volume=6[a0];[0:a]atrim=start=14.7:end=14.95,volume=8[a1];[a0][a1]amix=inputs=2,asetrate=31183[outa]" -map "[outa]" "..\kick air.wav" -y
"%ffmpeg%" -i "..\kick air.wav" -t 0.25 "..\kick air.wav" -y

"%ffmpeg%" -i kicking.m4a -filter_complex "[0:a]atrim=start=1.98:end=2.13[a0];[0:a]atrim=start=3.18:end=3.33[a1];[a0][a1]amix=inputs=2[a2];[a2]asetrate=55563,volume=2[outa]" -map "[outa]" ..\kick.wav -y

"%ffmpeg%" -i "grind metal.m4a" -ss 5.15 -t 0.75 -af volume=0.35 "..\rail slide.wav" -y
"%ffmpeg%" -i "grind metal.m4a" -filter_complex "[0:a]volume=0.35,atrim=start=5.15:end=5.9[a0];[a0]asetrate=58866[outa]" -map "[outa]" "..\rail slide square.wav" -y
"%ffmpeg%" -i "..\rail slide square.wav" -t 0.375 "..\rail slide square.wav" -y

REM original times are 3.66-3.86, 5.52-5.72, 7.31-7.51, 9.09-9.29, the 4th flick was discarded and the others were spaced out to be better distinguishable
"%ffmpeg%" -i "flick switch.m4a" -filter_complex "[0:a]atrim=start=3.64:end=3.84[a1];[0:a]atrim=start=5.51:end=5.71[a2];[0:a]atrim=start=7.31:end=7.51[a3];[a1][a2][a3]amix=inputs=3,asetrate=62367[apre];[apre]volume=1.5[outa]" -map "[outa]" "..\switch on.wav" -y
"%ffmpeg%" -i "flick switch.m4a" -filter_complex "[0:a]volume=0.5,atrim=start=4.596:end=4.65,asetrate=88200[outa]" -map "[outa]" ..\select.wav -y
"%ffmpeg%" -i "flick switch.m4a" -filter_complex "[0:a]volume=0.5,atrim=start=4.596:end=4.65,asetrate=66075[outa]" -map "[outa]" ..\confirm.wav -y

"%ffmpeg%" -i "step on rail.m4a" -filter_complex "[0:a]volume=0.4,atrim=start=6.61:end=6.61785[a0];[0:a]volume=2,atrim=start=6.61785:end=6.865[a1];[a0][a1]concat=n=2:v=0:a=1[outa]" -map "[outa]" "..\step on rail.wav" -y

"%ffmpeg%" -i "slide metal.m4a" -ss 5.7 -t 0.24 -af volume=1.5 "..\ride rail1.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 27.66 -t 0.24 -af volume=1.75 "..\ride rail2.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 8.86 -t 0.24 -af volume=1.75 "..\ride rail3.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 17.9 -t 0.24 -af volume=3 "..\ride rail4.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 11.51 -t 0.24 -af volume=2.7 "..\ride rail5.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 20.9 -t 0.24 -af volume=2.75 "..\ride rail6.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 6.98 -t 0.24 -af volume=1.5 "..\ride rail out3.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 7.06 -t 0.16 -af volume=1.5 "..\ride rail out2.wav" -y
"%ffmpeg%" -i "slide metal.m4a" -ss 7.14 -t 0.08 -af volume=1.5 "..\ride rail out1.wav" -y
