x264 encoder example  
build with cmake or gcc  
of course , the LD_LIBRARY_PATH=/usr/local/lib  should be configured in /etc/profile  
gcc x264_test1.c -o x264_encoder -I/usr/local/include -L/usr/local/lib -lpthread -lm -lx264  

./x264_encoder 640x480 decode.yuv output.264  
encode video with ffmpeg and libx264:  
ffmpeg -s 640x480 -i decode.yuv -r 25 -vcodec libx264 ds2.h264   
  
other commad for video show:  
ffplay -f rawvideo -video_size  640x480  decode.yuv  
 
ffplay -stats -f h264 ds2.h264  
./configure --enable-shared --enable-libx264 --enable-gpl  

decode h264  with ffmpeg  
ffmpeg -i out.h264 file.yuv  

nm -D /usr/local/lib/libx264.so | grep x264_encoder_open  

gcc psnr.c  -o psnr  -lm  
./psnr 640 480 420 videoraw.yuv file.yuv ssim > ref_ssim.txt  

And the encoded h264 bitstream should be used with Elecard StreamEye Tools to get the statistics.  
