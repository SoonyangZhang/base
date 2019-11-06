#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
float x264_pixel_ssim_wxh(
                           uint8_t *pix1, int stride1,
                           uint8_t *pix2, int stride2,
                           int width, int height );
double calculate_psnr(uint8_t *pix1,uint8_t *pix2,int inc,int y_size);
#ifdef __cplusplus
}
#endif