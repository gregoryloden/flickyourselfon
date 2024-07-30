@echo off
set ffmpeg=C:\path\to\ffmpeg.exe

"%ffmpeg%" -i "slow walking.m4a" -filter:a "asetrate=66075" "slow walking sped up.wav"
"%ffmpeg%" -i "slow walking sped up.wav" -ss 13.95 -t 0.5 -af volume=0.70 ..\step1.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 18.5 -t 0.5 -af volume=0.9 ..\step2.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 22.5 -t 0.5 -af volume=1 ..\step3.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 35.15 -t 0.5 -af volume=0.55 ..\step4.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 16.5 -t 0.5 -af volume=0.75 ..\step5.wav -y
"%ffmpeg%" -i "slow walking sped up.wav" -ss 17 -t 0.5 -af volume=0.7 ..\step6.wav -y
del "slow walking sped up.wav"

"%ffmpeg%" -i "stomp dragging.m4a" -filter_complex "[0:a]volume=0.6,atrim=start=5.62:end=5.77[a0];[a0]asetrate=29400[a0a];[a0a]aresample=44100[a0b];[0:a]volume=0.25,atrim=start=10.9:end=11.3[a1];[a0b][a1]concat=n=2:v=0:a=1[outa]" -map "[outa]" ..\climb.wav -y

"%ffmpeg%" -i kicking.m4a -filter_complex "[0:a]volume=1,atrim=start=1.98:end=2.13[a0];[0:a]volume=1,atrim=start=3.18:end=3.33[a1];[a0][a1]amix=inputs=2[a2];[a2]asetrate=74167[outa]" -map "[outa]" ..\kick.wav -y
