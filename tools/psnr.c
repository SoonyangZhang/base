#include <math.h>
#include <string.h>
#include "psnr.h"
#ifdef _WIN32
  #define alloca _alloca
#else
#endif

/****************************************************************************
 * structural similarity metric [from x264]
 ****************************************************************************/

#define x264_alloca(x) (void*)(((intptr_t)alloca((x)+15)+15)&~15)
#define XCHG(type,a,b) { type t = a; a = b; b = t; }
#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )

static void ssim_4x4x2_core( const uint8_t *pix1, int stride1,
                             const uint8_t *pix2, int stride2,
                             int sums[2][4])
{
    int x, y, z;
    for(z=0; z<2; z++)
    {
        uint32_t s1=0, s2=0, ss=0, s12=0;
        for(y=0; y<4; y++)
            for(x=0; x<4; x++)
            {
                int a = pix1[x+y*stride1];
                int b = pix2[x+y*stride2];
                s1  += a;
                s2  += b;
                ss  += a*a;
                ss  += b*b;
                s12 += a*b;
            }
        sums[z][0] = s1;
        sums[z][1] = s2;
        sums[z][2] = ss;
        sums[z][3] = s12;
        pix1 += 4;
        pix2 += 4;
    }
}

static float ssim_end1( int s1, int s2, int ss, int s12 )
{
    static const int ssim_c1 = (int)(.01*.01*255*255*64 + .5);
    static const int ssim_c2 = (int)(.03*.03*255*255*64*63 + .5);
    int vars = ss*64 - s1*s1 - s2*s2;
    int covar = s12*64 - s1*s2;
    return (float)(2*s1*s2 + ssim_c1) * (float)(2*covar + ssim_c2)\
           / ((float)(s1*s1 + s2*s2 + ssim_c1) * (float)(vars + ssim_c2));
}

static float ssim_end4( int sum0[5][4], int sum1[5][4], int width )
{
    int i;
    float ssim = 0.0;
    for( i = 0; i < width; i++ )
        ssim += ssim_end1( sum0[i][0] + sum0[i+1][0] + sum1[i][0] + sum1[i+1][0],
                           sum0[i][1] + sum0[i+1][1] + sum1[i][1] + sum1[i+1][1],
                           sum0[i][2] + sum0[i+1][2] + sum1[i][2] + sum1[i+1][2],
                           sum0[i][3] + sum0[i+1][3] + sum1[i][3] + sum1[i+1][3] );
    return ssim;
}

float x264_pixel_ssim_wxh(
                           uint8_t *pix1, int stride1,
                           uint8_t *pix2, int stride2,
                           int width, int height )
{
    int x, y, z;
    float ssim = 0.0;
    int (*sum0)[4] = x264_alloca(4 * (width/4+3) * sizeof(int));
    int (*sum1)[4] = x264_alloca(4 * (width/4+3) * sizeof(int));
    width >>= 2;
    height >>= 2;
    z = 0;
    for( y = 1; y < height; y++ )
    {
        for( ; z <= y; z++ )
        {
            XCHG( void*, sum0, sum1 );
            for( x = 0; x < width; x+=2 )
                ssim_4x4x2_core( &pix1[4*(x+z*stride1)], stride1, &pix2[4*(x+z*stride2)], stride2, &sum0[x] );
        }
        for( x = 0; x < width-1; x += 4 )
            ssim += ssim_end4( sum0+x, sum1+x, X264_MIN(4,width-x-1) );
    }
    return ssim / ((width-1) * (height-1));
}
double calculate_psnr(uint8_t *pix1,uint8_t *pix2,int inc,int y_size){
	double yrmse=0;
	double diff=0;
	double psnr=0;
	int i=0;
	for(i=0;i<y_size;i+=inc){
        diff = pix1[i] - pix2[i];
        yrmse += diff * diff;
	}
	psnr=yrmse ? 20 * (log10(255 / sqrt(yrmse / y_size))) : 0;
	return psnr;
}