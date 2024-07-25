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
