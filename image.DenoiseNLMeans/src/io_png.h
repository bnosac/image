#ifndef _IO_PNG_H
#define _IO_PNG_H

#ifdef __cplusplus
extern "C" {
#endif

#define IO_PNG_VERSION "0.20110608"

#include <stddef.h>

    unsigned char *io_png_read_u8(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp);
    unsigned char *io_png_read_u8_rgb(const char *fname, size_t *nxp, size_t *nyp);
    unsigned char *io_png_read_u8_gray(const char *fname, size_t *nxp, size_t *nyp);
    float *io_png_read_f32(const char *fname, size_t *nxp, size_t *nyp, size_t *ncp);
    float *io_png_read_f32_rgb(const char *fname, size_t *nxp, size_t *nyp);
    float *io_png_read_f32_gray(const char *fname, size_t *nxp, size_t *nyp);
    int io_png_write_u8(const char *fname, const unsigned char *data, size_t nx, size_t ny, size_t nc);
    int io_png_write_f32(const char *fname, const float *data, size_t nx, size_t ny, size_t nc);

#ifdef __cplusplus
}
#endif

#endif /* !_IO_PNG_H */
