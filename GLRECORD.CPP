#include "GLRECORD.HPP"
#define WATCOMGLAPI
#include "GL.H"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef __WATCOMC__
#include <i86.h>
#include <conio.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#endif // __DJGPP__

//--------------------------

#define MP4MUXBUFFERSIZE (1024*1024)
char glRecordDescFileName[MAXGLRECORDFILENAMELENGTH] = {"record.dsc"};
char glRecordFileName[MAXGLRECORDFILENAMELENGTH] = {"r:/snapshot.rec"}; // please use a ramdisc here
char glRecordH264FileName[MAXGLRECORDFILENAMELENGTH] = {"c:/h264.264"};
char glRecordMP4FileName[MAXGLRECORDFILENAMELENGTH] = {"c:/video.mp4"};
static double glRecordFrameRate = 25.0;
int glRecordWidth = -1;
int glRecordHeight = -1;

//--------------------------

static void recSetBiosCursor(int x, int y) {
    union REGS regs;
    regs.w.ax = 0x200;
    regs.h.bh = 0;
    regs.h.dl = x;
    regs.h.dh = y;
    int386(0x10, &regs, &regs);
}

//--------------------------

#define MINIH264_IMPLEMENTATION
#include "enc_h264.hpp"
#define MINIMP4_IMPLEMENTATION
#include "minimp4.hpp"

#include <stdlib.h>

#include <math.h>
#include "gl.h"

class H264Writer {
public:
  H264E_persist_t *enc;
  H264E_scratch_t *scratch;
  int error,width,height;

  H264Writer(int w, int h) {
    if ((w|h)&1) {error = -256;return;}
    enc = NULL; scratch = NULL;
    error = width = height = 0;
    H264E_create_param_t create_param;
    memset(&create_param,0,sizeof(H264E_create_param_t));
    create_param.enableNEON = 0;
    create_param.num_layers = 1;
    create_param.inter_layer_pred_flag = 0;
    create_param.gop = 20; // DEFAULT_GOP
    create_param.height = h;
    create_param.width  = w;
    create_param.max_long_term_reference_frames = 0;
    create_param.fine_rate_control_flag = 0;
    create_param.const_input_flag = 1;
    create_param.vbv_size_bytes = 0;//kbps*1000/8*2; // 2 seconds
    create_param.temporal_denoise_flag = 0;
    int sizeof_persist = 0, sizeof_scratch = 0;
    error = H264E_sizeof(&create_param, &sizeof_persist, &sizeof_scratch);
    if (error) {
        printf("H264E_init error = %d\n", error);
        return;
    }
    enc     = (H264E_persist_t *)glMalloc(sizeof_persist);
    scratch = (H264E_scratch_t *)glMalloc(sizeof_scratch);
    error = H264E_init(enc, &create_param);
    width = w;
    height = h;
  }

  ~H264Writer() {
    if (enc) glFree(enc);
    if (scratch) glFree(scratch);
  }

  bool yuvToFile(unsigned char *y, unsigned char *u, unsigned char *v, FILE *file, int uvstride = -1) {

    H264E_io_yuv_t yuv;
    yuv.yuv[0] = y; yuv.stride[0] = width;
    yuv.yuv[1] = u; yuv.stride[1] = uvstride>=0 ? uvstride : width/2;
    yuv.yuv[2] = v; yuv.stride[2] = uvstride>=0 ? uvstride : width/2;

    H264E_run_param_t run_param;
    memset(&run_param,0,sizeof(H264E_run_param_t));

    run_param.frame_type = 0;
    run_param.encode_speed = 0; // 0 means best quality, 10 fastest

    //if (kbps) {run_param.desired_frame_bytes = kbps*1000/8/30; run_param.qp_min = 10; run_param.qp_max = 50; }
    run_param.qp_min = run_param.qp_max = 10; // quality 10 = best, 50=worst for 320x200 10 is "ok"

    uint8_t *coded_data;
    int sizeof_coded_data;

    error = H264E_encode(enc, scratch, &run_param, &yuv, &coded_data, &sizeof_coded_data);
    if (error != 0)
      return false;

    if (!fwrite(coded_data, sizeof_coded_data, 1, file)) {
      error = -255;
      return false;
    }

    return error == 0;
  }

  bool rgbaToFile(unsigned int *rgba, FILE *file) {
    unsigned char *y = (unsigned char*)glMalloc(width*height);
    unsigned char *u = (unsigned char*)glMalloc(width*height);
    unsigned char *v = (unsigned char*)glMalloc(width*height);
    unsigned char *yk = y;
    unsigned char *uk = u;
    unsigned char *vk = v;
    unsigned int *k = rgba;
    for (int yp = 0; yp < height; yp++) {
      for (int xp = 0; xp < width; xp++) {
        int r = *k & 255;
        int g = ((*k)>>8) & 255;
        int b = ((*k)>>16) & 255;
        k++;
        *yk++ = ((66*r+129*g+25*b+128)>>8)+16;
        *uk++ = ((-38*r-74*g+112*b+128)>>8)+128;
        *vk++ = ((112*r-94*g-18*b+128)>>8)+128;
      }
    }

    {
      for (int yp = 0; yp < height/2; yp++) {
        uk = &u[yp*width];
        vk = &v[yp*width];
        unsigned char *uo = &u[yp*width*2];
        unsigned char *vo = &v[yp*width*2];
        for (int xp = 0; xp < width/2; xp++) {
          *uk++ = ((int)*uo + *(uo+1) + *(uo+width) + *(uo+width + 1))>>2;
          *vk++ = ((int)*vo + *(vo+1) + *(vo+width) + *(vo+width + 1))>>2;
          uo+=2;
          vo+=2;
        }
      }
    }
    yuvToFile(y, u, v, file, width); // stride is width not width/2
    glFree(y);
    glFree(u);
    glFree(v);
    return error==0;
  }
};

static H264Writer *h264Writer = NULL;
static FILE *h264File = NULL;
static int currentH264FrameWritten = 0;

void muxh264(const char *h264FileName, const char *mp4FileName);

static void stopH264Recording() {
  if (h264Writer != NULL) {delete h264Writer; h264Writer = NULL;}
  if (h264File != NULL) {
    fclose(h264File); h264File = NULL;
    muxh264(glRecordH264FileName, glRecordMP4FileName);
  }
}

static void startH264Recording(const char *fileName) {
  stopH264Recording();
  h264Writer = new H264Writer(glRecordWidth > 0 ? glRecordWidth : glFrameBufferWidth, glRecordHeight > 0 ? glRecordHeight : glFrameBufferHeight);
  h264File = fopen(fileName,"wb");
  currentH264FrameWritten = 0;
}

static uint8_t *preload(const char *path, ssize_t *data_size)
{
    FILE *file = fopen(path, "rb");
    uint8_t *data;
    *data_size = 0;
    if (!file)
        return 0;
    if (fseek(file, 0, SEEK_END))
        exit(1);
    *data_size = (ssize_t)ftell(file);
    if (*data_size < 0)
        exit(1);
    if (fseek(file, 0, SEEK_SET))
        exit(1);
    data = (unsigned char*)malloc(*data_size);
    if (!data)
        exit(1);
    if ((ssize_t)fread(data, 1, *data_size, file) != *data_size)
        exit(1);
    fclose(file);
    return data;
}

static ssize_t get_nal_size(uint8_t *buf, ssize_t size)
{
    ssize_t pos = 3;
    while ((size - pos) > 3)
    {
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 1)
            return pos;
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 0 && buf[pos + 3] == 1)
            return pos;
        pos++;
    }
    return size;
}

static int write_callback(int64_t offset, const void *buffer, size_t size, void *token)
{
    FILE *f = (FILE*)token;
    fseek(f, offset, SEEK_SET);
    return fwrite(buffer, 1, size, f) != size;
}

typedef struct
{
    uint8_t *buffer;
    ssize_t size;
} INPUT_BUFFER;

static int read_callback(int64_t offset, void *buffer, size_t size, void *token)
{
    INPUT_BUFFER *buf = (INPUT_BUFFER*)token;
    size_t to_copy = MINIMP4_MIN(size, buf->size - offset - size);
    memcpy(buffer, buf->buffer + offset, to_copy);
    return to_copy != size;
}

int demux(uint8_t *input_buf, ssize_t input_size, FILE *fout, int ntrack)
{
    int /*ntrack, */i, spspps_bytes;
    const void *spspps;
    INPUT_BUFFER buf = { input_buf, input_size };
    MP4D_demux_t mp4 = { 0, };
    MP4D_open(&mp4, read_callback, &buf, input_size);

    //for (ntrack = 0; ntrack < mp4.track_count; ntrack++)
    {
        MP4D_track_t *tr = mp4.track + ntrack;
        unsigned sum_duration = 0;
        i = 0;
        if (tr->handler_type == MP4D_HANDLER_TYPE_VIDE)
        {   // assume h264
#define USE_SHORT_SYNC 0
            char sync[4] = { 0, 0, 0, 1 };
            while (spspps = MP4D_read_sps(&mp4, ntrack, i, &spspps_bytes))
            {
                fwrite(sync + USE_SHORT_SYNC, 1, 4 - USE_SHORT_SYNC, fout);
                fwrite(spspps, 1, spspps_bytes, fout);
                i++;
            }
            i = 0;
            while (spspps = MP4D_read_pps(&mp4, ntrack, i, &spspps_bytes))
            {
                fwrite(sync + USE_SHORT_SYNC, 1, 4 - USE_SHORT_SYNC, fout);
                fwrite(spspps, 1, spspps_bytes, fout);
                i++;
            }
            for (i = 0; i < mp4.track[ntrack].sample_count; i++)
            {
                unsigned frame_bytes, timestamp, duration;
                MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
                uint8_t *mem = input_buf + ofs;
                sum_duration += duration;
                while (frame_bytes)
                {
                    uint32_t size = ((uint32_t)mem[0] << 24) | ((uint32_t)mem[1] << 16) | ((uint32_t)mem[2] << 8) | mem[3];
                    size += 4;
                    mem[0] = 0; mem[1] = 0; mem[2] = 0; mem[3] = 1;
                    fwrite(mem + USE_SHORT_SYNC, 1, size - USE_SHORT_SYNC, fout);
                    if (frame_bytes < size)
                    {
                        printf("error: demux sample failed\n");
                        exit(1);
                    }
                    frame_bytes -= size;
                    mem += size;
                }
            }
        } else if (tr->handler_type == MP4D_HANDLER_TYPE_SOUN)
        {   // assume aac
#if ENABLE_AUDIO
            HANDLE_AACDECODER dec = aacDecoder_Open(TT_MP4_RAW, 1);
            UCHAR *dsi = (UCHAR *)tr->dsi;
            UINT dsi_size = tr->dsi_bytes;
            if (AAC_DEC_OK != aacDecoder_ConfigRaw(dec, &dsi, &dsi_size))
            {
                printf("error: aac config fail\n");
                exit(1);
            }
#endif
            for (i = 0; i < mp4.track[ntrack].sample_count; i++)
            {
                unsigned frame_bytes, timestamp, duration;
                MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
                printf("ofs=%d frame_bytes=%d timestamp=%d duration=%d\n", (unsigned)ofs, frame_bytes, timestamp, duration);
#if ENABLE_AUDIO
                UCHAR *frame = (UCHAR *)(input_buf + ofs);
                UINT frame_size = frame_bytes;
                UINT valid = frame_size;
                if (AAC_DEC_OK != aacDecoder_Fill(dec, &frame, &frame_size, &valid))
                {
                    printf("error: aac decode fail\n");
                    exit(1);
                }
                INT_PCM pcm[2048*8];
                int err = aacDecoder_DecodeFrame(dec, pcm, sizeof(pcm), 0);
                if (AAC_DEC_OK != err)
                {
                    printf("error: aac decode fail %d\n", err);
                    exit(1);
                }
                CStreamInfo *info = aacDecoder_GetStreamInfo(dec);
                if (!info)
                {
                    printf("error: aac decode fail\n");
                    exit(1);
                }
                fwrite(pcm, sizeof(INT_PCM)*info->frameSize*info->numChannels, 1, fout);
#endif
            }
        }
    }

    MP4D_close(&mp4);
    if (input_buf)
        free(input_buf);
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
GLushort gl2NextKey();
#ifdef __cplusplus
}
#endif // __cplusplus

void muxh264(const char *h264FileName, const char *mp4FileName) {

  int is_hevc = false;
  int sequential_mode = 0;
  int fragmentation_mode = 0;
  
  FILE *fout = fopen(mp4FileName, "wb");
  if (!fout)
  {
      printf("muxh264 error: can't open output file\n");
      exit(1);
  }

  FILE *fin = fopen(h264FileName, "rb");
  if (!fin)
  {
      printf("muxh264 error: can't open input file\n");
      exit(1);
  }
  fseek(fin,0,SEEK_END);
  int h264_size = ftell(fin);
  int h264_pos = 0;
  fseek(fin,h264_pos,SEEK_SET);
  uint8_t *alloc_buf = (uint8_t*)glMalloc(MP4MUXBUFFERSIZE);
  uint8_t *buf_h264 = NULL;

  MP4E_mux_t *mux;
  mp4_h26x_writer_t mp4wr;
  
  mux = MP4E_open(sequential_mode, fragmentation_mode, fout, write_callback);
  if (MP4E_STATUS_OK != mp4_h26x_write_init(&mp4wr, mux, 352, 288, is_hevc))
  {
      printf("error: mp4_h26x_write_init failed\n");
      exit(1);
  }

  fseek(fin,h264_pos,SEEK_SET);
  fread(alloc_buf,1,MP4MUXBUFFERSIZE,fin);
  buf_h264 = alloc_buf;

  int nals = 0;  
  while (h264_size > 0)
  {
      ssize_t nal_size = get_nal_size(buf_h264, h264_size);
      if (nal_size < 4)
      {
          h264_pos += 1;
          buf_h264  += 1;
          h264_size -= 1;
          continue;
      }
      if (MP4E_STATUS_OK != mp4_h26x_write_nal(&mp4wr, buf_h264, nal_size, 90000/glRecordFrameRate))
      {
          printf("error: mp4_h26x_write_nal failed\n");
          exit(1);
      }
      h264_size -= nal_size;
      h264_pos += nal_size;

      recSetBiosCursor(0,0);
      nals++;
      printf("muxing nal:%d\n",nals);
      if (gl2NextKey()==GL_VK_END) {glDone(); printf("END key pressed in mp4 muxing.\n"); exit(0);}


      fseek(fin,h264_pos,SEEK_SET);
      fread(alloc_buf,1,MP4MUXBUFFERSIZE,fin);
      buf_h264 = alloc_buf;
  }
  if (alloc_buf)
      glFree(alloc_buf);
  MP4E_close(mux);
  mp4_h26x_write_close(&mp4wr);
  if (fout)
      fclose(fout);
  if (fin)
      fclose(fin);

}

int main_minimp4(int argc, char **argv)
{
    // check switches
    int sequential_mode = 0;
    int fragmentation_mode = 0;
    int do_demux = 0;
    int track = 0;
    int i;
    for(i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
            break;
        switch (argv[i][1])
        {
        case 'm': do_demux = 0; break;
        case 'd': do_demux = 1; break;
        case 's': sequential_mode = 1; break;
        case 'f': fragmentation_mode = 1; break;
        case 't': i++; if (i < argc) track = atoi(argv[i]); break;
        default:
            printf("error: unrecognized option\n");
            return 1;
        }
    }
    if (argc <= (i + 1))
    {
        printf("Usage: minimp4 [command] [options] input output\n"
               "Commands:\n"
               "    -m    - do muxing (default); input is h264 elementary stream, output is mp4 file\n"
               "    -d    - do de-muxing; input is mp4 file, output is h264 elementary stream\n"
               "Options:\n"
               "    -s    - enable mux sequential mode (no seek required for writing)\n"
               "    -f    - enable mux fragmentation mode (aka fMP4)\n"
               "    -t    - de-mux tack number\n");
        return 0;
    }
    ssize_t h264_size;
    uint8_t *alloc_buf;
    uint8_t *buf_h264 = alloc_buf = preload(argv[i], &h264_size);
    if (!buf_h264)
    {
        printf("error: can't open h264 file\n");
        exit(1);
    }

    FILE *fout = fopen(argv[i + 1], "wb");
    if (!fout)
    {
        printf("error: can't open output file\n");
        exit(1);
    }

    if (do_demux)
        return demux(alloc_buf, h264_size, fout, track);

    int is_hevc = (0 != strstr(argv[i], "265")) || (0 != strstr(argv[i], "hevc"));

    MP4E_mux_t *mux;
    mp4_h26x_writer_t mp4wr;
    mux = MP4E_open(sequential_mode, fragmentation_mode, fout, write_callback);
    if (MP4E_STATUS_OK != mp4_h26x_write_init(&mp4wr, mux, 352, 288, is_hevc))
    {
        printf("error: mp4_h26x_write_init failed\n");
        exit(1);
    }

#if ENABLE_AUDIO
    ssize_t pcm_size;
    int16_t *alloc_pcm;
    int16_t *buf_pcm  = alloc_pcm = (int16_t *)preload("stream.pcm", &pcm_size);
    if (!buf_pcm)
    {
        printf("error: can't open pcm file\n");
        exit(1);
    }
    uint32_t sample = 0, total_samples = pcm_size/2;
    uint64_t ts = 0, ats = 0;
    HANDLE_AACENCODER aacenc;
    AACENC_InfoStruct info;
    aacEncOpen(&aacenc, 0, 0);
    aacEncoder_SetParam(aacenc, AACENC_TRANSMUX, 0);
    aacEncoder_SetParam(aacenc, AACENC_AFTERBURNER, 1);
    aacEncoder_SetParam(aacenc, AACENC_BITRATE, 64000);
    aacEncoder_SetParam(aacenc, AACENC_SAMPLERATE, AUDIO_RATE);
    aacEncoder_SetParam(aacenc, AACENC_CHANNELMODE, 1);
    aacEncEncode(aacenc, NULL, NULL, NULL, NULL);
    aacEncInfo(aacenc, &info);

    MP4E_track_t tr;
    tr.track_media_kind = e_audio;
    tr.language[0] = 'u';
    tr.language[1] = 'n';
    tr.language[2] = 'd';
    tr.language[3] = 0;
    tr.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3;
    tr.time_scale = 90000;
    tr.default_duration = 0;
    tr.u.a.channelcount = 1;
    int audio_track_id = MP4E_add_track(mux, &tr);
    MP4E_set_dsi(mux, audio_track_id, info.confBuf, info.confSize);
#endif
    while (h264_size > 0)
    {
        ssize_t nal_size = get_nal_size(buf_h264, h264_size);
        if (nal_size < 4)
        {
            buf_h264  += 1;
            h264_size -= 1;
            continue;
        }
        /*int startcode_size = 4;
        if (buf_h264[0] == 0 && buf_h264[1] == 0 && buf_h264[2] == 1)
            startcode_size = 3;
        int nal_type = buf_h264[startcode_size] & 31;
        int is_intra = (nal_type == 5);
        printf("nal size=%ld, nal_type=%d\n", nal_size, nal_type);*/

        if (MP4E_STATUS_OK != mp4_h26x_write_nal(&mp4wr, buf_h264, nal_size, 90000/glRecordFrameRate))
        {
            printf("error: mp4_h26x_write_nal failed\n");
            exit(1);
        }
        buf_h264  += nal_size;
        h264_size -= nal_size;

#if ENABLE_AUDIO
        if (fragmentation_mode && !mux->fragments_count)
            continue; /* make sure mp4_h26x_write_nal writes sps/pps, because in fragmentation mode first MP4E_put_sample writes moov with track information and dsi.
                         all tracks dsi must be set (MP4E_set_dsi) before first MP4E_put_sample. */
        ts += 90000/glRecordFrameRate;
        while (ats < ts)
        {
            AACENC_BufDesc in_buf, out_buf;
            AACENC_InArgs  in_args;
            AACENC_OutArgs out_args;
            uint8_t buf[2048];
            if (total_samples < 1024)
            {
                buf_pcm = alloc_pcm;
                total_samples = pcm_size/2;
            }
            in_args.numInSamples = 1024;
            void *in_ptr = buf_pcm, *out_ptr = buf;
            int in_size          = 2*in_args.numInSamples;
            int in_element_size  = 2;
            int in_identifier    = IN_AUDIO_DATA;
            int out_size         = sizeof(buf);
            int out_identifier   = OUT_BITSTREAM_DATA;
            int out_element_size = 1;

            in_buf.numBufs            = 1;
            in_buf.bufs               = &in_ptr;
            in_buf.bufferIdentifiers  = &in_identifier;
            in_buf.bufSizes           = &in_size;
            in_buf.bufElSizes         = &in_element_size;
            out_buf.numBufs           = 1;
            out_buf.bufs              = &out_ptr;
            out_buf.bufferIdentifiers = &out_identifier;
            out_buf.bufSizes          = &out_size;
            out_buf.bufElSizes        = &out_element_size;

            if (AACENC_OK != aacEncEncode(aacenc, &in_buf, &out_buf, &in_args, &out_args))
            {
                printf("error: aac encode fail\n");
                exit(1);
            }
            sample  += in_args.numInSamples;
            buf_pcm += in_args.numInSamples;
            total_samples -= in_args.numInSamples;
            ats = (uint64_t)sample*90000/AUDIO_RATE;

            if (MP4E_STATUS_OK != MP4E_put_sample(mux, audio_track_id, buf, out_args.numOutBytes, 1024*90000/AUDIO_RATE, MP4E_SAMPLE_RANDOM_ACCESS))
            {
                printf("error: MP4E_put_sample failed\n");
                exit(1);
            }
        }
#endif
    }
#if ENABLE_AUDIO
    if (alloc_pcm)
        free(alloc_pcm);
    aacEncClose(&aacenc);
#endif
    if (alloc_buf)
        free(alloc_buf);
    MP4E_close(mux);
    mp4_h26x_write_close(&mp4wr);
    if (fout)
        fclose(fout);
    return 0;
}

//--------------------------

typedef GLubyte uint8_t;
typedef GLushort uint16_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern GLint glFrameBufferWidth0;
extern GLint glFrameBufferHeight0;
extern GLint glKey;
extern GLint glLastMouseX;
extern GLint glLastMouseY;
extern GLint glLastMouseB;
extern GLint glMouseX;
extern GLint glMouseY;
extern GLint glMouseB;
extern GLuint *glFrameBuffer0;

#ifdef __WATCOMC__
extern GLboolean glHasTSC;
extern GLdouble glTscFrequency;
extern GLdouble glTscStartValue;
GLuint glReadTsc();
GLuint glReadTscHi();
GLuint glHasTsc_();
GLdouble glReadTscDouble();
GLvoid glInitTSC();
#endif // __WATCOMC__

GLboolean gl2WatcomPrecisionTimer(GLboolean enable) {
#ifdef __WATCOMC__
  glHasTSC = GL_FALSE;
  if (enable)
    glInitTSC();
  return glHasTSC;
#else // __WATCOMC__
  return GL_FALSE;
#endif // __WATCOMC__
}


#ifdef __WATCOMC__
GLuint int16_1__(); // we need the zero flag it's not well exposed by the watcom api sorry that's why some asm here
#pragma aux int16_1__="mov ah,0x01""int 0x16""jz do""mov eax,0""jmp dont""do:""mov eax,1""dont:" value[eax] modify[eax ebx ecx edx]
#endif
#ifdef __DJGPP__
GLuint int16_1__() {
  GLuint ret;
  __asm__ __volatile__("\tmovb $1,%%ah\n""\tint $0x16\n""\tjz 1f\n""\tmovl $0,%%eax\n""\tjmp 2f\n""1:\n""\tmovl $1,%%eax\n""2:\n" : "=a" (ret));
  return ret;
}
#endif

GLushort gl2NextKey() {
  if (int16_1__()) return 0;
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x0000;
  int386(0x16, &regs, &regs);
  glKey = regs.w.ax;
  if ((glKey & 0xff) != 0x00) glKey &= 0xff;
  return (GLushort)glKey;
}

GLvoid gl2SetMousePos(GLint x, GLint y) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x04;
  regs.w.cx = (uint16_t)(x & 0xffff);
  regs.w.dx = (uint16_t)(y & 0xffff);
  int386(0x33, &regs, &regs);
}

GLvoid gl2NextMouseDelta(GLdouble *mouseX, GLdouble *mouseY) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseX = glMouseX;
  glLastMouseY = glMouseY;
  glMouseX = regs.w.cx & 0xffff;
  glMouseY = regs.w.dx & 0xffff;
  *mouseX = glMouseX - glLastMouseX;
  *mouseY = glMouseY - glLastMouseY;
  // center mouse
  gl2SetMousePos(glFrameBufferWidth0/2, glFrameBufferHeight0/2);
  // update lastMouseX this way
  glMouseX = glFrameBufferWidth0/2;
  glMouseY = glFrameBufferHeight0/2;
}


GLushort gl2MouseButtons() {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseB = glMouseB;
  glMouseB = regs.w.bx & 0xffff;
  return (GLushort)glMouseB;
}

GLvoid gl2SpecialKeys(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x0200;
  int386(0x16, &regs, &regs);
  uint8_t keyState = (uint8_t)(regs.w.ax & 0xff);
  *shiftKey = ((keyState & 0x03) != 0) ? GL_TRUE : GL_FALSE;
  *ctrlKey = ((keyState & 0x04) != 0) ? GL_TRUE : GL_FALSE;
  *altKey = ((keyState & 0x08) != 0) ? GL_TRUE : GL_FALSE;
}

#ifdef __DJGPP__
  typedef signed long long int int64_t;
  typedef unsigned long long int uint64_t;
  extern int64_t glStartClock;
  GLdouble gl2Seconds() {
    return (GLdouble)(uclock() - glStartClock) / UCLOCKS_PER_SEC;
  }
  GLvoid gl2SetTime(GLdouble seconds) {
    glStartClock = uclock() - (int64_t)(seconds * UCLOCKS_PER_SEC);
  }
#endif // __DJGPP__

#ifdef __WATCOMC__
  extern GLdouble glStartClock;
  GLdouble gl2Seconds() {
    if (glHasTSC) {
      return glReadTscDouble()-glTscStartValue;
    }
    printf("Recording error 1.\n");
    exit(0);
    return 0;
  }
  GLvoid gl2SetTime(GLdouble seconds) {
    if (glHasTSC) {
      glTscStartValue = glReadTscDouble()-seconds;
      return;
    }
    printf("Recording error 2.\n");
    exit(0);
  }
#endif // __WATCOMC__

#ifdef __cplusplus
}
#endif // __cplusplus

typedef enum GLRECEVENT {
  GLRECEVENT_GLSECONDS = 0,
  GLRECEVENT_GLSETTIME = 1,
  GLRECEVENT_GLNEXTKEY = 2,
  GLRECEVENT_GLNEXTMOUSEDELTA = 3,
  GLRECEVENT_GLMOUSEBUTTONS = 4,
  GLRECEVENT_GLSPECIALKEYS = 5,
  GLRECEVENT_GLREFRESH = 6,
  GLRECEVENT_GLDONE = 7,
  GLRECEVENT_GLKEYMATRIX = 8
} GLRECEVENT;

// ----------------------------------------

static GLvoid recEvent(unsigned char eventId, void *data, unsigned char dataLength) {
  if (data == NULL) dataLength = 0;
  FILE *out = fopen(glRecordFileName,"ab");
  GLdouble t = gl2Seconds();
  fwrite(&eventId,1,sizeof(eventId),out);
  fwrite(&t,1,sizeof(t),out);
  fwrite(&dataLength,1,sizeof(dataLength),out);
  if (dataLength != 0)
    fwrite(data,1,dataLength,out);
  fclose(out);
}

static GLvoid startRecording(const char *fileName) {
  FILE *out = fopen(fileName,"wb");
  if (out != NULL) fclose(out);
}

// ----------------------------------------

static unsigned char *recording = NULL;
static int recordingLength = 0;
static int recordingPos = 0;

void stopPlayBack() {
  if (recording != NULL) {free(recording); recording = NULL; recordingLength = 0;}
}

static GLvoid loadEvents(const char *fileName) {
  FILE *in = fopen(fileName,"rb"); 
  fseek(in, 0, SEEK_END);
  recordingLength = ftell(in);
  fseek(in, 0, SEEK_SET);
  stopPlayBack();
  recording = (unsigned char*)malloc(recordingLength);
  fread(recording,1,recordingLength,in);
  fclose(in);
  recordingPos = 0;
//  glDone(); printf("%d\n",recordingLength); exit(0);
}

static double playEvent(unsigned char eventId, void *data, unsigned char dataLength) {
  while(recordingPos < recordingLength) {
    unsigned char *r = &recording[recordingPos];
    unsigned char *rStart = r;
    unsigned char eEventId = *((unsigned char*)r); r+= sizeof(unsigned char);
    GLdouble eTimeStamp = *((GLdouble*)r); r += sizeof(GLdouble);
    unsigned char eDataLength = *((unsigned char*)r); r+= sizeof(unsigned char);
    void *eData = r; r += eDataLength;
    if (eEventId == eventId) {
      recordingPos += r - rStart;
      if (eDataLength != 0) 
        memcpy(data,eData,eDataLength);
      return eTimeStamp;
    } else {
      //glDone(); printf("Playback Error.. received event:%d expected event:%d\n",eEventId,eventId); exit(0);
      break; // not so good..
    }
  }
  return -1;
}

// ----------------------------------------

static GLdouble recSeconds() {
  GLdouble t = gl2Seconds();
  recEvent(GLRECEVENT_GLSECONDS,&t,sizeof(GLdouble));
  return t;                                                        
}

static GLvoid recSetTime(GLdouble seconds) {
  gl2SetTime(seconds);
  recEvent(GLRECEVENT_GLSETTIME,&seconds,sizeof(GLdouble));
}

static GLushort recNextKey() {
  GLushort k = gl2NextKey();
  recEvent(GLRECEVENT_GLNEXTKEY,&k,sizeof(GLushort));
  return k;
}

static GLvoid recNextMouseDelta(GLdouble *deltaX, GLdouble *deltaY) {
  gl2NextMouseDelta(deltaX,deltaY);
  GLdouble k[2] = {*deltaX, *deltaY};
  recEvent(GLRECEVENT_GLNEXTMOUSEDELTA,&k,sizeof(k));
}

static GLushort recMouseButtons() {
  GLushort b = gl2MouseButtons();
  recEvent(GLRECEVENT_GLMOUSEBUTTONS,&b,sizeof(GLushort));
  return b;
}

static GLvoid recSpecialKeys(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey) {
  gl2SpecialKeys(shiftKey, ctrlKey, altKey);
  GLboolean k[3] = {*shiftKey, *ctrlKey, *altKey};
  recEvent(GLRECEVENT_GLSPECIALKEYS,&k,sizeof(k));
}

static GLvoid recRefresh() {
  recEvent(GLRECEVENT_GLREFRESH,NULL,0);
}

static GLvoid recDone() {
  recEvent(GLRECEVENT_GLDONE,NULL,0);
}

// ----------------------------------------

static GLdouble playSeconds() {
  GLdouble t = 0;
  playEvent(GLRECEVENT_GLSECONDS,&t,sizeof(GLdouble));
  return t;
}

static GLvoid playSetTime(GLdouble seconds) {
  playEvent(GLRECEVENT_GLSETTIME,&seconds,sizeof(GLdouble));
}

static GLushort playCurrentKey = 0;

static GLushort playNextKey() {
  GLushort k = gl2NextKey();
  playCurrentKey = k;
  playEvent(GLRECEVENT_GLNEXTKEY,&k,sizeof(GLushort));
  if (playCurrentKey != 0) k = playCurrentKey;
  return k;
}

static GLvoid playNextMouseDelta(GLdouble *deltaX, GLdouble *deltaY) {
  GLdouble k[2];
  playEvent(GLRECEVENT_GLNEXTMOUSEDELTA,&k,sizeof(k));
  *deltaX = k[0];
  *deltaY = k[1];
}

static GLushort playMouseButtons() {
  GLushort b;
  playEvent(GLRECEVENT_GLMOUSEBUTTONS,&b,sizeof(GLushort));
  return b;
}

static GLvoid playSpecialKeys(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey) {
  GLboolean k[3];
  playEvent(GLRECEVENT_GLSPECIALKEYS,&k,sizeof(k));
  *shiftKey = k[0];
  *ctrlKey = k[1];
  *altKey = k[2];
}

static GLvoid playRefresh() {
  playEvent(GLRECEVENT_GLREFRESH,NULL,0);
}

static GLvoid playDone() {
  playEvent(GLRECEVENT_GLDONE,NULL,0);
  stopPlayBack();
}

// ----------------------------------------

static GLvoid renderRefresh() {

  if (playCurrentKey == GL_VK_END) {glDone(); printf("END pressed during h264 rendering.\n");exit(0);}

  double timeStamp = playEvent(GLRECEVENT_GLREFRESH,NULL,0);
  int frameNr = timeStamp * glRecordFrameRate;
  if (h264Writer != NULL) {
    while(frameNr > currentH264FrameWritten) {
      recSetBiosCursor(0,0);
      printf("h264frame:%d for second:%f\n",currentH264FrameWritten,timeStamp);
      if (glRecordWidth > 0 || glRecordHeight > 0) {
        unsigned int *frame = (unsigned int*)glMalloc(glRecordWidth*glRecordHeight*sizeof(unsigned int));
        for (int y = 0; y < glRecordHeight; y++) {
          unsigned int *source = &glFrameBuffer0[(y*glFrameBufferHeight/glRecordHeight)*glFrameBufferWidth];
          unsigned int txp = 0;
          unsigned int txa = 4096*glFrameBufferWidth/glRecordWidth;
          for (int x = 0; x < glRecordWidth; x++) {
            frame[x+y*glRecordWidth] = source[txp>>12];
            txp += txa;
          }
        }
        h264Writer->rgbaToFile(frame, h264File);
        glFree(frame);
      } else 
        h264Writer->rgbaToFile(glFrameBuffer0, h264File);
      currentH264FrameWritten++;
    }
  }
}

static GLvoid renderDone() {
  playEvent(GLRECEVENT_GLDONE,NULL,0);
  stopPlayBack();
  stopH264Recording();
}

// ----------------------------------------

static char line1[MAXGLRECORDFILENAMELENGTH];

static void removeLN(char *n) {
  if (strlen(n)>=1) if (n[strlen(n)-1]==0x0d||n[strlen(n)-1]==0x0a) n[strlen(n)-1] = 0;
  if (strlen(n)>=1) if (n[strlen(n)-1]==0x0d||n[strlen(n)-1]==0x0a) n[strlen(n)-1] = 0;
}

void useGLRecordWithDescFile(const char *fileName) {
  sprintf(glRecordDescFileName,"%s",fileName);
   
  int recordMode = 0;

  FILE *in = fopen(glRecordDescFileName,"r");
  if (in != NULL) {
    fgets(line1,MAXGLRECORDFILENAMELENGTH,in);
    fgets(glRecordFileName,MAXGLRECORDFILENAMELENGTH,in);
    fgets(glRecordH264FileName,MAXGLRECORDFILENAMELENGTH,in);
    fgets(glRecordMP4FileName,MAXGLRECORDFILENAMELENGTH,in);
    removeLN(glRecordFileName);
    removeLN(glRecordH264FileName);
    removeLN(glRecordMP4FileName);
    fclose(in);
    recordMode = atoi(line1);
  }

  int newRecordMode = recordMode;

  switch(recordMode) {
  case 0: {
  } break;
  case 1: {
    newRecordMode = 2; // next call is playback
    setGLRecordMode(GLRECORD_RECORDING);
  } break;
  case 2: {
    setGLRecordMode(GLRECORD_PLAYBACK);
  } break;
  case 3: {
    newRecordMode = 0; // next call is recording off
    setGLRecordMode(GLRECORD_RENDERH264);
  } break;
  }

  FILE *out = fopen(glRecordDescFileName,"w");
  fprintf(out,"%d\n",newRecordMode);
  fprintf(out,"%s\n",glRecordFileName);
  fprintf(out,"%s\n",glRecordH264FileName);
  fprintf(out,"%s\n",glRecordMP4FileName);
  fprintf(out,"#---------------------------------\n");
  fprintf(out,"# first line of this file is the mode:\n");
  fprintf(out,"# 0 = NO RECORDING\n");
  fprintf(out,"# 1 = RECORDING IN THE BACKGROUND.\n");
  fprintf(out,"# 2 = PLAYBACK OF RECORDING.\n");
  fprintf(out,"# 3 = H264/MP4 RENDERING OF THE RECORDING (use <end> key to break).\n");
  fprintf(out,"# second line of this file is the recording filename (use a ramdisk for that).\n");
  fprintf(out,"# third line of this file is the h264 filename to write.\n");
  fprintf(out,"# fourth line of this file is the mp4 filename to convert the h264 to.\n");
  fprintf(out,"#---------------------------------\n");
  fclose(out);
}

void overideGLRecordVideoResolution(int width, int height) {
  glRecordWidth = width;
  glRecordHeight = height;
}

void setGLRecordRenderFrameRate(double frameRate) {
  glRecordFrameRate = frameRate;
}

void setGLRecordFileName(const char *fileName) {
  sprintf(glRecordFileName,"%s",fileName);
}

void setGLRecordH264FileName(const char *fileName) {
  sprintf(glRecordH264FileName,"%s",fileName);
}

unsigned short glNextKeyx() {
  return gl2NextKey();
}

#define MAXNUMBEROFKEYS 256 // usually just 128
unsigned char lastKeyMatrix[MAXNUMBEROFKEYS+2];

GLvoid glKeyMatrixRecord(bool *keys, int keyCount) {
  if (keyCount > MAXNUMBEROFKEYS) return;
  int keysChanged = 0;
  for (int i = 0; i < keyCount; i++) if (keys[i] != (bool)(lastKeyMatrix[i])) keysChanged++;
  unsigned char *arr = (unsigned char*)malloc(keysChanged*2+2);
  int j = 0;
  {for (int i = 0; i < keyCount; i++) if (keys[i] != (bool)(lastKeyMatrix[i])) {arr[j*2+0] = i; arr[j*2+1] = keys[i]; j++; lastKeyMatrix[i]=keys[i];}}
  arr[j*2+0] = 255;
  arr[j*2+1] = 255;
  recEvent(GLRECEVENT_GLKEYMATRIX,arr,keysChanged*2+2);
  free(arr);
}

GLvoid glKeyMatrixPlay(bool *keys, int keyCount) {
  if (keyCount > MAXNUMBEROFKEYS) return;
  playEvent(GLRECEVENT_GLKEYMATRIX,lastKeyMatrix,-1);
  for (int i = 0; i < MAXNUMBEROFKEYS; i++) {
    if (lastKeyMatrix[i*2+0] == 255 && lastKeyMatrix[i*2+1] == 255) break;
    keys[lastKeyMatrix[i*2+0]] = lastKeyMatrix[i*2+1] ? true : false;
  }
}

typedef GLvoid (*GLKeyMatrixCallback)(bool *keys, int keyCount);
GLKeyMatrixCallback glKeyMatrixCallback = NULL;

void glKeyMatrix(bool *keys, int keyCount) {
  if (glKeyMatrixCallback != NULL) glKeyMatrixCallback(keys, keyCount);
}

void setGLRecordMode(int glRecordMode) {

  gl2WatcomPrecisionTimer(GL_TRUE); // in WatcomC this "waits" around a second.
  glSetTime(0);
  stopPlayBack();
  stopH264Recording();
  memset(lastKeyMatrix,0,sizeof(lastKeyMatrix));

  switch(glRecordMode) {
  case GLRECORD_OFF: {
    glSetSecondsCallback(NULL);
    glSetSetTimeCallback(NULL);
    glSetNextKeyCallback(NULL);
    glSetNextMouseDeltaCallback(NULL);
    glSetMouseButtonsCallback(NULL);
    glSetSpecialKeysCallback(NULL);
    glSetRefreshNotification(NULL);
    glSetDoneNotification(NULL);
    glKeyMatrixCallback = NULL;
  } break;
  case GLRECORD_RECORDING: {
    startRecording(glRecordFileName);
    glSetSecondsCallback(recSeconds);
    glSetSetTimeCallback(recSetTime);
    glSetNextKeyCallback(recNextKey);
    glSetNextMouseDeltaCallback(recNextMouseDelta);
    glSetMouseButtonsCallback(recMouseButtons);
    glSetSpecialKeysCallback(recSpecialKeys);
    glSetRefreshNotification(recRefresh);
    glSetDoneNotification(recDone);
    glKeyMatrixCallback = glKeyMatrixRecord;
  } break;
  case GLRECORD_PLAYBACK: {
    loadEvents(glRecordFileName);
    glSetSecondsCallback(playSeconds);
    glSetSetTimeCallback(playSetTime);
    glSetNextKeyCallback(playNextKey);
    glSetNextMouseDeltaCallback(playNextMouseDelta);
    glSetMouseButtonsCallback(playMouseButtons);
    glSetSpecialKeysCallback(playSpecialKeys);
    glSetRefreshNotification(playRefresh);
    glSetDoneNotification(playDone);
    glKeyMatrixCallback = glKeyMatrixPlay;
  } break;
  case GLRECORD_RENDERH264: {
    loadEvents(glRecordFileName);
    startH264Recording(glRecordH264FileName);
    glSetSecondsCallback(playSeconds);
    glSetSetTimeCallback(playSetTime);
    glSetNextKeyCallback(playNextKey);
    glSetNextMouseDeltaCallback(playNextMouseDelta);
    glSetMouseButtonsCallback(playMouseButtons);
    glSetSpecialKeysCallback(playSpecialKeys);
    glSetRefreshNotification(renderRefresh);
    glSetDoneNotification(renderDone);
    glKeyMatrixCallback = glKeyMatrixPlay;
  } break;
  }

  glSetTime(0);
}
