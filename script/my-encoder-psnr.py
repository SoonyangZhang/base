import os
import time
import subprocess ,signal
encode_rates=[1000,1500,2000,3000,4000]
outName=["1","1.5","2","3","4"]
yuv="../data/mobcal.yuv"
width="1280"
height="720"
encode_cmd="../build/encoder -w  %s -h %s -i %s -o %sM.h264 -b %s"
'''
for i in range(len(encode_rates)):
    cmd=encode_cmd%(width,height,yuv,outName[i],str(encode_rates[i]))
    process= subprocess.Popen(cmd,shell = True)
    while 1:
        time.sleep(1)
        ret=subprocess.Popen.poll(process)
        if ret is None:
            continue
        else:
            break 
decode_cmd="ffmpeg -i %sM.h264 %s.yuv"
for i in range(len(encode_rates)):
    cmd=decode_cmd%(outName[i],outName[i])
    process= subprocess.Popen(cmd,shell = True)
    while 1:
        time.sleep(1)
        ret=subprocess.Popen.poll(process)
        if ret is None:
            continue
        else:
            break 
psnr_cmd="../build/t_psnr -w %s -h %s -s %s -d %s.yuv -f 420 -o %s_psnr.txt"
for i in range(len(encode_rates)):
    cmd=psnr_cmd%(width,height,yuv,outName[i],str(i+1))
    process= subprocess.Popen(cmd,shell = True)
    while 1:
        time.sleep(1)
        ret=subprocess.Popen.poll(process)
        if ret is None:
            continue
        else:
            break
'''
for i in range(len(encode_rates)):
    encodeFile="%sM.h264"%(outName[i])
    decodeFile="%s.yuv"%(outName[i])
    os.remove(encodeFile)
    os.remove(decodeFile)