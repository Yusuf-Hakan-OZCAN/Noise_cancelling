#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FFT_SIZE 2048
#define OVERLAP (FFT_SIZE/32)
#define LOW_CUT 300
#define HIGH_CUT 6000
#define TRANSITION_WIDTH 100
#define NOISE_PROFILE_MS 1000
#define SPEECH_GAIN 1.5f
#define NOISE_MULTIPLIER 25.0f  // Daha agresif gürültü bastırma
#define NOISE_FLOOR 0.002f     // Daha düşük gürültü zemin seviyesi

typedef struct {
    float real;
    float imag;
} Complex;

// Iterative FFT implementation (Cooley-Tukey)
void fft(Complex *x, int N, int inverse) {
    // Bit-reversal permutation
    int j = 0;
    for (int i = 0; i < N; i++) {
        if (j > i) {
            Complex temp = x[j];
            x[j] = x[i];
            x[i] = temp;
        }
        int m = N >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    // FFT stages
    for (int stage = 1; stage < N; stage <<= 1) {
        float angle = (inverse ? 1.0f : -1.0f) * M_PI / stage;
        Complex w_step;
        w_step.real = cosf(angle);
        w_step.imag = sinf(angle);

        for (int k = 0; k < N; k += stage * 2) {
            Complex w;
            w.real = 1.0f;
            w.imag = 0.0f;

            for (int j = 0; j < stage; j++) {
                Complex t;
                t.real = w.real * x[k+j+stage].real - w.imag * x[k+j+stage].imag;
                t.imag = w.real * x[k+j+stage].imag + w.imag * x[k+j+stage].real;

                Complex u = x[k+j];

                // Butterfly operation
                x[k+j].real = u.real + t.real;
                x[k+j].imag = u.imag + t.imag;

                x[k+j+stage].real = u.real - t.real;
                x[k+j+stage].imag = u.imag - t.imag;

                // Update twiddle factor
                Complex w_temp;
                w_temp.real = w.real * w_step.real - w.imag * w_step.imag;
                w_temp.imag = w.real * w_step.imag + w.imag * w_step.real;
                w = w_temp;
            }
        }
    }

    // Scaling for inverse FFT
    if (inverse) {
        for (int i = 0; i < N; i++) {
            x[i].real /= N;
            x[i].imag /= N;
        }
    }
}

// Hann window function
void create_hann_window(float* window, int size) {
    for(int i = 0; i < size; i++) {
        window[i] = 0.5f * (1 - cosf(2 * M_PI * i / (size - 1)));
    }
}

// Improved noise threshold calculation
float calculate_noise_threshold(const float* audio, int sample_rate, int total_samples) {
    int noise_samples = sample_rate * NOISE_PROFILE_MS / 1000;
    noise_samples = (noise_samples > total_samples) ? total_samples : noise_samples;
    if (noise_samples <= 0) return 0.01f;

    // Calculate RMS in a more robust way
    int num_blocks = noise_samples / FFT_SIZE;
    if (num_blocks < 1) num_blocks = 1;

    float sum_rms = 0.0f;
    for (int b = 0; b < num_blocks; b++) {
        float block_sum_sq = 0.0f;
        int start = b * FFT_SIZE;

        for (int i = 0; i < FFT_SIZE && (start+i) < total_samples; i++) {
            block_sum_sq += audio[start+i] * audio[start+i];
        }
        sum_rms += sqrtf(block_sum_sq / FFT_SIZE);
    }

    return (sum_rms / num_blocks) * NOISE_MULTIPLIER;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("       !UYARI!\n1-YUKLEYECEGINIZ SES DOSYASI .WAV FORMATINDA OLMALIDIR.\n2-KODUN CALISABILMESI ICIN SES DOSYANIZI sinyallerproje.exe'nin BULUNDUGU DIZINE ATMALISINIZ.\n3-OLUSTURULACAK TEMIZ SES DOSYASINI AYNI DIZINDE BULABILIRSINIZ.\n---------------\nKULLANIM: %s <ses_dosyasi.wav>\n", argv[0]);
        return 1;
    }

    const char* input = argv[1];
    char output[256];
    snprintf(output, sizeof(output), "temiz_%s", input);

    avformat_network_init();

    // Open input file
    AVFormatContext* fmt_ctx = NULL;
    if(avformat_open_input(&fmt_ctx, input, NULL, NULL) < 0) {
        fprintf(stderr, "Dosya acilamadi: %s\n", input);
        return 1;
    }

    if(avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Boyle bir akis bilgisi bulunamadi.\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Find audio stream
    int stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(stream_index < 0) {
        fprintf(stderr, "Ses akisi bulunamadi.\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }
    AVStream* stream = fmt_ctx->streams[stream_index];

    // Get decoder
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!codec) {
        fprintf(stderr, "Codec bulunamadi\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Create codec context
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx) {
        fprintf(stderr, "Codec contexti oluşturulamadi\n");
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Set codec parameters
    if(avcodec_parameters_to_context(codec_ctx, stream->codecpar) < 0) {
        fprintf(stderr, "Parametreler aktarilamadi\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Open codec
    if(avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Codec acilamadi.\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Prepare for reading frames
    AVPacket pkt;
    AVFrame* frame = av_frame_alloc();
    if(!frame) {
        fprintf(stderr, "Frame olusturulamadi\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Read and convert audio data
    float* audio_data = NULL;
    int sample_count = 0;
    int num_channels = codec_ctx->ch_layout.nb_channels;
    int sample_rate = codec_ctx->sample_rate;

    while(av_read_frame(fmt_ctx, &pkt) >= 0) {
        if(pkt.stream_index == stream_index) {
            if(avcodec_send_packet(codec_ctx, &pkt) < 0) {
                av_packet_unref(&pkt);
                continue;
            }

            while(avcodec_receive_frame(codec_ctx, frame) == 0) {
                // Convert to mono by averaging channels
                int16_t* pcm = (int16_t*)frame->data[0];
                int new_size = sample_count + frame->nb_samples;
                float* new_audio_data = realloc(audio_data, new_size * sizeof(float));
                if(!new_audio_data) {
                    fprintf(stderr, "Bellek hatasi\n");
                    free(audio_data);
                    av_packet_unref(&pkt);
                    break;
                }
                audio_data = new_audio_data;

                for(int i = 0; i < frame->nb_samples; i++) {
                    float sum = 0.0f;
                    for(int c = 0; c < num_channels; c++) {
                        sum += pcm[i * num_channels + c];
                    }
                    audio_data[sample_count + i] = (sum / num_channels) / 32768.0f;
                }
                sample_count = new_size;
            }
        }
        av_packet_unref(&pkt);
    }

    // Check for sufficient samples
    if(sample_count < FFT_SIZE) {
        fprintf(stderr, "Ses dosyasi çok kisa (en az %d ornek gerekli)!\n", FFT_SIZE);
        free(audio_data);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Prepare processing
    float hann_window[FFT_SIZE];
    create_hann_window(hann_window, FFT_SIZE);
    float* output_audio = calloc(sample_count, sizeof(float));
    if(!output_audio) {
        fprintf(stderr, "Bellek hatasi\n");
        free(audio_data);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    float noise_threshold = calculate_noise_threshold(audio_data, sample_rate, sample_count);
    const float overlap_factor = (float)FFT_SIZE / OVERLAP;

    // Process audio in blocks with improved noise reduction
    for(int block = 0; block <= sample_count - FFT_SIZE; block += OVERLAP) {
        Complex fft_block[FFT_SIZE] = {{0}};

        // Apply window function
        for(int i = 0; i < FFT_SIZE; i++) {
            fft_block[i].real = audio_data[block + i] * hann_window[i];
        }

        // Forward FFT
        fft(fft_block, FFT_SIZE, 0);

        // Process frequency bins with spectral subtraction
        for(int i = 0; i <= FFT_SIZE/2; i++) {
            float freq = (float)i * sample_rate / FFT_SIZE;
            float magnitude = sqrtf(fft_block[i].real * fft_block[i].real +
                                   fft_block[i].imag * fft_block[i].imag);
            float phase = atan2f(fft_block[i].imag, fft_block[i].real);

            // Apply noise reduction using spectral subtraction
            float noise_reduced_magnitude = magnitude - noise_threshold;
            if (noise_reduced_magnitude < NOISE_FLOOR) {
                noise_reduced_magnitude = NOISE_FLOOR;
            }

            // Calculate frequency-based gain
            float freq_gain = 1.0f;

            // Low frequency roll-off with smooth transition
            if(freq < LOW_CUT) {
                if(freq < LOW_CUT - TRANSITION_WIDTH) {
                    freq_gain = 0.0f;
                } else {
                    freq_gain = (freq - (LOW_CUT - TRANSITION_WIDTH)) / TRANSITION_WIDTH;
                }
            }
            // High frequency roll-off with smooth transition
            else if(freq > HIGH_CUT) {
                if(freq > HIGH_CUT + TRANSITION_WIDTH) {
                    freq_gain = 0.0f;
                } else {
                    freq_gain = 1.0f - (freq - HIGH_CUT) / TRANSITION_WIDTH;
                }
            }
            // Speech band boost
            else {
                freq_gain = SPEECH_GAIN;
            }

            // Apply combined gains
            noise_reduced_magnitude *= freq_gain;

            // Convert back to complex
            fft_block[i].real = noise_reduced_magnitude * cosf(phase);
            fft_block[i].imag = noise_reduced_magnitude * sinf(phase);

            // Update symmetric bin (FFT symmetry)
            if(i > 0 && i < FFT_SIZE/2) {
                int sym_index = FFT_SIZE - i;
                fft_block[sym_index].real = fft_block[i].real;
                fft_block[sym_index].imag = -fft_block[i].imag;
            }
        }

        // Handle Nyquist frequency bin
        if (FFT_SIZE % 2 == 0) {
            int nyquist_index = FFT_SIZE / 2;
            float magnitude = sqrtf(fft_block[nyquist_index].real * fft_block[nyquist_index].real +
                                   fft_block[nyquist_index].imag * fft_block[nyquist_index].imag);
            float phase = atan2f(fft_block[nyquist_index].imag, fft_block[nyquist_index].real);

            float noise_reduced_magnitude = magnitude - noise_threshold;
            if (noise_reduced_magnitude < NOISE_FLOOR) {
                noise_reduced_magnitude = NOISE_FLOOR;
            }

            fft_block[nyquist_index].real = noise_reduced_magnitude * cosf(phase);
            fft_block[nyquist_index].imag = noise_reduced_magnitude * sinf(phase);
        }

        // Inverse FFT
        fft(fft_block, FFT_SIZE, 1);

        // Overlap-add with window
        for(int i = 0; i < FFT_SIZE; i++) {
            output_audio[block + i] += fft_block[i].real * hann_window[i] / overlap_factor;
        }
    }

    // Replace original audio with processed
    memcpy(audio_data, output_audio, sample_count * sizeof(float));
    free(output_audio);

    // Normalize audio to [-1, 1]
    float max_val = 0.001f;
    for(int i = 0; i < sample_count; i++) {
        float abs_val = fabsf(audio_data[i]);
        if(abs_val > max_val) max_val = abs_val;
    }
    for(int i = 0; i < sample_count; i++) {
        audio_data[i] /= max_val;
    }

    // Prepare output file
    AVFormatContext* out_ctx = NULL;
    avformat_alloc_output_context2(&out_ctx, NULL, "wav", output);
    if(!out_ctx) {
        fprintf(stderr, "Cikis dosyasi olusturulamadi.\n");
        free(audio_data);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    AVStream* out_stream = avformat_new_stream(out_ctx, NULL);
    if(!out_stream) {
        fprintf(stderr, "Cikis akisi olusturulamadi\n");
        free(audio_data);
        avformat_free_context(out_ctx);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Configure output stream (mono, 16-bit PCM)
    out_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    out_stream->codecpar->codec_id = AV_CODEC_ID_PCM_S16LE;
    out_stream->codecpar->sample_rate = sample_rate;
    out_stream->codecpar->ch_layout.nb_channels = 1;
    out_stream->codecpar->format = AV_SAMPLE_FMT_S16;
    out_stream->codecpar->bit_rate = 16 * sample_rate;
    out_stream->time_base = (AVRational){1, sample_rate};

    // Open output file
    if(avio_open(&out_ctx->pb, output, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "Cikis dosyasi acilamadi: %s\n", output);
        free(audio_data);
        avformat_free_context(out_ctx);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Write header
    if(avformat_write_header(out_ctx, NULL) < 0) {
        fprintf(stderr, "Header yazilamadi\n");
        free(audio_data);
        avio_closep(&out_ctx->pb);
        avformat_free_context(out_ctx);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return 1;
    }

    // Convert to PCM and write in chunks with correct PTS
    const int SAMPLES_PER_PACKET = 1024;
    AVPacket out_pkt;
    int64_t pts = 0;
    for(int offset = 0; offset < sample_count; offset += SAMPLES_PER_PACKET) {
        int samples_to_write = (offset + SAMPLES_PER_PACKET > sample_count) ?
                             sample_count - offset : SAMPLES_PER_PACKET;

        av_new_packet(&out_pkt, samples_to_write * sizeof(int16_t));
        int16_t* pcm_out = (int16_t*)out_pkt.data;

        for(int i = 0; i < samples_to_write; i++) {
            float sample = audio_data[offset + i];
            sample = fmaxf(-1.0f, fminf(1.0f, sample));
            pcm_out[i] = (int16_t)(sample * 32767.0f);
        }

        out_pkt.stream_index = out_stream->index;
        out_pkt.pts = pts;
        out_pkt.dts = pts;
        pts += samples_to_write;

        if(av_write_frame(out_ctx, &out_pkt) < 0) {
            fprintf(stderr, "Paket yazilamadi\n");
        }
        av_packet_unref(&out_pkt);
    }

    // Write trailer and cleanup
    av_write_trailer(out_ctx);

    free(audio_data);
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);
    avio_closep(&out_ctx->pb);
    avformat_free_context(out_ctx);

    printf("Ses dosyaniz basariyla temizlenip kaydedildi: %s\n", output);
    return 0;
}
